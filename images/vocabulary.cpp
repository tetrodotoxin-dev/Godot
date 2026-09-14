// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "images/vocabulary.hpp"

using namespace Godot;
using namespace Perimortem;

auto Images::Vocabulary::find(Core::View::Bytes name) const
    -> const Operation* {
  for (const auto* operation : operations) {
    if (operation->get_name() == name) {
      return operation;
    }
  }

  return nullptr;
}

auto Images::Vocabulary::resolve_concept(Core::View::Bytes name) const
    -> const Abstract& {
  if (const auto* found = find(name)) {
    return *found;
  }

  return Abstract::resolve_concept(name);
}

void Images::Vocabulary::visit_concepts(Visitor visitor) const {
  for (const auto* operation : operations) {
    visitor(operation->get_name(), *operation);
  }
}
