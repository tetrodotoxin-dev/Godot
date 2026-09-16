// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "imaging/contracts/provider.h"
#include "ttx/semantic/ownership/publication.h"

namespace Godot::Plugins::Cpu {

// Backend construction stays independent of the Render discovery policy.
// The module supplies these operations without exposing private image storage
// or requiring a native Godot registration implementation.
class Provider {
 public:
  static auto images(image_provider* output) -> image_error;
  static auto samples(ttx_publication* output) -> ttx_data_status;
};

}  // namespace Godot::Plugins::Cpu
