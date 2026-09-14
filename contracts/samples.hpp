// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "contracts/operation.hpp"
#include "contracts/samples.h"
#include "ttx/semantic/bound.hpp"
#include "ttx/semantic/thunk.hpp"

namespace Godot::Contracts {

// The interval identifies work rather than backing storage. A CPU loop and a
// device reduction can answer it without transferring a sample array, and the
// returned count needs no lifetime beyond the caller's ordinary integer value.
class Samples {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    GODOT_SAMPLES_ID_HIGH,
    GODOT_SAMPLES_ID_LOW,
  };
  static constexpr auto convention =
      Ttx::Semantic::Thunk::Convention::SystemVAMD64;
  using Operations = sample_operations;
  static auto get_representation() -> const Ttx::Data::Form::Representation& {
    return Operation::get_representation();
  }

  // This is a borrowed callable, not an owner of provider state. Fulfillment
  // establishes the table once. Repeated counts only invoke that agreed entry.
  class Handle : public Ttx::Semantic::Bound<Operations> {
   public:
    using Bound::Bound;
    auto count(U32 seed, U32 first, U32 size) const
        -> Perimortem::Utility::Result<U64, Ttx::Data::Status> {
      U64 output;
      const auto status = Ttx::Data::Status(
          operations.count(source, seed, first, size, &output));
      if (status != Ttx::Data::Status::Success) {
        return status;
      }

      return output;
    }
  };
};

}  // namespace Godot::Contracts
