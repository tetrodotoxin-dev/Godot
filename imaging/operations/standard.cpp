// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "imaging/operations/standard.hpp"

#include "imaging/contracts/composite.hpp"
#include "imaging/contracts/convolve.hpp"
#include "imaging/contracts/invert.hpp"

using namespace Godot;
using namespace Perimortem;

const Imaging::Graph::Operation Imaging::Operations::Standard::invert =
    Imaging::Graph::Operation::unary<Imaging::Contracts::Invert>("invert"_view);
const Imaging::Graph::Operation Imaging::Operations::Standard::convolve =
    Imaging::Graph::Operation::convolution<Imaging::Contracts::Convolve>(
        "convolve"_view);
const Imaging::Graph::Operation Imaging::Operations::Standard::composite =
    Imaging::Graph::Operation::binary<Imaging::Contracts::Composite>(
        "composite"_view);

auto Imaging::Operations::Standard::get_vocabulary()
    -> const Imaging::Graph::Vocabulary& {
  static const Imaging::Graph::Operation* entries[] = {
    &invert, &convolve, &composite};
  static const Imaging::Graph::Vocabulary vocabulary(entries);
  return vocabulary;
}
