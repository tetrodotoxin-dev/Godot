// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "demo/imaging/cpu/image.hpp"

namespace Godot::Demo::Imaging::Cpu {

// An overlay can come from another module, so composition observes its public
// pixels before combining them with the receiver's local bytes. The integer
// source over calculation keeps rounding and transparent color deterministic.
class Composite {
 public:
  static auto apply(const Image& image, image_object overlay)
      -> Perimortem::Utility::Result<Image&, Perimortem::Core::View::Bytes>;
};

}  // namespace Godot::Demo::Imaging::Cpu
