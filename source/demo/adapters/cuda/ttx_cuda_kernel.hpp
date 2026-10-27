// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "toolchain/toolchain.h"

#include "perimortem/memory/global/bytes.hpp"
#include "perimortem/memory/global/vector.hpp"

#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/variant/array.hpp"
#include "godot_cpp/variant/vector3i.hpp"

#ifndef TOOLCHAIN_WASM
#include "cuda/contracts/kernel.h"
#endif
#include "ttx/data/form/representation.hpp"

#include "demo/adapters/cuda/ttx_cuda_buffer.hpp"

namespace Godot::Demo::Adapters::Cuda {

// Kernel is a prepared Godot conversion into one CUDA argument frame. Each
// argument has a fixed kind and offset, so repeated calls only marshal values
// and invoke the retained kernel. Buffer arguments retain their Godot objects
// for the synchronous call and contribute device addresses rather than pixels.
class TtxCudaKernel : public godot::RefCounted {
  GDCLASS(TtxCudaKernel, godot::RefCounted)
 public:
  ~TtxCudaKernel() override;
  struct Argument {
    Ttx::Data::Form::Schema::Value type;
    Count offset;
    Count extent;
    bool buffer;
    bool bytes;
  };

#ifndef TOOLCHAIN_WASM

  static auto adopt(
      ttx_retained result,
      Library& library,
      const Ttx::Data::Form::Representation& form,
      Perimortem::Memory::Global::Vector<Argument> arguments)
      -> godot::Ref<TtxCudaKernel>;

#endif

  auto launch(
      const godot::Array& values,
      godot::Vector3i grid,
      godot::Vector3i block,
      GDExtensionInt shared_bytes = 0) -> bool;
  auto get_error() const -> godot::String { return error; }

 protected:
  static auto _bind_methods() -> void;

 private:

#ifndef TOOLCHAIN_WASM

  ttx_retained retained{};
  cuda_kernel kernel{};
  Library* library = nullptr;
  Ttx::Data::Form::Representation form;
  Perimortem::Memory::Global::Vector<Argument> arguments;

#endif

  godot::String error;
};

}  // namespace Godot::Demo::Adapters::Cuda
