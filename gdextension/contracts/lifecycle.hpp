// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/system/uuid.hpp"

#include "gdextension/contracts/lifecycle.h"
#include "ttx/semantic/negotiation/binding.hpp"

TTX_DATA_RECORD(
    godot_lifecycle_operations,
    TTX_DATA_MEMBER(godot_lifecycle_operations, entered),
    TTX_DATA_MEMBER(godot_lifecycle_operations, ready));

TTX_DATA_RECORD(
    godot_lifecycle,
    TTX_DATA_MEMBER(godot_lifecycle, source),
    TTX_DATA_MEMBER(godot_lifecycle, operations));

namespace Gdextension::Contracts {

// Lifecycle is an optional scene observation policy. Its presence does not
// make the underlying runtime instance a Godot object.
class Lifecycle {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    GODOT_LIFECYCLE_ID_HIGH,
    GODOT_LIFECYCLE_ID_LOW,
  };
  using Api = godot_lifecycle;
  using Operations = godot_lifecycle_operations;
  explicit constexpr Lifecycle(Api api) : api(api) {}
  auto entered() const -> void { api.operations->entered(api.source); }
  auto ready() const -> void { api.operations->ready(api.source); }

 private:
  Api api;
};

}  // namespace Gdextension::Contracts
