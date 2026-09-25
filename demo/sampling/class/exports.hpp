// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "demo/sampling/class/declaration.hpp"
#include "ttx/concept/abstract.hpp"

namespace Godot::Demo::Sampling::Class {

// Exports owns the declaration graph only. Its lifetime can end as soon as
// a terminal has prepared the class, while emitted Runtime owners keep the
// injected services required by future sampling instances.
class Exports {
 public:
  explicit Exports(Ttx::Semantic::Negotiation::Query host)
      : declaration(host) {}
  auto get_data() const -> Perimortem::Core::View::Bytes;

  auto resolve_concept(Perimortem::Core::View::Bytes name) const
      -> Ttx::Concept::Abstract;
  auto visit_concepts(Ttx::Concept::Abstract::Visitor visitor) const -> void;

 private:
  Declaration declaration;
};

}  // namespace Godot::Demo::Sampling::Class
