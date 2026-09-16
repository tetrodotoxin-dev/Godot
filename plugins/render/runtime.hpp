// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "plugins/render/module.hpp"
#include "ttx/data/form/representation.hpp"

namespace Godot::Plugins::Render {

// Runtime emits the independent owner used after class discovery. Instance
// creation owns a backend and its current image, while the image graph can
// request an independent backend through the factory's Render provider role.
class Runtime {
 public:
  static auto emit(Module::OpenImages open, ttx_publication* output)
      -> ttx_data_status;
  static auto input(U32 method) -> const Ttx::Data::Form::Representation&;
  static auto output(U32 method) -> const Ttx::Data::Form::Representation&;
};

}  // namespace Godot::Plugins::Render
