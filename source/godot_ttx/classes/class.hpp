// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "godot_ttx/classes/method.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/capabilities/create.hpp"
#include "ttx/concept/policies/borrowed.hpp"

namespace Godot::Extension::Classes {

// Class owns a Godot registration, its copied method descriptions and the
// retained construction capability. These are the inputs used to construct
// instances after discovery ends. Destruction removes the registration and
// returns the provider state while its code remains loaded.
class Class {
 public:
  Class(const Class&) = delete;
  auto operator=(const Class&) -> Class& = delete;
  Class(
      Ttx::Concept::Policies::Borrowed factory,
      Ttx::Concept::Capabilities::Create constructor,
      godot::String name,
      godot::String base,
      Perimortem::Memory::Dynamic::Vector<Method> methods);
  ~Class();
  auto publish() -> Ttx::Data::Status;
  auto get_methods() const -> Perimortem::Core::View::Vector<Method> {
    return methods.get_view();
  }

  auto release_instance() -> void { --instances; }
  auto is_node() const -> bool { return node; }

 private:
  // Godot calls these through the registration's userdata. They pair native
  // object creation with the provider instance and update the class's live
  // instance count as that relationship begins and ends.
  static auto create(void* source, GDExtensionBool notify)
      -> GDExtensionObjectPtr;
  static auto destroy(void* source, GDExtensionClassInstancePtr instance)
      -> void;
  Ttx::Concept::Policies::Borrowed factory;
  Ttx::Concept::Capabilities::Create constructor;
  godot::StringName name;
  godot::StringName base;
  Perimortem::Memory::Dynamic::Vector<Method> methods;
  U64 instances = 0;
  bool published = false;
  bool node = false;
};

}  // namespace Godot::Extension::Classes
