// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "demo/imaging/cpu/image.hpp"

namespace Godot::Demo::Imaging::Cpu {

// FFT convolution replaces a large spatial neighborhood with transforms and
// a pointwise product. The CPU provider owns its transform scratch and uses
// PocketFFT for real input. Callers observe a centered convolution with zero
// padding and the original alpha, independently of that algorithm choice.
class Convolve {
 public:
  static auto apply(const Image& image, image_kernel kernel)
      -> Perimortem::Utility::Result<Image&, Perimortem::Core::View::Bytes>;
};

}  // namespace Godot::Demo::Imaging::Cpu
