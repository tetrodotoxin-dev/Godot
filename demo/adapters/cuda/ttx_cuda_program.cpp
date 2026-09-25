// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/adapters/cuda/ttx_cuda_program.hpp"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "extension/modules/imports.hpp"

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
  if (!module) {
    Godot::Extension::Modules::Imports imports(
        godot::ProjectSettings::get_singleton()->get_setting_with_override(
            "ttx/imports"));
    // The project selects the Compiler capability independently of its image
    // backend. The first compilation retains that module for this Resource.
    const auto configured =
        godot::String(
            godot::ProjectSettings::get_singleton()->get_setting(
                "ttx/cuda_compiler", "cuda"))
            .utf8();
    imports
        .open(
            Core::View::Bytes(
                reinterpret_cast<const U8*>(configured.get_data()),
                Count(configured.length())))
        .visit(
            [&](Ttx::Concept::Modules::Module& value) {
              module = Core::Data::take(value);
            },
            [&](Ttx::Data::Status) {
              error = "CUDA could not load a compiler in this environment.";
            });
    if (!module) {
      return false;
    }
  }

  Memory::Allocator::Arena arena;
  const auto copy = [&](const godot::String& value) -> perimortem_view_bytes {
    const auto encoded = value.utf8();
    const auto bytes = arena.proxy(
        {reinterpret_cast<const U8*>(encoded.get_data()),
         Count(encoded.length())});
    return {bytes.get_data(), bytes.get_size()};
  };

  Memory::Dynamic::Vector<cuda_source> included;
  const godot::Array names = headers.keys();
  for (int64_t i = 0; i != names.size(); ++i) {
    included.insert({copy(names[i]), copy(headers[names[i]])});
  }

  Memory::Dynamic::Vector<perimortem_view_bytes> selected;
  for (int64_t i = 0; i != options.size(); ++i) {
    selected.insert(copy(options[i]));
  }

  const cuda_compile_request request{
    {copy("project.cu"), copy(source)},
    included.get_view().get_data(),
    included.get_size(),
    selected.get_view().get_data(),
    selected.get_size(),
    0};

  Memory::Dynamic::Bytes diagnostic;
  bool success = false;
  module->open().visit(
      [&](Ttx::Concept::Modules::Module::Acquisition& root) {
        root.bind<::Cuda::Contracts::Compiler>().visit(
            [&](::Cuda::Contracts::Compiler compiler) {
              compiler.compile(request, diagnostic)
                  .visit(
                      [&](Ttx::Semantic::Ownership::Publication& candidate) {
                        candidate.get_query()
                            .bind<::Cuda::Contracts::Program>()
                            .visit(
                                [&](::Cuda::Contracts::Program value) {
                                  // A provider's finalizer may call the host.
                                  // Publish the complete replacement before
                                  // releasing the preceding generation.
                                  auto previous = Core::Data::take(publication);
                                  publication = Core::Data::take(candidate);
                                  program = value;
                                  success = true;
                                },
                                [&](Ttx::Semantic::Negotiation::Binding::
                                        Failure) {
                                  error =
                                      "Compiled publication did not bind "
                                      "Program.";
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
            [&](Ttx::Semantic::Negotiation::Binding::Failure) {
              error = "The imported provider does not bind the CUDA Compiler.";
            });
      },
      [&](Ttx::Data::Status) { error = "CUDA module acquisition failed."; });
  return success;
}

auto Godot::Demo::Adapters::Cuda::TtxCudaProgram::allocate(int64_t size)
    -> godot::Ref<TtxCudaBuffer> {
  if (!program || size <= 0) {
    error = "Buffer allocation requires a program and positive extent.";
    return {};
  }

  return program->allocate(size).visit(
      [&](Ttx::Semantic::Ownership::Publication& owner) {
        return TtxCudaBuffer::adopt(*module, Core::Data::take(owner));
      },
      [&](Ttx::Data::Status) -> godot::Ref<TtxCudaBuffer> {
        error = "CUDA buffer allocation failed.";
        return {};
      });
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

  return {};
}

auto Godot::Demo::Adapters::Cuda::TtxCudaProgram::prepare(
    const godot::String& entry,
    const godot::Array& description) -> godot::Ref<TtxCudaKernel> {
  using Schema = Ttx::Data::Form::Schema;
  using Representation = Ttx::Data::Form::Representation;
  if (!program) {
    error = "Compile a program before preparing a kernel.";
    return {};
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
      return {};
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
        return {};
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
      return {};
    }

    positions.emplace(Schema::Position(*schema, extent));
    native.insert({extent, prepared});
    arguments.insert(
        {value ? *value : Schema::Value::U8, extent, size, name == "buffer",
         bytes});
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
                    [&](Ttx::Semantic::Ownership::Publication& owner) {
                      result = TtxCudaKernel::adopt(
                          *module, Core::Data::take(owner), form,
                          Core::Data::take(arguments));
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
