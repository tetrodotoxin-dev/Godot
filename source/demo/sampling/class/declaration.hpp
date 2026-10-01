// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "demo/sampling/class/method.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/capabilities/borrow.hpp"
#include "ttx/concept/capabilities/create.hpp"

namespace Godot::Demo::Sampling::Class {

// This answer describes the sampler's methods and independently supplies
// Create and Borrow. Retention copies the declaration and its host services,
// so later questions observe the same methods after discovery has ended.
class Declaration {
 public:
  explicit Declaration(Ttx::Semantic::Negotiation::Query host);
  auto borrow() const -> Perimortem::Utility::Result<
      Ttx::Concept::Policies::Borrowed,
      Ttx::Semantic::Negotiation::Binding::Failure>;
  auto release() const -> void;
  auto create(
      Ttx::Concept::Abstract arguments,
      void* receiver,
      void (*receive)(void*, ttx_abstract)) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
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
  Count references = 0;
};

}  // namespace Godot::Demo::Sampling::Class
