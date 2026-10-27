// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "toolchain/toolchain.h"

#include "demo/adapters/cuda/library.hpp"

#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"

#ifndef TOOLCHAIN_WASM
#include "cuda/contracts/buffer.h"
#endif
#include "ttx/graph/policies/retained.h"

namespace Godot::Demo::Adapters::Cuda {

// This Godot object holds one Retained acquisition for its device storage.
// Buffers remain usable after their creating Program is released. Its Library
// reference keeps the supplying code loaded through final release.
class TtxCudaBuffer : public godot::RefCounted {
  GDCLASS(TtxCudaBuffer, godot::RefCounted)
 public:
  ~TtxCudaBuffer() override;

#ifndef TOOLCHAIN_WASM

  static auto adopt(ttx_retained result, Library& library)
      -> godot::Ref<TtxCudaBuffer>;

#endif

  auto get_address() const -> U64;
  auto get_size() const -> GDExtensionInt;
  auto write(const godot::PackedByteArray& bytes, GDExtensionInt offset = 0) -> bool;
  auto read() const -> godot::PackedByteArray;

 protected:
  static auto _bind_methods() -> void;

 private:

#ifndef TOOLCHAIN_WASM

  ttx_retained retained{};
  cuda_buffer buffer{};
  Library* library = nullptr;

#endif
};

}  // namespace Godot::Demo::Adapters::Cuda
