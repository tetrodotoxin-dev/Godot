// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include "perimortem/memory/dynamic/vector.hpp"

#include "ttx/concept/capabilities/import.hpp"
#include "ttx/semantic/negotiation/library.hpp"

namespace Godot::Extension::Modules {

// Interprets a described byte view as a configured module alias. The host
// keeps this service alive through its registrations and instances, then
// closes the loaded libraries after those users have released their data.
class Imports {
 public:
  auto get_data() const -> Perimortem::Core::View::Bytes {
    return Perimortem::Core::View::Bytes();
  }
  auto get_query() const -> Ttx::Semantic::Negotiation::Query {
    return Ttx::Concept::Abstract::provide(*this).get_query();
  }
  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage target) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  template <typename Receiver>
  auto visit(
      const void* input,
      const Ttx::Data::Form::Representation& representation,
      Receiver& receiver) const -> Ttx::Semantic::Negotiation::Binding::Status {
    using namespace Ttx::Data::Form;
    if (!representation.compatible(
            Compiled<Native<perimortem_view_bytes>::reference>::
                get_representation())) {
      return Ttx::Semantic::Negotiation::Binding::Status::Rejected;
    }
    const auto& name = *static_cast<const perimortem_view_bytes*>(input);
    return load(
        {name.data, name.size}, &receiver,
        [](void* state, ttx_abstract subject) {
          (*static_cast<Receiver*>(state))(Ttx::Concept::Abstract(subject));
        });
  }

 private:
  struct Loaded {
    godot::String path;
    Ttx::Semantic::Negotiation::Library library;
  };
  auto open(Perimortem::Core::View::Bytes name) const -> Perimortem::Utility::
      Result<Ttx::Semantic::Negotiation::Library, Ttx::Data::Status>;
  auto load(
      Perimortem::Core::View::Bytes name,
      void* receiver,
      void (*receive)(void*, ttx_abstract)) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  mutable Perimortem::Memory::Dynamic::Vector<Loaded> loaded;
};

}  // namespace Godot::Extension::Modules
