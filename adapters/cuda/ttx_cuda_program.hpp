// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include "adapters/cuda/ttx_cuda_kernel.hpp"
#include "cuda/contracts/program.hpp"

namespace Godot::Adapters::Cuda {

// A project resource supplies source and named includes to an independently
// bound compiler. Successful preparation replaces only this Resource's current
// publication. Kernels and buffers retain earlier publications independently,
// and failed compilation leaves the current program available.
class TtxCudaProgram : public godot::Resource {
  GDCLASS(TtxCudaProgram, godot::Resource)
 public:
  auto compile(
      const godot::String& source,
      const godot::Dictionary& headers = {},
      const godot::PackedStringArray& options = {}) -> bool;
  auto compile_file(
      const godot::String& path,
      const godot::Dictionary& headers = {},
      const godot::PackedStringArray& options = {}) -> bool;
  auto allocate(int64_t size) -> godot::Ref<TtxCudaBuffer>;
  auto prepare(const godot::String& entry, const godot::Array& arguments)
      -> godot::Ref<TtxCudaKernel>;
  auto get_error() const -> godot::String { return error; }

 protected:
  static auto _bind_methods() -> void;

 private:
  Perimortem::Core::Option<Ttx::Concept::Modules::Module> module;
  Perimortem::Core::Option<Ttx::Semantic::Ownership::Publication> publication;
  Perimortem::Core::Option<::Cuda::Contracts::Program> program;
  godot::String error;
};

}  // namespace Godot::Adapters::Cuda
