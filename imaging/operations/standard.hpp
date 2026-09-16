// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "imaging/graph/vocabulary.hpp"

namespace Godot::Imaging::Operations {

// The lab exposes a small set of independently optional image operations.
// Standard owns their host publications and lends them to the vocabulary used
// for Godot registration. Each publication captures its actual typed contract,
// so a method's argument shape cannot substitute for its semantic identity.
class Standard {
 public:
  static const Imaging::Graph::Operation invert;
  static const Imaging::Graph::Operation convolve;
  static const Imaging::Graph::Operation composite;

  static auto get_vocabulary() -> const Imaging::Graph::Vocabulary&;
};

}  // namespace Godot::Imaging::Operations
