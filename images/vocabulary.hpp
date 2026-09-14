// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "images/operation.hpp"
#include "ttx/concept/abstract.hpp"

namespace Godot::Images {

// Godot registers methods from the same publications that graph consumers
// discover by name. Vocabulary lends that inventory as one navigable object.
// Both the pointer array and its Operations belong to the supplying owner and
// must outlive this view and any Calls that retain an operation.
//
// The standard catalogue uses a static array owned by the module. It therefore
// needs no allocation tied to the current thread whose release could run after
// thread teardown. A dynamic catalogue can lend an array from its own explicit
// lifetime owner.
class Vocabulary : public Ttx::Concept::Abstract {
 public:
  constexpr explicit Vocabulary(
      Perimortem::Core::View::Vector<const Operation*> operations)
      : operations(operations) {}

  auto find(Perimortem::Core::View::Bytes name) const -> const Operation*;
  auto resolve_concept(Perimortem::Core::View::Bytes name) const
      -> const Abstract& override;
  void visit_concepts(Visitor visitor) const override;

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Images"_view;
  }

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return Ttx::Concept::Documentation::get_empty();
  }

  auto get_operations() const { return operations; }

 private:
  Perimortem::Core::View::Vector<const Operation*> operations;
};

}  // namespace Godot::Images
