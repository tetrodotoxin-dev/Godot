// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "imaging/cuda/image.hpp"

namespace Godot::Imaging::Cuda {

// Inversion launches over the existing device buffer and publishes another
// device allocation. No host observation is required to complement RGB while
// carrying the source alpha into the result.
class Invert {
 public:
  static auto apply(const Image& image)
      -> Perimortem::Utility::Result<Image&, Perimortem::Core::View::Bytes>;
};

}  // namespace Godot::Imaging::Cuda
