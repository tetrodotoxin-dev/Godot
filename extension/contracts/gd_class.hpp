// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "extension/contracts/gd_class.h"
#include "ttx/semantic/ownership/publication.hpp"

namespace Godot::Extension::Contracts {

// GDClass lets a provider offer Godot class emission without exposing its
// compiler state. The returned publication owns the prepared class, allowing
// this borrowed policy and its source graph to disappear after emission.
class GDClass {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    GODOT_GDCLASS_ID_HIGH,
    GODOT_GDCLASS_ID_LOW,
  };
  using Api = godot_gdclass;
  explicit constexpr GDClass(Api api) : api(api) {}

  auto emit() const -> Perimortem::Utility::
      Result<Ttx::Semantic::Ownership::Publication, Ttx::Data::Status> {
    ttx_publication output = {};
    const auto status = api.operations->emit(api.source, &output);
    if (status != TTX_DATA_SUCCESS) {
      return static_cast<Ttx::Data::Status>(status);
    }

    return Ttx::Semantic::Ownership::Publication(output);
  }

 private:
  Api api;
};

}  // namespace Godot::Extension::Contracts

TTX_DATA_RECORD(
    godot_gdclass_operations,
    TTX_DATA_MEMBER(godot_gdclass_operations, emit));
TTX_DATA_RECORD(
    godot_gdclass,
    TTX_DATA_MEMBER(godot_gdclass, source),
    TTX_DATA_MEMBER(godot_gdclass, operations));
