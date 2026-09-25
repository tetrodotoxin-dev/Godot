// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include "perimortem/core/option.hpp"

#include "cuda/contracts/buffer.hpp"
#include "ttx/concept/modules/module.hpp"
#include "ttx/semantic/ownership/publication.hpp"

namespace Godot::Demo::Adapters::Cuda {

// Godot retains an ordinary object while the CUDA publication owns its device
// storage. Keeping a separate module reference makes the final release callable
// even when the program Resource that allocated this buffer has disappeared.
class TtxCudaBuffer : public godot::RefCounted {
  GDCLASS(TtxCudaBuffer, godot::RefCounted)
 public:
  static auto adopt(
      Ttx::Concept::Modules::Module module,
      Ttx::Semantic::Ownership::Publication publication)
      -> godot::Ref<TtxCudaBuffer>;
  auto get_address() const -> U64;
  auto get_size() const -> int64_t;
  auto write(const godot::PackedByteArray& bytes, int64_t offset = 0) -> bool;
  auto read() const -> godot::PackedByteArray;

 protected:
  static auto _bind_methods() -> void;

 private:
  Perimortem::Core::Option<Ttx::Concept::Modules::Module> module;
  Perimortem::Core::Option<Ttx::Semantic::Ownership::Publication> owner;
  Perimortem::Core::Option<::Cuda::Contracts::Buffer> buffer;
};

}  // namespace Godot::Demo::Adapters::Cuda
