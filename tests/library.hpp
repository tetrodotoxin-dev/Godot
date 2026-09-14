// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "images/vocabulary.hpp"

namespace Godot::Tests {

// The loader's important result is a usable lifetime, not merely a symbol.
// This check keeps an image and its borrowed binding after releasing the
// original factory reference, then observes the actual module unload.
class Library {
 public:
  static void check(
      Perimortem::Core::View::Bytes module,
      const Images::Vocabulary& vocabulary);
};

}  // namespace Godot::Tests
