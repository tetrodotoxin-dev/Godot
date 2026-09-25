// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/adapters/imaging/scripts/invocation.hpp"

#include "perimortem/core/null_terminated.hpp"

using namespace Godot::Demo;
using namespace Perimortem;

auto Adapters::Imaging::Scripts::Invocation::invoke(
    const godot::Callable& method,
    Core::Access::Vector<const godot::Variant*> arguments,
    Memory::Dynamic::Bytes& errors)
    -> Utility::Result<godot::Variant, Core::View::Bytes> {
  godot::Variant callable(method);
  godot::Variant result;
  GDExtensionCallError failure;
  callable.callp(
      "call", arguments.get_data(), arguments.get_size(), result, failure);
  if (failure.error != GDEXTENSION_CALL_OK) {
    return "GDScript callable invocation failed."_view;
  }

  if (result.get_type() == godot::Variant::STRING) {
    const godot::String message = result;
    const auto text = message.utf8();
    errors = Memory::Dynamic::Bytes(
        Core::View::Bytes(
            reinterpret_cast<const U8*>(text.get_data()), text.length()));
    if (errors.is_empty()) {
      return "GDScript returned an empty failure message."_view;
    }

    return errors.get_view();
  }

  return result;
}
