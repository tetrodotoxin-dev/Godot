// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/vector3i.hpp>

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "cuda/contracts/kernel.hpp"
#include "demo/adapters/cuda/ttx_cuda_buffer.hpp"

namespace Godot::Demo::Adapters::Cuda {

// Kernel is a prepared Godot conversion into one CUDA argument frame. Each
// argument has a fixed kind and offset, so repeated calls only marshal values
// and invoke the retained kernel. Buffer arguments retain their Godot objects
// for the synchronous call and contribute device addresses rather than pixels.
class TtxCudaKernel : public godot::RefCounted {
  GDCLASS(TtxCudaKernel, godot::RefCounted)
 public:
  struct Argument {
    Ttx::Data::Form::Schema::Value type;
    Count offset;
    Count extent;
    bool buffer;
    bool bytes;
  };

  static auto adopt(
      Ttx::Concept::Modules::Module module,
      Ttx::Semantic::Ownership::Publication publication,
      const Ttx::Data::Form::Representation& form,
      Perimortem::Memory::Dynamic::Vector<Argument> arguments)
      -> godot::Ref<TtxCudaKernel>;
  auto launch(
      const godot::Array& values,
      godot::Vector3i grid,
      godot::Vector3i block,
      int64_t shared_bytes = 0) -> bool;
  auto get_error() const -> godot::String { return error; }

 protected:
  static auto _bind_methods() -> void;

 private:
  Perimortem::Core::Option<Ttx::Concept::Modules::Module> module;
  Perimortem::Core::Option<Ttx::Semantic::Ownership::Publication> owner;
  Perimortem::Core::Option<::Cuda::Contracts::Kernel> kernel;
  Perimortem::Memory::Dynamic::Bytes form;
  Perimortem::Memory::Dynamic::Vector<Argument> arguments;
  godot::String error;
};

}  // namespace Godot::Demo::Adapters::Cuda
