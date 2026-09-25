// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/adapters/cuda/ttx_cuda_kernel.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace Godot::Demo;
using namespace Perimortem;

auto Godot::Demo::Adapters::Cuda::TtxCudaKernel::_bind_methods() -> void {
  godot::ClassDB::bind_method(
      godot::D_METHOD("launch", "arguments", "grid", "block", "shared_bytes"),
      &TtxCudaKernel::launch, DEFVAL(0));
  godot::ClassDB::bind_method(
      godot::D_METHOD("get_error"), &TtxCudaKernel::get_error);
}

auto Godot::Demo::Adapters::Cuda::TtxCudaKernel::adopt(
    Ttx::Concept::Modules::Module module,
    Ttx::Semantic::Ownership::Publication publication,
    const Ttx::Data::Form::Representation& form,
    Memory::Dynamic::Vector<Argument> arguments) -> godot::Ref<TtxCudaKernel> {
  godot::Ref<TtxCudaKernel> result;
  publication.get_query().bind<::Cuda::Contracts::Kernel>().visit(
      [&](::Cuda::Contracts::Kernel kernel) {
        result.instantiate();
        result->module = Core::Data::take(module);
        result->owner = Core::Data::take(publication);
        result->kernel = kernel;
        result->form = Memory::Dynamic::Bytes(form.get_bytes());
        result->arguments = Core::Data::take(arguments);
      },
      [](Ttx::Semantic::Negotiation::Binding::Failure) {});
  return result;
}

// The engine supplies signed integers and double precision values. Narrowing
// is explicit here at the Godot boundary, before CUDA receives the native
// frame.
static auto scalar(
    const godot::Variant& value,
    Ttx::Data::Form::Schema::Value type,
    U8* target) -> bool {
  using Value = Ttx::Data::Form::Schema::Value;
  if (type == Value::R32 || type == Value::R64) {
    if (value.get_type() != godot::Variant::FLOAT &&
        value.get_type() != godot::Variant::INT) {
      return false;
    }

    const R64 real = double(value);
    if (type == Value::R32) {
      const R32 narrowed = real;
      if (__builtin_isfinite(real) && !__builtin_isfinite(narrowed)) {
        return false;
      }

      Core::Data::copy(target, &narrowed);
    } else {
      Core::Data::copy(target, &real);
    }

    return true;
  }

  if (value.get_type() != godot::Variant::INT) {
    return false;
  }

  const S64 integer = int64_t(value);
  const Count size = Ttx::Data::Form::Schema::get_width(type);
  const bool negative = type == Value::S8 || type == Value::S16 ||
                        type == Value::S32 || type == Value::S64;
  if (negative && size < 8) {
    const S64 bound = S64(1) << (size * 8 - 1);
    if (integer < -bound || integer >= bound) {
      return false;
    }
  } else if (
      !negative &&
      (integer < 0 || (size < 8 && U64(integer) >= (U64(1) << (size * 8))))) {
    return false;
  }

  Core::Data::copy(target, reinterpret_cast<const U8*>(&integer), size);
  return true;
}

auto Godot::Demo::Adapters::Cuda::TtxCudaKernel::launch(
    const godot::Array& values,
    godot::Vector3i grid,
    godot::Vector3i block,
    int64_t shared_bytes) -> bool {
  error = "";
  if (!kernel || values.size() != int64_t(arguments.get_size())) {
    error = "Kernel argument count disagrees with preparation.";
    return false;
  }

  if (grid.x <= 0 || grid.y <= 0 || grid.z <= 0 || block.x <= 0 ||
      block.y <= 0 || block.z <= 0 || shared_bytes < 0 ||
      shared_bytes > U32(-1)) {
    error = "CUDA launch dimensions must be positive.";
    return false;
  }

  const Ttx::Data::Form::Representation representation(
      form.get_view().get_data(), form.get_size());
  auto* allocation = static_cast<U8*>(__builtin_alloca(
      representation.get_extent() + representation.get_alignment()));
  auto* frame = allocation + ((-reinterpret_cast<Count>(allocation)) &
                              (representation.get_alignment() - 1));
  for (Count i = 0; i != arguments.get_size(); ++i) {
    const auto& argument = arguments[i];
    const godot::Variant value = values[i];
    auto* target = frame + argument.offset;
    bool valid = false;
    if (argument.buffer) {
      const godot::Ref<TtxCudaBuffer> buffer = value;
      if (buffer.is_valid()) {
        const U64 address = buffer->get_address();
        Core::Data::copy(target, &address);
        valid = true;
      }
    } else if (
        argument.bytes &&
        value.get_type() == godot::Variant::PACKED_BYTE_ARRAY) {
      const godot::PackedByteArray bytes = value;
      if (Count(bytes.size()) == argument.extent) {
        Core::Data::copy(target, bytes.ptr(), argument.extent);
        valid = true;
      }
    } else if (!argument.bytes) {
      valid = scalar(value, argument.type, target);
    }

    if (!valid) {
      error = "Kernel argument cannot satisfy its prepared carrier.";
      return false;
    }
  }

  const cuda_launch geometry{U32(grid.x),      U32(grid.y),  U32(grid.z),
                             U32(block.x),     U32(block.y), U32(block.z),
                             U32(shared_bytes)};

  const auto status = kernel->launch(
      geometry, Ttx::Data::Form::Storage(
                    {&representation, frame, representation.get_extent()}));
  if (status != Ttx::Data::Status::Success) {
    error = "CUDA launch failed with status " +
            godot::String::num_int64(int(status));
    return false;
  }

  return true;
}
