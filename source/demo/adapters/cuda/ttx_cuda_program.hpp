// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "toolchain/toolchain.h"

#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/resource.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/variant/packed_string_array.hpp"

#ifndef TOOLCHAIN_WASM
#include "cuda/contracts/program.h"
#endif
#include "demo/adapters/cuda/ttx_cuda_kernel.hpp"

namespace Godot::Demo::Adapters::Cuda {

// A project resource supplies source and named includes to an independently
// bound compiler. Successful preparation replaces only this Resource's current
// publication. Kernels and buffers retain earlier publications independently,
// and failed compilation leaves the current program available.
class TtxCudaProgram : public godot::Resource {
  GDCLASS(TtxCudaProgram, godot::Resource)
 public:
  ~TtxCudaProgram() override;
  auto compile(
      const godot::String& source,
      const godot::Dictionary& headers = {},
      const godot::PackedStringArray& options = {}) -> bool;
  auto compile_file(
      const godot::String& path,
      const godot::Dictionary& headers = {},
      const godot::PackedStringArray& options = {}) -> bool;
  auto allocate(GDExtensionInt size) -> godot::Ref<TtxCudaBuffer>;
  auto prepare(const godot::String& entry, const godot::Array& arguments)
      -> godot::Ref<TtxCudaKernel>;
  auto get_error() const -> godot::String { return error; }

 protected:
  static auto _bind_methods() -> void;

 private:

#ifndef TOOLCHAIN_WASM

  ttx_retained retained{};
  cuda_program program{};
  Library* library = nullptr;

#endif

  godot::String error;
};

}  // namespace Godot::Demo::Adapters::Cuda
