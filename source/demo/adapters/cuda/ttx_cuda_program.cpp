// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/adapters/cuda/ttx_cuda_program.hpp"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "cuda/contracts/compiler.hpp"
#include "demo/adapters/imports.hpp"

using namespace Godot::Demo;
using namespace Perimortem;

auto Godot::Demo::Adapters::Cuda::TtxCudaProgram::_bind_methods() -> void {
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

auto Godot::Demo::Adapters::Cuda::TtxCudaProgram::compile_file(
    const godot::String& path,
    const godot::Dictionary& headers,
    const godot::PackedStringArray& options) -> bool {
  const auto file = godot::FileAccess::open(path, godot::FileAccess::READ);
  if (file.is_null()) {
    error = "CUDA project source could not be opened: " + path;
    return false;
  }

  return compile(file->get_as_text(), headers, options);
}

auto Godot::Demo::Adapters::Cuda::TtxCudaProgram::compile(
    const godot::String& source,
    const godot::Dictionary& headers,
    const godot::PackedStringArray& options) -> bool {
  error = "";
  Memory::Allocator::Arena arena;
  const auto copy = [&](const godot::String& value) -> perimortem_view_bytes {
    const auto encoded = value.utf8();
    const auto bytes = arena.proxy(
        Core::View::Bytes(
            reinterpret_cast<const U8*>(encoded.get_data()),
            Count(encoded.length())));
    return perimortem_view_bytes(bytes.get_data(), bytes.get_size());
  };

  Memory::Dynamic::Vector<cuda_source> included;
  const godot::Array names = headers.keys();
  for (int64_t i = 0; i != names.size(); ++i) {
    included.insert(cuda_source(copy(names[i]), copy(headers[names[i]])));
  }

  Memory::Dynamic::Vector<perimortem_view_bytes> selected;
  for (int64_t i = 0; i != options.size(); ++i) {
    selected.insert(copy(options[i]));
  }

  const cuda_compile_request request = cuda_compile_request(
      {copy("project.cu"), copy(source)}, included.get_view().get_data(),
      included.get_size(), selected.get_view().get_data(), selected.get_size(),
      0);

  using namespace Ttx::Semantic::Negotiation;
  Memory::Dynamic::Bytes diagnostic;
  bool success = false;
  auto receive = [&](Ttx::Concept::Abstract root) {
    if (root.supports<::Cuda::Contracts::Compiler>() ==
        Binding::Status::Unknown) {
      root = root.resolve_concept("Compiler"_view);
    }
    root.bind<::Cuda::Contracts::Compiler>().visit(
        [&](::Cuda::Contracts::Compiler compiler) {
          compiler.compile(request, diagnostic)
              .visit(
                  [&](Ttx::Concept::Policies::Borrowed acquired) {
                    acquired.bind<::Cuda::Contracts::Program>().visit(
                        [&](::Cuda::Contracts::Program value) {
                          const auto previous = publication;
                          publication = acquired;
                          program = value;
                          success = true;
                          if (previous) {
                            previous->release();
                          }
                        },
                        [&](Binding::Failure) {
                          acquired.release();
                          error = "Compiled subject did not bind Program.";
                        });
                  },
                  [&](Ttx::Data::Status) {
                    error = diagnostic.is_empty()
                                ? godot::String("CUDA compilation failed.")
                                : godot::String::utf8(
                                      reinterpret_cast<const char*>(
                                          diagnostic.get_view().get_data()),
                                      diagnostic.get_size());
                  });
        },
        [&](Binding::Failure) {
          error = "The imported provider did not bind the CUDA Compiler.";
        });
  };
  const auto configured =
      godot::String(
          godot::ProjectSettings::get_singleton()->get_setting(
              "ttx/cuda_compiler", "cuda"))
          .utf8();
  const perimortem_view_bytes input = perimortem_view_bytes(
      reinterpret_cast<const U8*>(configured.get_data()),
      Count(configured.length()));
  if (Adapters::imports().visit(
          &input,
          Ttx::Data::Form::Compiled<Ttx::Data::Form::Native<
              perimortem_view_bytes>::reference>::get_representation(),
          receive) != Binding::Status::Satisfied &&
      error.is_empty()) {
    error = "CUDA could not load a compiler in this environment.";
  }
  return success;
}

auto Godot::Demo::Adapters::Cuda::TtxCudaProgram::allocate(int64_t size)
    -> godot::Ref<TtxCudaBuffer> {
  if (!program || size <= 0) {
    error = "Buffer allocation requires a program and positive extent.";
    return godot::Ref<TtxCudaBuffer>();
  }

  using namespace Ttx::Semantic::Negotiation;
  godot::Ref<TtxCudaBuffer> result;
  program->allocate(size).visit(
      [&](Ttx::Concept::Policies::Borrowed subject) {
        result = TtxCudaBuffer::adopt(subject);
      },
      [&](Ttx::Data::Status) { error = "CUDA buffer allocation failed."; });
  return result;
}

static auto primitive(const godot::String& name)
    -> Core::Option<Ttx::Data::Form::Schema::Value> {
  using Value = Ttx::Data::Form::Schema::Value;
  const char* names[] = {"u8",  "u16", "u32", "u64", "s8",    "s16",
                         "s32", "s64", "r32", "r64", "buffer"};

  const Value values[] = {Value::U8,  Value::U16, Value::U32, Value::U64,
                          Value::S8,  Value::S16, Value::S32, Value::S64,
                          Value::R32, Value::R64, Value::U64};

  for (Count i = 0; i != 11; ++i) {
    if (name == names[i]) {
      return values[i];
    }
  }

  return Core::Option<Ttx::Data::Form::Schema::Value>();
}

auto Godot::Demo::Adapters::Cuda::TtxCudaProgram::prepare(
    const godot::String& entry,
    const godot::Array& description) -> godot::Ref<TtxCudaKernel> {
  using Schema = Ttx::Data::Form::Schema;
  using Representation = Ttx::Data::Form::Representation;
  if (!program) {
    error = "Compile a program before preparing a kernel.";
    return godot::Ref<TtxCudaKernel>();
  }

  error = "";
  Memory::Allocator::Arena arena;
  Memory::Dynamic::Vector<Schema::Position> positions;
  Memory::Dynamic::Vector<cuda_argument> native;
  Memory::Dynamic::Vector<TtxCudaKernel::Argument> arguments;
  Count extent = 0, alignment = 1;
  for (int64_t i = 0; i != description.size(); ++i) {
    const godot::Variant item = description[i];
    const bool bytes = item.get_type() == godot::Variant::DICTIONARY;
    const godot::String name =
        bytes ? godot::String("bytes") : godot::String(item);
    const auto value = primitive(name);
    if (!bytes && !value) {
      error = "Unsupported CUDA argument carrier: " + name;
      return godot::Ref<TtxCudaKernel>();
    }

    Count size = 0, align = 1;
    const Schema* schema = nullptr;
    if (bytes) {
      const godot::Dictionary spec = item;
      const int64_t supplied = spec.get("size", 0),
                    boundary = spec.get("alignment", 1);
      if (supplied <= 0 || boundary <= 0 || (boundary & (boundary - 1))) {
        error =
            "Aggregate arguments require positive size and power of two "
            "alignment.";
        return godot::Ref<TtxCudaKernel>();
      }

      size = supplied;
      align = boundary;
      schema = &arena.construct<Schema>(Schema::range(
          Ttx::Data::Form::Native<U8>::reference, size, 1, size, align));
    } else {
      schema = &arena.construct<Schema>(Schema::primitive(*value));
      size = Schema::get_width(*value);
      align = size;
    }

    extent = (extent + align - 1) & ~(align - 1);
    alignment = Core::Math::max(alignment, align);
    const Representation* prepared = nullptr;
    Representation::compile(*schema, arena)
        .visit(
            [&](const Representation& form) { prepared = &form; },
            [](Ttx::Data::Status) {});
    if (!prepared) {
      error = "CUDA argument representation could not be prepared.";
      return godot::Ref<TtxCudaKernel>();
    }

    positions.emplace(Schema::Position(*schema, extent));
    native.insert(cuda_argument(extent, prepared));
    arguments.insert(
        TtxCudaKernel::Argument(
            value ? *value : Schema::Value::U8, extent, size, name == "buffer",
            bytes));
    extent += size;
  }

  extent = (extent + alignment - 1) & ~(alignment - 1);
  const auto schema =
      Schema::composite(positions.get_view(), extent, alignment);
  godot::Ref<TtxCudaKernel> result;
  Representation::compile(schema, arena)
      .visit(
          [&](const Representation& form) {
            const auto named = entry.utf8();
            Memory::Dynamic::Bytes diagnostic;
            program
                ->prepare(
                    {reinterpret_cast<const U8*>(named.get_data()),
                     Count(named.length())},
                    form, native.get_view(), diagnostic)
                .visit(
                    [&](Ttx::Concept::Policies::Borrowed subject) {
                      result = TtxCudaKernel::adopt(
                          subject, form, Core::Data::take(arguments));
                    },
                    [&](Ttx::Data::Status status) {
                      error = "CUDA kernel preparation failed with status " +
                              godot::String::num_int64(int(status));
                    });
          },
          [&](Ttx::Data::Status) {
            error = "CUDA argument frame could not be prepared.";
          });
  return result;
}

Godot::Demo::Adapters::Cuda::TtxCudaProgram::~TtxCudaProgram() {
  if (publication) {
    publication->release();
  }
}
