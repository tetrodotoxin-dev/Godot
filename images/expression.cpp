// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "images/expression.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/object.hpp"

#include "images/observation.hpp"

using namespace Godot;
using namespace Perimortem;

void Images::Expression::retain() {
  Core::Object<>(allocation).retain();
}

void Images::Expression::release() {
  Core::Object<>(allocation).release();
}

void Images::Expression::advance() {
  if (++revision == 0) {
    Core::Diagnostics::Log::fatal("Image revision exhausted."_view);
  }
}

auto Images::Expression::evaluate(Memory::Allocator::Arena& errors)
    -> Utility::Result<const Image&, Core::View::Bytes> {
  Observation observation;
  return evaluate_value(errors, observation.get_pull());
}
