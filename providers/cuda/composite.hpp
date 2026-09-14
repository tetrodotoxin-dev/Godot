// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "providers/cuda/image.hpp"

namespace Godot::Providers::Cuda {

// Composition can combine a foreign CPU overlay with a resident CUDA image.
// It observes and uploads the overlay through the public contract, then blends
// against the unchanged device background without downloading that background.
class Composite {
 public:
  static auto apply(const Image& image, image_object overlay)
      -> Perimortem::Utility::Result<Image&, Perimortem::Core::View::Bytes>;
};

}  // namespace Godot::Providers::Cuda
