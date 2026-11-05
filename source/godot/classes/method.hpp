// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "godot/values/frame.hpp"

namespace Godot::Extension::Classes {

// Method keeps one discovered Callable and the conversions advertised to
// Godot. Runtime calls compile that Callable again and compare its resulting
// Invocation with the forms fixed during registration. Providers may refuse a
// later call without changing Godot's registered method signature.
class Method {
 public:
  static auto compile(
      Ttx::Graph::Abstract declaration,
      Values::Field::Resolver resolver,
      const Ttx::Graph::Abstract* receiver = nullptr)
      -> Perimortem::Core::Storage::Option<Method>;
  auto publish(const godot::StringName& class_name) -> void;
  auto get_name() const -> const godot::StringName& { return name; }
  auto has_receiver() const -> bool { return bool(receiver); }

  auto compile() const -> Coherence::Compilation::Compiler::Result<
      Coherence::Compilation::Executable>;

  auto get_inputs() const -> const Godot::Extension::Values::Frame& {
    return inputs;
  }

  auto get_outputs() const -> const Godot::Extension::Values::Frame& {
    return outputs;
  }

 private:
  Method(
      godot::String name,
      Ttx::Graph::Abstract callable,
      Perimortem::Core::Storage::Option<Ttx::Graph::Abstract> receiver,
      Godot::Extension::Values::Frame inputs,
      Godot::Extension::Values::Frame outputs)
      : name(name),
        callable(callable),
        receiver(receiver),
        inputs(Perimortem::Core::Data::take(inputs)),
        outputs(Perimortem::Core::Data::take(outputs)) {}
  godot::StringName name;
  Ttx::Graph::Abstract callable;
  Perimortem::Core::Storage::Option<Ttx::Graph::Abstract> receiver;
  Godot::Extension::Values::Frame inputs;
  Godot::Extension::Values::Frame outputs;
};

}  // namespace Godot::Extension::Classes
