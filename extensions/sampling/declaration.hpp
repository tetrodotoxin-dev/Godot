// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "extensions/sampling/method.hpp"
#include "extensions/sampling/runtime.hpp"
#include "ttx/concept/declarations/extensible.hpp"

namespace Godot::Extensions::Sampling {

// This class declaration describes the sampling object's public methods. It
// emits a runtime owner by copying the host capability into a fresh factory,
// allowing the declaration and its Method children to be destroyed immediately.
class Declaration {
 public:
  explicit Declaration(Ttx::Semantic::Negotiation::Query host);
  auto get_data() const -> Perimortem::Core::View::Bytes;

  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto bind_interface(
      Perimortem::System::Uuid requested,
      Ttx::Data::Form::Storage target) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto resolve_concept(Perimortem::Core::View::Bytes name) const
      -> Ttx::Concept::Abstract;
  auto visit_concepts(Ttx::Concept::Abstract::Visitor visitor) const -> void;

 private:
  Ttx::Semantic::Negotiation::Query host;
  Method methods[3];
};

}  // namespace Godot::Extensions::Sampling
