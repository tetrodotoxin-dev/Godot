// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "demo/imaging/contracts/provider.h"

#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"

namespace Godot::Demo::Adapters::Imaging::Scripts {

// The host keeps a strong Godot reference inside each published image. Script
// calls finish synchronously, and returned images receive their own acquisition
// before the temporary Variant leaves scope. Graph consumers use the same
// Pixels, Offers and transform contracts as a native provider.
class Bridge {
 public:
  static auto create(
      const godot::Ref<godot::RefCounted>& factory,
      U32 width,
      U32 height,
      const godot::PackedByteArray& pixels,
      ttx_retained& output,
      godot::String& error) -> bool;
};

}  // namespace Godot::Demo::Adapters::Imaging::Scripts
