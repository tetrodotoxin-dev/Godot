// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once
#include "ttx/concept/capabilities/import.hpp"

namespace Godot::Demo::Adapters {

// The demo retains its configured import service through scene shutdown.
// Resources use this observation while the host keeps provider code loaded.
auto imports() -> Ttx::Concept::Capabilities::Import;

}  // namespace Godot::Demo::Adapters
