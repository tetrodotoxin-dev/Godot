// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "imaging/cpu/image.hpp"

namespace Godot::Imaging::Cpu {

// FFT convolution replaces a large spatial neighborhood with transforms and
// a pointwise product. This CPU implementation owns FFTW planning and scratch.
// The public contract still describes the centered, image result with zero
// padding.
class Convolve {
 public:
  static auto apply(const Image& image, image_kernel kernel)
      -> Perimortem::Utility::Result<Image&, Perimortem::Core::View::Bytes>;
};

}  // namespace Godot::Imaging::Cpu
