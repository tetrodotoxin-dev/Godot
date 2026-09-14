// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "images/provider.hpp"

namespace Godot::Tests {

// The native publishers reuse a form without retaining the image that first
// compiled it. Exercise that storage lifetime through the public image surface
// so both CPU and CUDA must preserve it across real derived results.
class Forms {
 public:
  static void check(Images::Provider& provider, Bool device);
};

}  // namespace Godot::Tests
