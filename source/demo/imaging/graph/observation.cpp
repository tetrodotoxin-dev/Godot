// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/imaging/graph/observation.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Godot::Demo;
using namespace Perimortem;

static thread_local U64 current_pull = 0;
static thread_local Count depth = 0;

Imaging::Graph::Observation::Observation() {
  if (depth++ == 0 && ++current_pull == 0) {
    Core::Diagnostics::Log::fatal("Image traversal revision exhausted."_view);
  }

  pull = current_pull;
}

Imaging::Graph::Observation::~Observation() {
  --depth;
}

auto Imaging::Graph::Observation::active() -> Bool {
  return depth != 0;
}
