// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "images/vocabulary.hpp"

namespace Godot::Operations {

// The lab exposes a small set of independently optional image operations.
// Standard owns their host publications and lends them to the vocabulary used
// for Godot registration. Each publication captures its actual typed contract,
// so a method's argument shape cannot substitute for its semantic identity.
class Standard {
 public:
  static const Images::Operation invert;
  static const Images::Operation convolve;
  static const Images::Operation composite;

  static auto get_vocabulary() -> const Images::Vocabulary&;
};

}  // namespace Godot::Operations
