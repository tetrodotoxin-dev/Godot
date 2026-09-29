// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include "perimortem/core/option.hpp"

#include "cuda/contracts/buffer.hpp"
#include "ttx/concept/policies/borrowed.hpp"

namespace Godot::Demo::Adapters::Cuda {

// This Godot object owns the Borrowed CUDA answer for its device storage.
// Buffers remain usable after their creating Program is released. The host
// import service keeps the supplying code loaded through resource destruction.
class TtxCudaBuffer : public godot::RefCounted {
  GDCLASS(TtxCudaBuffer, godot::RefCounted)
 public:
  ~TtxCudaBuffer() override;
  static auto adopt(Ttx::Concept::Policies::Borrowed subject)
      -> godot::Ref<TtxCudaBuffer>;
  auto get_address() const -> U64;
  auto get_size() const -> int64_t;
  auto write(const godot::PackedByteArray& bytes, int64_t offset = 0) -> bool;
  auto read() const -> godot::PackedByteArray;

 protected:
  static auto _bind_methods() -> void;

 private:
  Perimortem::Core::Option<Ttx::Concept::Policies::Borrowed> owner;
  Perimortem::Core::Option<::Cuda::Contracts::Buffer> buffer;
};

}  // namespace Godot::Demo::Adapters::Cuda
