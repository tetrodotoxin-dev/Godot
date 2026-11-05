// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "godot/classes/method.hpp"

namespace Godot::Extension::Classes {

// Class publishes a Godot type from discovery metadata kept accessible by its
// loaded module. A represented value receives its record from a successful
// Invocation. Its member methods receive that record as their explicit first
// argument.
// Godot keeps registration metadata fixed and checks each new Invocation's
// forms. GDScript controls the value's provider access and release obligations.
class Class {
 public:
  Class(
      Ttx::Graph::Abstract factory,
      godot::String name,
      Perimortem::Memory::Global::Vector<Method> methods)
      : abstract(factory),
        name(name),
        methods(Perimortem::Core::Data::take(methods)) {}
  Class(Ttx::Graph::Abstract abstract, godot::String name)
      : abstract(abstract), name(name) {}
  Class(const Class&) = delete;
  auto operator=(const Class&) -> Class& = delete;
  ~Class();
  auto publish() -> bool;
  auto get_name() const -> const godot::StringName& { return name; }
  auto release_instance() -> void { --instances; }
  auto set_value_type(Ttx::Data::Form::Representation form) -> void {
    value_form = Perimortem::Core::Data::take(form);
    represented = true;
  }

  auto set_methods(Perimortem::Memory::Global::Vector<Method> values) -> void {
    methods = Perimortem::Core::Data::take(values);
  }

  auto refuse() -> void { complete = false; }
  auto is_complete() const -> bool { return complete; }
  auto is_represented() const -> bool { return represented; }
  auto get_representation() const -> const Ttx::Data::Form::Representation& {
    return value_form;
  }

  auto get_abstract() const -> Ttx::Graph::Abstract { return abstract; }

  auto materialize(const U8* bytes) -> GDExtensionObjectPtr;
  auto get_materializer() -> Values::Field::Materializer;
  auto depends_on(const Class& other) const -> bool;

 private:
  static auto create(void* context, GDExtensionBool notify)
      -> GDExtensionObjectPtr;
  static auto destroy(void* context, GDExtensionClassInstancePtr instance)
      -> void;
  Ttx::Graph::Abstract abstract;
  godot::StringName name;
  Perimortem::Memory::Global::Vector<Method> methods;
  Count instances = 0;
  bool published = false;
  bool complete = true;
  bool represented = false;
  Ttx::Data::Form::Representation value_form;
};

}  // namespace Godot::Extension::Classes
