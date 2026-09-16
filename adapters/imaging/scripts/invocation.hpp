// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/variant.hpp>

#include "perimortem/core/access/vector.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/utility/result.hpp"

namespace Godot::Adapters::Imaging::Scripts {

// Script calls have a Variant boundary even when their enclosing TTX thunk has
// an exact native signature. Invocation confines that conversion and Godot's
// call error to this provider. The script convention returns a String only for
// failure, and the supplying image or factory owns the copied diagnostic bytes.
// Arguments remain on the caller's stack for this synchronous invocation.
class Invocation {
 public:
  template <typename... Values>
  static auto call(
      const godot::Callable& method,
      Perimortem::Memory::Dynamic::Bytes& errors,
      const Values&... values) -> Perimortem::Utility::
      Result<godot::Variant, Perimortem::Core::View::Bytes> {
    if constexpr (sizeof...(Values) == 0) {
      return invoke(method, {}, errors);
    } else {
      const godot::Variant arguments[] = {godot::Variant(values)...};
      const godot::Variant* pointers[sizeof...(Values)];
      for (Count i = 0; i < sizeof...(Values); ++i) {
        pointers[i] = &arguments[i];
      }

      return invoke(method, {pointers, sizeof...(Values)}, errors);
    }
  }

 private:
  // The template constructs typed arguments, while this one implementation
  // handles the engine boundary and diagnostic lifetime for every caller.
  static auto invoke(
      const godot::Callable& method,
      Perimortem::Core::Access::Vector<const godot::Variant*> arguments,
      Perimortem::Memory::Dynamic::Bytes& errors) -> Perimortem::Utility::
      Result<godot::Variant, Perimortem::Core::View::Bytes>;
};

}  // namespace Godot::Adapters::Imaging::Scripts
