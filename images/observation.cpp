// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "images/observation.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Godot;
using namespace Perimortem;

static thread_local U64 current_pull = 0;
static thread_local Count depth = 0;

Images::Observation::Observation() {
  if (depth++ == 0 && ++current_pull == 0) {
    Core::Diagnostics::Log::fatal("Image traversal revision exhausted."_view);
  }

  pull = current_pull;
}

Images::Observation::~Observation() {
  --depth;
}

auto Images::Observation::active() -> Bool {
  return depth != 0;
}
