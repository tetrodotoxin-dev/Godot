// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "operations/standard.hpp"

#include "contracts/composite.hpp"
#include "contracts/convolve.hpp"
#include "contracts/invert.hpp"

using namespace Godot;
using namespace Perimortem;

const Images::Operation Operations::Standard::invert =
    Images::Operation::unary<Contracts::Invert>("invert"_view);
const Images::Operation Operations::Standard::convolve =
    Images::Operation::convolution<Contracts::Convolve>("convolve"_view);
const Images::Operation Operations::Standard::composite =
    Images::Operation::binary<Contracts::Composite>("composite"_view);

auto Operations::Standard::get_vocabulary() -> const Images::Vocabulary& {
  static const Images::Operation* entries[] = {&invert, &convolve, &composite};
  static const Images::Vocabulary vocabulary(entries);
  return vocabulary;
}
