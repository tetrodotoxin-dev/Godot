// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "perimortem/memory/global/bytes.hpp"

#include "godot/values/frame.hpp"

namespace Godot::Extension::Classes {

// Method keeps a copied discovery route and the conversions advertised to
// Godot. Runtime calls resolve that route on the retained instance and check
// the newly supplied Invocation against these stored forms. Providers may
// refuse a later call without changing Godot's registered method signature.
class Method {
 public:
  static auto compile(
      Perimortem::Core::View::Bytes route,
      Ttx::Graph::Abstract declaration)
      -> Perimortem::Core::Storage::Option<Method>;
  auto publish(const godot::StringName& class_name) -> void;
  auto get_name() const -> const godot::StringName& { return name; }
  auto get_route() const -> Perimortem::Core::View::Bytes {
    return route.get_view();
  }

  auto get_inputs() const -> const Godot::Extension::Values::Frame& {
    return inputs;
  }

  auto get_outputs() const -> const Godot::Extension::Values::Frame& {
    return outputs;
  }

 private:
  Method(
      Perimortem::Core::View::Bytes route,
      godot::String name,
      Godot::Extension::Values::Frame inputs,
      Godot::Extension::Values::Frame outputs)
      : route(route),
        name(name),
        inputs(Perimortem::Core::Data::take(inputs)),
        outputs(Perimortem::Core::Data::take(outputs)) {}
  Perimortem::Memory::Global::Bytes route;
  godot::StringName name;
  Godot::Extension::Values::Frame inputs;
  Godot::Extension::Values::Frame outputs;
};

}  // namespace Godot::Extension::Classes
