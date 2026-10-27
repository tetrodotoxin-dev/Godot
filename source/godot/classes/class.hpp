// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "ttx/graph/capabilities/create.hpp"
#include "ttx/graph/policies/retained.hpp"

#include "godot/classes/method.hpp"

namespace Godot::Extension::Classes {

// Class publishes a RefCounted Godot type from a retained TTX factory. Its
// metadata stays fixed for Godot registration. Each construction requests
// Create again and retains the supplied result before its callback ends.
// Provider refusal remains a failed construction or invocation at the host.
class Class {
 public:
  Class(
      Ttx::Graph::Policies::Retained factory,
      godot::String name,
      Perimortem::Memory::Global::Vector<Method> methods)
      : factory(factory),
        name(name),
        methods(Perimortem::Core::Data::take(methods)) {}
  Class(const Class&) = delete;
  auto operator=(const Class&) -> Class& = delete;
  ~Class();
  auto publish() -> bool;
  auto get_name() const -> const godot::StringName& { return name; }
  auto release_instance() -> void { --instances; }

 private:
  static auto create(void* context, GDExtensionBool notify)
      -> GDExtensionObjectPtr;
  static auto destroy(void* context, GDExtensionClassInstancePtr instance)
      -> void;
  Ttx::Graph::Policies::Retained factory;
  godot::StringName name;
  Perimortem::Memory::Global::Vector<Method> methods;
  Count instances = 0;
  bool published = false;
};

}  // namespace Godot::Extension::Classes
