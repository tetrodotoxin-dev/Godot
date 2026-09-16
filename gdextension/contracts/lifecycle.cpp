// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "gdextension/contracts/lifecycle.hpp"

auto godot_lifecycle_representation() -> const ttx_representation* {
  return &Ttx::Semantic::Negotiation::Binding::representation<
      ::Gdextension::Contracts::Lifecycle>();
}
