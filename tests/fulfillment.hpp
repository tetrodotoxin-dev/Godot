// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "images/vocabulary.hpp"

namespace Godot::Tests {

// These checks cross a separately loaded provider before calling the production
// image consumer, so both publication and invocation must preserve the
// contract.
class Fulfillment {
 public:
  static void check(
      Perimortem::Core::View::Bytes module,
      const Images::Vocabulary& vocabulary);
};

}  // namespace Godot::Tests
