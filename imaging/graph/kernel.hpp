// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "imaging/contracts/image.h"
#include "ttx/concept/abstract.hpp"

namespace Godot::Imaging::Graph {

// A Call needs to retain its authored coefficients as a discoverable input,
// while the provider only needs a borrowed array for one convolution. Kernel
// bridges those lifetimes without imposing the host graph on the backend.
// The Call's Arena owns persistent coefficients. An immediate caller may lend
// its own vector. The C record passed to the provider borrows that same
// storage.
class Kernel {
 public:
  constexpr Kernel(
      U32 width,
      U32 height,
      Perimortem::Core::View::Vector<R32> values)
      : value(width, height, values.get_data(), values.get_size()) {}

  ~Kernel();

  auto get_abi() const -> image_kernel { return value; }

  constexpr auto get_data() const -> Perimortem::Core::View::Bytes {
    return "Kernel"_view;
  }

 private:
  image_kernel value;
};

}  // namespace Godot::Imaging::Graph
