// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "extension/classes/class.hpp"
#include "ttx/concept/capabilities/export.hpp"

namespace Godot::Extension::Classes {

// GDClass supplies Export under a configured Godot name and native base. It
// copies the supported method descriptions and retains their construction
// capability before registration. The resulting Class answers its own lifetime
// questions and no longer consults the offered graph.
class GDClass {
 public:
  GDClass(
      Perimortem::Memory::Dynamic::Vector<Class*>& registrations,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Bytes base)
      : registrations(registrations), name(name), base(base) {}
  auto get_data() const -> Perimortem::Core::View::Bytes { return name; }
  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage target) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto expose(Ttx::Concept::Abstract subject) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto get_error() const -> Perimortem::Core::View::Bytes { return error; }

 private:
  Perimortem::Memory::Dynamic::Vector<Class*>& registrations;
  Perimortem::Core::View::Bytes name;
  Perimortem::Core::View::Bytes base;
  mutable Perimortem::Core::View::Bytes error;
};

}  // namespace Godot::Extension::Classes
