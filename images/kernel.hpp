// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "contracts/image.h"
#include "ttx/concept/abstract.hpp"

namespace Godot::Images {

// A Call needs to retain its authored coefficients as a discoverable input,
// while the provider only needs a borrowed array for one convolution. Kernel
// bridges those lifetimes without imposing the host graph on the backend.
// The Call's Arena owns persistent coefficients. An immediate caller may lend
// its own vector. The C record passed to the provider borrows that same
// storage.
class Kernel : public Ttx::Concept::Abstract {
 public:
  constexpr Kernel(
      U32 width,
      U32 height,
      Perimortem::Core::View::Vector<R32> values)
      : value(width, height, values.get_data(), values.get_size()) {}

  // Keep the virtual table in the native target. Godot constructs this node
  // in a translation unit with RTTI, but its Abstract base deliberately
  // supplies no C++ RTTI as part of the semantic contract.
  ~Kernel() override;

  auto get_abi() const -> image_kernel { return value; }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Kernel"_view;
  }

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return Ttx::Concept::Documentation::get_empty();
  }

 private:
  image_kernel value;
};

}  // namespace Godot::Images
