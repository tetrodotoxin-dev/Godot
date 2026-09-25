// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/declarations/callable.hpp"

namespace Godot::Demo::Sampling::Class {

// Method is a declaration node in the sampling plugin's source graph. The
// runtime class factory supplies the actual operation later, so this node may
// be discarded once a terminal has checked and copied its signature.
class Method {
 public:
  Method(
      Perimortem::Core::View::Bytes name,
      ttx_callable_description description)
      : name(name), description(description) {}
  auto get_data() const -> Perimortem::Core::View::Bytes { return name; }

  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    using Ttx::Semantic::Negotiation::Binding::Status;
    return id == Ttx::Concept::Declarations::Callable::contract_id
               ? Status::Satisfied
               : Status::Unsupported;
  }

  auto bind_interface(
      Perimortem::System::Uuid requested,
      Ttx::Data::Form::Storage target) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    if (requested == Ttx::Concept::Declarations::Callable::contract_id) {
      static const ttx_callable_operations operations = {
        [](const void* source,
           ttx_callable_description* output) -> ttx_binding_status {
          *output = static_cast<const Method*>(source)->description;
          return TTX_BINDING_SATISFIED;
        },
      };
      return Ttx::Semantic::Negotiation::Binding::provide<
          Ttx::Concept::Declarations::Callable>(
          ttx_callable(this, &operations), target);
    }

    return Ttx::Semantic::Negotiation::Binding::Status::Unsupported;
  }

 private:
  Perimortem::Core::View::Bytes name;
  ttx_callable_description description;
};

}  // namespace Godot::Demo::Sampling::Class
