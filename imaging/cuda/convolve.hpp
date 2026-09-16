// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "imaging/cuda/image.hpp"

namespace Godot::Imaging::Cuda {

// The CUDA path keeps images, padded planes and transform products on the GPU.
// Only kernel coefficients move from the host during convolution.
// The cropped result remains resident until its pixel contract is observed.
class Convolve {
 public:
  static auto apply(const Image& image, image_kernel kernel)
      -> Perimortem::Utility::Result<Image&, Perimortem::Core::View::Bytes>;
};

}  // namespace Godot::Imaging::Cuda
