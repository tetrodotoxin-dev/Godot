// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/adapters/cuda/ttx_cuda_kernel.hpp"

#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

using namespace Godot::Demo;
using namespace Perimortem;

auto Godot::Demo::Adapters::Cuda::TtxCudaKernel::_bind_methods() -> void {
  godot::ClassDB::bind_method(
      godot::D_METHOD("launch", "arguments", "grid", "block", "shared_bytes"),
      &TtxCudaKernel::launch, DEFVAL(0));
  godot::ClassDB::bind_method(
      godot::D_METHOD("get_error"), &TtxCudaKernel::get_error);
}

#ifndef TOOLCHAIN_WASM

auto Godot::Demo::Adapters::Cuda::TtxCudaKernel::adopt(
    ttx_retained acquired,
    Library& library,
    const Ttx::Data::Form::Representation& form,
    Memory::Global::Vector<Argument> arguments) -> godot::Ref<TtxCudaKernel> {
  const auto abstract = acquired.operations->get_abstract(acquired.context);
  cuda_kernel api{};
  if (abstract.operations->bind(
          abstract.context, {TTX_CUDA_KERNEL_HIGH, TTX_CUDA_KERNEL_LOW},
          &api) != TTX_NEGOTIATION_SATISFIED) {
    if (acquired.operations->release(acquired.context) != TTX_RELEASE_SUCCESS) {
      godot::UtilityFunctions::push_error("CUDA kernel cleanup failed.");
    }

    return {};
  }

  godot::Ref<TtxCudaKernel> result;
  result.instantiate();
  result->retained = acquired;
  result->kernel = api;
  result->form = form;
  result->arguments = Core::Data::take(arguments);
  library.retain();
  result->library = &library;
  return result;
}

Godot::Demo::Adapters::Cuda::TtxCudaKernel::~TtxCudaKernel() {
  if (retained.operations &&
      retained.operations->release(retained.context) != TTX_RELEASE_SUCCESS) {
    godot::UtilityFunctions::push_error("CUDA kernel cleanup failed.");
  }

  if (library) {
    library->release();
  }
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

      __builtin_memcpy(target, &narrowed, sizeof(narrowed));
    } else {
      __builtin_memcpy(target, &real, sizeof(real));
    }

    return true;
  }

  if (value.get_type() != godot::Variant::INT) {
    return false;
  }

  const GDExtensionInt integer = GDExtensionInt(value);
  const Count size = Ttx::Data::Form::Schema::get_width(type);
  const bool negative = type == Value::S8 || type == Value::S16 ||
                        type == Value::S32 || type == Value::S64;
  if (negative && size < 8) {
    const GDExtensionInt bound = GDExtensionInt(1) << (size * 8 - 1);
    if (integer < -bound || integer >= bound) {
      return false;
    }
  } else if (
      !negative &&
      (integer < 0 || (size < 8 && U64(integer) >= (U64(1) << (size * 8))))) {
    return false;
  }

  __builtin_memcpy(target, reinterpret_cast<const U8*>(&integer), size);
  return true;
}

auto Godot::Demo::Adapters::Cuda::TtxCudaKernel::launch(
    const godot::Array& values,
    godot::Vector3i grid,
    godot::Vector3i block,
    GDExtensionInt shared_bytes) -> bool {
  error = "";
  if (!kernel.operations || values.size() != GDExtensionInt(arguments.get_size())) {
    error = "Kernel argument count disagrees with preparation.";
    return false;
  }

  if (grid.x <= 0 || grid.y <= 0 || grid.z <= 0 || block.x <= 0 ||
      block.y <= 0 || block.z <= 0 || shared_bytes < 0 ||
      shared_bytes > U32(-1)) {
    error = "CUDA launch dimensions must be positive.";
    return false;
  }

  const auto& representation = form;
  Memory::Global::Bytes storage;
  storage.forgetful_resize(
      representation.get_extent() + representation.get_alignment());

  auto* allocation = storage.get_data();
  auto* frame = allocation + ((-reinterpret_cast<Count>(allocation)) &
                              (representation.get_alignment() - 1));
  for (Count i = 0; i != arguments.get_size(); ++i) {
    const auto& argument = arguments.get_data()[i];
    const godot::Variant value = values[i];
    auto* target = frame + argument.offset;
    bool valid = false;
    if (argument.buffer) {
      const godot::Ref<TtxCudaBuffer> buffer = value;
      if (buffer.is_valid()) {
        const U64 address = buffer->get_address();
        __builtin_memcpy(target, &address, sizeof(address));
        valid = true;
      }
    } else if (
        argument.bytes &&
        value.get_type() == godot::Variant::PACKED_BYTE_ARRAY) {
      const godot::PackedByteArray bytes = value;
      if (Count(bytes.size()) == argument.extent) {
        __builtin_memcpy(target, bytes.ptr(), argument.extent);
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

  const cuda_launch geometry = cuda_launch(
      U32(grid.x), U32(grid.y), U32(grid.z), U32(block.x), U32(block.y),
      U32(block.z), U32(shared_bytes));

  const auto status = kernel.operations->launch(
      kernel.context, geometry,
      {&representation.get_abi(), frame, representation.get_extent()});
  if (status != CUDA_SUCCESS_STATUS) {
    error = "CUDA launch failed with status " +
            godot::String::num_int64(int(status));
    return false;
  }

  return true;
}

#else

// Godot keeps the same Resource names so projects can report unavailable
// CUDA capabilities through their existing provider-selection UI.
Godot::Demo::Adapters::Cuda::TtxCudaKernel::~TtxCudaKernel() = default;
auto Godot::Demo::Adapters::Cuda::TtxCudaKernel::launch(
    const godot::Array& values,
    godot::Vector3i grid,
    godot::Vector3i block,
    GDExtensionInt shared_bytes) -> bool {
  error = "CUDA is unavailable in the Web platform.";
  return false;
}

#endif
