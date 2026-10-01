// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/system/uuid.hpp"

#include "godot_ttx/contracts/lifecycle.h"
#include "ttx/concept/abstract.hpp"

TTX_DATA_RECORD(
    godot_lifecycle_operations,
    TTX_DATA_MEMBER(godot_lifecycle_operations, abstract),
    TTX_DATA_MEMBER(godot_lifecycle_operations, entered),
    TTX_DATA_MEMBER(godot_lifecycle_operations, ready));

TTX_DATA_RECORD(
    godot_lifecycle,
    TTX_DATA_MEMBER(godot_lifecycle, source),
    TTX_DATA_MEMBER(godot_lifecycle, operations));

namespace Godot::Extension::Contracts {

// Receives scene entry and readiness through the instance's governing policy.
// The C++ view keeps the full Abstract interface available for subsequent
// queries. Its callbacks borrow the same access that supplied this view.
class Lifecycle : public Ttx::Concept::Abstract {
 public:
  static constexpr auto contract_id =
      Perimortem::System::Uuid(GODOT_LIFECYCLE_ID_HIGH, GODOT_LIFECYCLE_ID_LOW);
  using Api = godot_lifecycle;
  using Operations = godot_lifecycle_operations;
  static auto accept(Api value) -> Bool {
    return value.operations && value.operations->entered &&
           value.operations->ready &&
           Abstract::accept(
               ttx_abstract(value.source, &value.operations->abstract));
  }
  explicit constexpr Lifecycle(Api value)
      : Abstract(value.source, value.operations->abstract) {}
  constexpr auto get_abi() const -> Api {
    const auto value = Abstract::get_abi();
    return Api(
        value.source, reinterpret_cast<const Operations*>(value.operations));
  }
  auto entered() const -> void {
    const auto value = get_abi();
    value.operations->entered(value.source);
  }
  auto ready() const -> void {
    const auto value = get_abi();
    value.operations->ready(value.source);
  }
  template <typename Owner>
    requires(!__is_base_of(Abstract, Owner))
  static auto provide(const Owner& owner) -> Lifecycle {
    static const Operations operations = Operations(
        *Abstract::provide(owner).get_abi().operations,
        [](const void* source) {
          static_cast<const Owner*>(source)->entered();
        },
        [](const void* source) { static_cast<const Owner*>(source)->ready(); });
    return Lifecycle(Api(&owner, &operations));
  }
};

}  // namespace Godot::Extension::Contracts
