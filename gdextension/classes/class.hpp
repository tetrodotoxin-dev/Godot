// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "gdextension/classes/method.hpp"
#include "gdextension/contracts/class.hpp"
#include "ttx/concept/modules/module.hpp"
#include "ttx/semantic/ownership/factory.hpp"
#include "ttx/semantic/ownership/publication.hpp"

namespace Gdextension::Classes {

// Class is the emitted terminal. Its metadata and factory have independent
// lifetimes from the source graph. The module is declared first so its code is
// released after the factory finalizer and every other owned runtime resource.
// Godot retains this owner's address while its registration exists.
class Class {
 public:
  Class(
      Ttx::Concept::Modules::Module module,
      Ttx::Semantic::Ownership::Publication factory,
      Ttx::Semantic::Ownership::Factory constructor,
      godot::String name,
      godot::String base,
      Perimortem::Memory::Dynamic::Vector<Method> methods);
  ~Class();
  auto get_query() const -> ttx_semantic_query;
  auto get_methods() const -> Perimortem::Core::View::Vector<Method> {
    return methods.get_view();
  }

  auto release_instance() -> void { --instances; }
  auto is_node() const -> bool { return node; }

 private:
  // Godot keeps this owner's userdata. These callbacks create and destroy the
  // private instance relationship, so keeping them on the owner makes their
  // lifetime accounting visible without exposing transaction state.
  static auto create(void* source, GDExtensionBool notify)
      -> GDExtensionObjectPtr;
  static auto destroy(void* source, GDExtensionClassInstancePtr instance)
      -> void;
  auto publish() -> Ttx::Data::Status;
  Ttx::Concept::Modules::Module module;
  Ttx::Semantic::Ownership::Publication factory;
  Ttx::Semantic::Ownership::Factory constructor;
  godot::StringName name;
  godot::StringName base;
  Perimortem::Memory::Dynamic::Vector<Method> methods;
  U64 instances = 0;
  bool published = false;
  bool node = false;
};

}  // namespace Gdextension::Classes
