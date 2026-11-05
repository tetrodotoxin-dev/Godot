// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "godot/classes/instance.hpp"

#include "perimortem/core/view/null_terminated.hpp"

#include "godot/classes/class.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Data;
using namespace Ttx::Graph;
using namespace Ttx::Semantic;
using namespace Godot::Extension;

Classes::Instance::~Instance() {
  type.release_instance();
}

auto Classes::Instance::invoke(
    const Method& method,
    const void* input,
    void* output) -> Status {
  return method.compile().visit(
      [&](Coherence::Compilation::Executable& executable) {
        if (!executable.accepts(
                method.get_inputs().get_representation(),
                method.get_outputs().get_representation())) {
          return Status::Incompatible;
        }

        return executable.invoke(input, output);
      },

      [](Status) { return Status::Unsupported; });
}
