// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <godot_cpp/classes/ref_counted.hpp>

#include "perimortem/core/option.hpp"

#include "sampling/function.hpp"

namespace Godot::Extension {

// One Godot owner keeps a fulfilled sampling function and its module alive.
// Counts return ordinary integers, so a caller can repeat or partition work
// without constructing Resources or recording dependency nodes. Configuration
// is outside that hot path, and a failed replacement preserves the old
// function.
class TtxSampler : public godot::RefCounted {
  GDCLASS(TtxSampler, godot::RefCounted)
 public:
  auto configure(const godot::String& provider) -> bool;
  auto count(int64_t seed, int64_t first, int64_t size) -> int64_t;
  auto get_error() const -> godot::String { return error; }

 protected:
  static void _bind_methods();

 private:
  Perimortem::Core::Option<Sampling::Function> function;
  godot::String error;
};

}  // namespace Godot::Extension
