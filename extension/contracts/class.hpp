// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "extension/contracts/class.h"
#include "ttx/semantic/ownership/publication.hpp"

namespace Godot::Extension::Contracts {

// Class is the negotiated publication capability of an emitted Godot class.
// The implementation owns its factory and registration data. Retaining this
// record borrows that publication without retaining a discovery graph.
class Class {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    GODOT_CLASS_ID_HIGH,
    GODOT_CLASS_ID_LOW,
  };
  using Api = godot_class;
  explicit constexpr Class(Api api) : api(api) {}

  auto publish() const -> Ttx::Data::Status {
    return static_cast<Ttx::Data::Status>(api.operations->publish(api.source));
  }

 private:
  Api api;
};

}  // namespace Godot::Extension::Contracts

TTX_DATA_RECORD(
    godot_class_operations,
    TTX_DATA_MEMBER(godot_class_operations, publish));
TTX_DATA_RECORD(
    godot_class,
    TTX_DATA_MEMBER(godot_class, source),
    TTX_DATA_MEMBER(godot_class, operations));
