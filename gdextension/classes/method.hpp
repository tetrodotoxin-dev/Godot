// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "gdextension/values/frame.hpp"

namespace Gdextension::Classes {

// Method is the emitted Godot adapter for a callable declaration. Preparation
// retains field conversions and copies the wire forms. Any ordered combination
// of supported field meanings uses the same invocation ABI, so adding another
// function signature does not enlarge TTX or require another native trampoline.
class Method {
 public:
  static auto compile(
      Perimortem::Core::View::Bytes name,
      Ttx::Concept::Declarations::Callable::Description description,
      U32 index) -> Perimortem::Utility::Result<Method, Ttx::Data::Status>;
  auto publish(const godot::StringName& class_name) const -> void;
  auto get_contract() const -> Perimortem::System::Uuid { return contract; }
  auto get_name() const -> const godot::StringName& { return name; }
  auto get_index() const -> U32 { return index; }
  auto get_inputs() const -> const Gdextension::Values::Frame& {
    return inputs;
  }
  auto get_outputs() const -> const Gdextension::Values::Frame& {
    return outputs;
  }

 private:
  Method(
      Perimortem::Core::View::Bytes name,
      Perimortem::System::Uuid contract,
      U32 index,
      Gdextension::Values::Frame inputs,
      Gdextension::Values::Frame outputs);
  godot::StringName name;
  Perimortem::System::Uuid contract;
  U32 index;
  Gdextension::Values::Frame inputs;
  Gdextension::Values::Frame outputs;
};

}  // namespace Gdextension::Classes
