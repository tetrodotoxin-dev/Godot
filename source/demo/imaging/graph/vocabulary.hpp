// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "demo/imaging/graph/operation.hpp"
#include "ttx/concept/abstract.hpp"

namespace Godot::Demo::Imaging::Graph {

// Godot registers methods from the same publications that graph consumers
// discover by name. Vocabulary lends that inventory as one navigable object.
// Both the pointer array and its Operations belong to the supplying owner and
// must outlive this view and any Calls that retain an operation.
//
// The standard catalogue uses a static array owned by the module. It therefore
// needs no allocation tied to the current thread whose release could run after
// thread teardown. A dynamic catalogue can lend an array from its own explicit
// lifetime owner.
class Vocabulary {
 public:
  // The demo starts with these operations and can add provider offers.
  static auto standard() -> const Vocabulary&;

  constexpr explicit Vocabulary(
      Perimortem::Core::View::Vector<const Operation*> operations)
      : operations(operations) {}

  auto find(Perimortem::Core::View::Bytes name) const -> const Operation*;
  auto resolve_concept(Perimortem::Core::View::Bytes name) const
      -> Ttx::Concept::Abstract;
  void visit_concepts(Ttx::Concept::Abstract::Visitor visitor) const;

  constexpr auto get_data() const -> Perimortem::Core::View::Bytes {
    return "Images"_view;
  }

  auto get_operations() const { return operations; }

 private:
  Perimortem::Core::View::Vector<const Operation*> operations;
};

}  // namespace Godot::Demo::Imaging::Graph
