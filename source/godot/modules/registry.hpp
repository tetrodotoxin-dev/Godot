// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "godot/classes/class.hpp"
#include "godot/modules/imports.hpp"

namespace Godot::Extension::Modules {

// Registry discovers factories through module graphs and publishes the subset
// whose metadata and call forms Godot can represent. Discovery performs no
// application construction. Unknown contracts and incomplete declarations
// remain available in the graph without becoming Godot registrations.
//
// This consumer profile selects a named Coherence Type whose static Callables
// become factory methods. A represented result Type supplies Member Callables
// whose receiver is compiled as the hidden first input. Argument visitation
// gives positional order, each declaration's "type" route selects its value
// Type, and Describe supplies Godot labels. A Type whose Selection cannot
// establish the requested scope remains unpublished. Duplicate class labels
// are refused along with callers that depend on them. Until Coherence supplies
// canonical Type locations, Registry accepts only Constant locations whose
// Type and Constant reentry operations return one stable context address.
//
// GDScript keeps each returned value's referenced state available and performs
// the release operations required by its typed contract. Imports keeps loaded
// modules and their discovery metadata available through Registry destruction.
class Registry {
 public:
  Registry() = default;
  Registry(const Registry&) = delete;
  auto operator=(const Registry&) -> Registry& = delete;
  ~Registry();
  auto load(const godot::Array& paths) -> void;
  auto materialized_type(Ttx::Graph::Abstract abstract)
      -> Godot::Extension::Classes::Class*;
  auto resolver() -> Values::Field::Resolver;

 private:
  auto visit(Ttx::Graph::Abstract abstract, Count depth, Count& budget) -> void;
  auto consider(Ttx::Graph::Abstract abstract) -> bool;
  Imports imports;
  Count type_budget = 128;
  Perimortem::Memory::Global::Vector<Godot::Extension::Classes::Class*>
      registrations;
};

}  // namespace Godot::Extension::Modules
