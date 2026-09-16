// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "sampling/contracts/samples.h"
#include "ttx/semantic/negotiation/query.hpp"

TTX_DATA_RECORD(sample_operations, TTX_DATA_MEMBER(sample_operations, count));

TTX_DATA_RECORD(
    sample_api,
    TTX_DATA_MEMBER(sample_api, source),
    TTX_DATA_MEMBER(sample_api, operations));

namespace Godot::Sampling::Contracts {

// The interval identifies work rather than backing storage. A CPU loop and a
// device reduction can answer it without transferring a sample array, and the
// returned count needs no lifetime beyond the caller's ordinary integer value.
class Samples {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    GODOT_SAMPLES_ID_HIGH,
    GODOT_SAMPLES_ID_LOW,
  };
  using Api = sample_api;
  using Operations = sample_operations;
  static auto get_representation() -> const Ttx::Data::Form::Representation& {
    return Ttx::Semantic::Negotiation::Binding::representation<Samples>();
  }

  // This is a borrowed callable, not an owner of provider state. Fulfillment
  // establishes the table once. Repeated counts only invoke that agreed entry.
  explicit constexpr Samples(Api api) : api(api) {}
  auto count(U32 seed, U32 first, U32 size) const
      -> Perimortem::Utility::Result<U64, Ttx::Data::Status> {
    U64 output;
    const auto status = Ttx::Data::Status(
        api.operations->count(api.source, seed, first, size, &output));
    if (status != Ttx::Data::Status::Success) {
      return status;
    }

    return output;
  }

 private:
  Api api;
};

}  // namespace Godot::Sampling::Contracts
