// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/adapters/cuda/ttx_cuda_program.hpp"

#include "godot_cpp/classes/file_access.hpp"
#include "godot_cpp/classes/project_settings.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#ifndef TOOLCHAIN_WASM
#include "cuda/contracts/compiler.h"
#endif
#include "perimortem/core/view/null_terminated.hpp"
#include "perimortem/memory/global/vector.hpp"

#include "ttx/data/form/native.hpp"

using namespace Perimortem;
using namespace Godot::Demo::Adapters::Cuda;
namespace Form = Ttx::Data::Form;

void TtxCudaProgram::_bind_methods() {
  godot::ClassDB::bind_method(
      godot::D_METHOD("compile", "source", "headers", "options"),
      &TtxCudaProgram::compile, DEFVAL(godot::Dictionary()),
      DEFVAL(godot::PackedStringArray()));
  godot::ClassDB::bind_method(
      godot::D_METHOD("compile_file", "path", "headers", "options"),
      &TtxCudaProgram::compile_file, DEFVAL(godot::Dictionary()),
      DEFVAL(godot::PackedStringArray()));
  godot::ClassDB::bind_method(
      godot::D_METHOD("allocate", "size"), &TtxCudaProgram::allocate);
  godot::ClassDB::bind_method(
      godot::D_METHOD("prepare", "entry", "arguments"),
      &TtxCudaProgram::prepare);
  godot::ClassDB::bind_method(
      godot::D_METHOD("get_error"), &TtxCudaProgram::get_error);
}

#ifndef TOOLCHAIN_WASM

auto TtxCudaProgram::compile_file(
    const godot::String& path,
    const godot::Dictionary& headers,
    const godot::PackedStringArray& options) -> bool {
  const auto file = godot::FileAccess::open(path, godot::FileAccess::READ);
  if (file.is_null()) {
    error = "CUDA source could not be opened: " + path;
    return false;
  }

  return compile(file->get_as_text(), headers, options);
}

static auto diagnostic(void* context, perimortem_view_bytes message) -> void {
  *Core::Data::cast<godot::String>(context) += godot::String::utf8(
      Core::Data::cast<const char>(message.data), message.size);
}

auto TtxCudaProgram::compile(
    const godot::String& source,
    const godot::Dictionary& headers,
    const godot::PackedStringArray& options) -> bool {
  error = godot::String();

  const godot::String path =
      godot::ProjectSettings::get_singleton()->get_setting(
          "ttx/cuda_compiler", "res://addons/godot_ttx/libttx_cuda.so");
  Library* loaded = Library::open(path, error);
  if (!loaded) {
    return false;
  }

  auto abstract = loaded->get_abstract();
  cuda_compiler compiler{};
  if (abstract.operations->bind(
          abstract.context, {TTX_CUDA_COMPILER_HIGH, TTX_CUDA_COMPILER_LOW},
          &compiler) != TTX_NEGOTIATION_SATISFIED) {
    abstract =
        abstract.operations->resolve_route(abstract.context, "compiler"_view);
    if (abstract.operations->bind(
            abstract.context, {TTX_CUDA_COMPILER_HIGH, TTX_CUDA_COMPILER_LOW},
            &compiler) != TTX_NEGOTIATION_SATISFIED) {
      error = "The selected module does not provide a CUDA compiler.";
      loaded->release();
      return false;
    }
  }

  Memory::Allocator::Arena arena;
  auto copy = [&](const godot::String& value) -> perimortem_view_bytes {
    const auto encoded = value.utf8();
    return arena.copy(
        Core::View::Bytes(
            perimortem_view_bytes{
              Core::Data::cast<const U8>(encoded.get_data()),
              Count(encoded.length())}));
  };

  Memory::Global::Vector<cuda_source> includes;
  const auto names = headers.keys();
  for (GDExtensionInt i = 0; i < names.size(); ++i) {
    includes.insert({copy(names[i]), copy(headers[names[i]])});
  }

  Memory::Global::Vector<perimortem_view_bytes> arguments;
  for (GDExtensionInt i = 0; i < options.size(); ++i) {
    arguments.insert(copy(options[i]));
  }

  const cuda_compile_request request{
    {copy("project.cu"), copy(source)},
    includes.get_data(),
    includes.get_size(),
    arguments.get_data(),
    arguments.get_size(),
    0};

  ttx_retained acquired{};
  const auto status = compiler.operations->compile(
      compiler.context, request, {&error, diagnostic}, &acquired);
  if (status != CUDA_SUCCESS_STATUS) {
    if (error.is_empty()) {
      error = "CUDA compilation failed.";
    }

    loaded->release();
    return false;
  }

  const auto result = acquired.operations->get_abstract(acquired.context);
  cuda_program api{};
  if (result.operations->bind(
          result.context, {TTX_CUDA_PROGRAM_HIGH, TTX_CUDA_PROGRAM_LOW},
          &api) != TTX_NEGOTIATION_SATISFIED) {
    if (acquired.operations->release(acquired.context) != TTX_RELEASE_SUCCESS) {
      godot::UtilityFunctions::push_error("CUDA program cleanup failed.");
    }

    loaded->release();
    error = "Compiled result does not provide Program.";
    return false;
  }

  const auto previous = retained;
  auto* previous_library = library;
  retained = acquired;
  program = api;
  library = loaded;
  if (previous.operations &&
      previous.operations->release(previous.context) != TTX_RELEASE_SUCCESS) {
    godot::UtilityFunctions::push_error("CUDA program cleanup failed.");
  }

  if (previous_library) {
    previous_library->release();
  }

  return true;
}

auto TtxCudaProgram::allocate(GDExtensionInt size) -> godot::Ref<TtxCudaBuffer> {
  if (!program.operations || size <= 0) {
    error = "Buffer allocation requires a program and positive size.";
    return {};
  }

  ttx_retained acquired{};
  if (program.operations->allocate(program.context, size, &acquired) !=
      CUDA_SUCCESS_STATUS) {
    error = "CUDA buffer allocation failed.";
    return {};
  }

  return TtxCudaBuffer::adopt(acquired, *library);
}

auto TtxCudaProgram::prepare(
    const godot::String& entry,
    const godot::Array& description) -> godot::Ref<TtxCudaKernel> {
  if (!program.operations) {
    error = "Compile a program before preparing a kernel.";
    return {};
  }

  if (description.size() > 256) {
    error = "Kernel arguments exceed the lab's limit.";
    return {};
  }

  error = godot::String();

  using Schema = Form::Schema;
  using Value = Schema::Value;
  Memory::Global::Vector<Schema> schemas(description.size());
  Memory::Global::Vector<Schema::Position> positions;
  Memory::Global::Vector<Form::Representation> forms(description.size());
  Memory::Global::Vector<cuda_argument> native;
  Memory::Global::Vector<TtxCudaKernel::Argument> arguments;
  Count extent = 0, alignment = 1;
  for (GDExtensionInt i = 0; i < description.size(); ++i) {
    const godot::Variant item = description[i];
    const bool bytes = item.get_type() == godot::Variant::DICTIONARY;
    const godot::String name =
        bytes ? godot::String("bytes") : godot::String(item);
    const char* names[]{"u8",  "u16", "u32", "u64", "s8",    "s16",
                        "s32", "s64", "r32", "r64", "buffer"};

    const Value types[]{Value::U8,  Value::U16, Value::U32, Value::U64,
                        Value::S8,  Value::S16, Value::S32, Value::S64,
                        Value::R32, Value::R64, Value::U64};

    int selected = -1;
    for (int n = 0; n < 11; ++n) {
      if (name == names[n]) {
        selected = n;
      }
    }

    if (!bytes && selected < 0) {
      error = "Unsupported kernel argument: " + name;
      return {};
    }

    Count size, boundary;
    if (bytes) {
      const godot::Dictionary spec = item;
      const GDExtensionInt length = spec.get("size", 0), align = spec.get("alignment", 1);
      if (length <= 0 || length > 1048576 || align <= 0 || align > 4096 ||
          (align & (align - 1))) {
        error = "Invalid aggregate size or alignment.";
        return {};
      }

      size = length;
      boundary = align;
      schemas.insert(
          Schema::range(Form::Native<U8>::reference, size, 1, size, boundary));
    } else {
      size = Schema::get_width(types[selected]);
      boundary = size;
      schemas.insert(Schema::primitive(types[selected]));
    }

    extent = (extent + boundary - 1) & ~(boundary - 1);
    if (boundary > alignment) {
      alignment = boundary;
    }

    bool compiled = false;
    Form::Representation::compile(schemas.get_data()[i])
        .visit(
            [&](Form::Representation& form) {
              forms.insert(Core::Data::take(form));
              compiled = true;
            },

            [&](Ttx::Data::Status status) {});
    if (!compiled) {
      error = "Argument representation could not be compiled.";
      return {};
    }

    positions.insert(Schema::Position(schemas.get_data()[i], extent));
    native.insert(cuda_argument{extent, &forms.get_data()[i].get_abi()});
    arguments.insert(
        TtxCudaKernel::Argument{
          bytes ? Value::U8 : types[selected], extent, size, name == "buffer",
          bytes});

    extent += size;
  }

  extent = (extent + alignment - 1) & ~(alignment - 1);

  const auto schema =
      Schema::composite(positions.get_view(), extent, alignment);
  godot::Ref<TtxCudaKernel> result;
  Form::Representation::compile(schema).visit(
      [&](Form::Representation& form) {
        const auto name = entry.utf8();
        ttx_retained acquired{};
        const auto status = program.operations->prepare(
            program.context,
            {Core::Data::cast<const U8>(name.get_data()), Count(name.length())},
            &form.get_abi(), native.get_data(), native.get_size(),
            {&error, diagnostic}, &acquired);
        if (status == CUDA_SUCCESS_STATUS) {
          result = TtxCudaKernel::adopt(
              acquired, *library, form, Core::Data::take(arguments));
        } else if (error.is_empty()) {
          error = "CUDA kernel preparation failed.";
        }
      },

      [&](Ttx::Data::Status status) {
        error = "Kernel frame could not be compiled.";
      });

  return result;
}

TtxCudaProgram::~TtxCudaProgram() {
  if (retained.operations &&
      retained.operations->release(retained.context) != TTX_RELEASE_SUCCESS) {
    godot::UtilityFunctions::push_error("CUDA program cleanup failed.");
  }

  if (library) {
    library->release();
  }
}

#else

// Godot keeps the same Resource names so projects can report unavailable
// CUDA capabilities through their existing provider-selection UI.
TtxCudaProgram::~TtxCudaProgram() = default;
auto TtxCudaProgram::compile_file(
    const godot::String& path,
    const godot::Dictionary& headers,
    const godot::PackedStringArray& options) -> bool {
  error = "CUDA is unavailable in the Web platform.";
  return false;
}

auto TtxCudaProgram::compile(
    const godot::String& source,
    const godot::Dictionary& headers,
    const godot::PackedStringArray& options) -> bool {
  error = "CUDA is unavailable in the Web platform.";
  return false;
}

auto TtxCudaProgram::allocate(GDExtensionInt size) -> godot::Ref<TtxCudaBuffer> {
  error = "CUDA is unavailable in the Web platform.";
  return {};
}

auto TtxCudaProgram::prepare(
    const godot::String& entry,
    const godot::Array& description) -> godot::Ref<TtxCudaKernel> {
  error = "CUDA is unavailable in the Web platform.";
  return {};
}

#endif
