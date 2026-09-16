// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

namespace Godot::Imaging::Contracts {

// Both implementations must accept the same coefficient domain before choosing
// how to evaluate it. These checks belong to the convolution contract, allowing
// a provider to consume the borrowed values without reconstructing the host's
// Kernel node. The caller keeps that storage alive until the call returns.
class Kernel {
 public:
  struct Coefficients {
    U32 width;
    U32 height;
    Perimortem::Core::View::Vector<R32> values;
  };

  static auto validate(
      U32 width,
      U32 height,
      Perimortem::Core::View::Vector<R32> values)
      -> Perimortem::Core::View::Bytes;
};

}  // namespace Godot::Imaging::Contracts
