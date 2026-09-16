// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "imaging/contracts/render.h"
#include "ttx/concept/modules/module.h"
#include "ttx/semantic/negotiation/query.hpp"

namespace Godot::Plugins::Render {

// The Render policy exposes an image implementation as a constructible TTX
// declaration. Its publication owns only discovery and optional sample access.
// Emitted factories retain the native creation operation independently, so
// neither Godot nor the image graph keeps this discovery allocation alive.
// CPU and CUDA supply these creation operations from separate modules.
class Module {
 public:
  using OpenImages = image_error (*)(image_provider*);
  using OpenSamples = ttx_data_status (*)(ttx_publication*);
  static auto open(
      OpenImages images,
      OpenSamples samples,
      ttx_module_acquisition* output,
      Ttx::Semantic::Negotiation::Query capabilities = {}) -> ttx_data_status;
};

}  // namespace Godot::Plugins::Render
