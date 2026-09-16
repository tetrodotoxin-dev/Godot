// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "imaging/graph/vocabulary.hpp"

using namespace Godot;
using namespace Perimortem;

auto Imaging::Graph::Vocabulary::find(Core::View::Bytes name) const
    -> const Operation* {
  for (const auto* operation : operations) {
    if (operation->get_data() == name) {
      return operation;
    }
  }

  return nullptr;
}

auto Imaging::Graph::Vocabulary::resolve_concept(Core::View::Bytes name) const
    -> Ttx::Concept::Abstract {
  if (const auto* found = find(name)) {
    return Ttx::Concept::Abstract::provide(*found);
  }

  return Ttx::Concept::Abstract(ttx_none());
}

void Imaging::Graph::Vocabulary::visit_concepts(
    Ttx::Concept::Abstract::Visitor visitor) const {
  for (const auto* operation : operations) {
    visitor(operation->get_data(), Ttx::Concept::Abstract::provide(*operation));
  }
}
