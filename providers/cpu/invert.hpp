// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "providers/cpu/image.hpp"

namespace Godot::Providers::Cpu {

// Inversion only needs the source bytes. Copying before complementing RGB
// preserves the original publication, including alpha, for other graph users.
class Invert {
 public:
  static auto apply(const Image& image)
      -> Perimortem::Utility::Result<Image&, Perimortem::Core::View::Bytes>;
};

}  // namespace Godot::Providers::Cpu
