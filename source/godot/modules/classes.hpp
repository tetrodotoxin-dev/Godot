// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "godot/classes/class.hpp"
#include "godot/modules/imports.hpp"

namespace Godot::Extension::Modules {

// Classes discovers factories through module graphs and publishes the subset
// whose metadata and call forms Godot can represent. Discovery performs no
// application construction. Unknown contracts and incomplete declarations
// remain available in the graph without becoming Godot registrations.
class Classes {
 public:
  Classes() = default;
  Classes(const Classes&) = delete;
  auto operator=(const Classes&) -> Classes& = delete;
  ~Classes();
  auto load(const godot::Array& paths) -> void;

 private:
  auto visit(Ttx::Graph::Abstract abstract, Count depth, Count& budget) -> void;
  auto consider(Ttx::Graph::Abstract abstract) -> bool;
  Imports imports;
  Perimortem::Memory::Global::Vector<Godot::Extension::Classes::Class*>
      registrations;
};

}  // namespace Godot::Extension::Modules
