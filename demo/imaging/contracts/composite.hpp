// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "demo/imaging/contracts/image.hpp"
#include "ttx/semantic/negotiation/query.hpp"

TTX_DATA_RECORD(
    image_composite_operations,
    TTX_DATA_MEMBER(image_composite_operations, apply));

TTX_DATA_RECORD(
    image_composite,
    TTX_DATA_MEMBER(image_composite, source),
    TTX_DATA_MEMBER(image_composite, operations));

namespace Godot::Demo::Imaging::Contracts {

// Composition joins images that may come from different providers. Both use
// RGBA8 with straight alpha and equal dimensions. The result is source over in
// byte color space, with zero RGB when resulting alpha is zero. The receiving
// provider observes the overlay through its pixel protocol and keeps its own
// background storage private. A CPU overlay can therefore compose with a
// resident CUDA blur without the native classes sharing an object layout.
class Composite {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    GODOT_IMAGE_COMPOSITE_ID_HIGH,
    GODOT_IMAGE_COMPOSITE_ID_LOW,
  };

  using Api = image_composite;
  using Operations = image_composite_operations;

  static auto get_representation() -> const Ttx::Data::Form::Representation& {
    return Ttx::Semantic::Negotiation::Binding::representation<Composite>();
  }

  // The caller retains the receiver and its module through this borrowed view.
  // Success transfers one image reference for the enclosing image owner to
  // adopt. Error bytes remain borrowed until the next provider call.
  explicit constexpr Composite(Api api) : api(api) {}
  auto apply(image_object overlay) const -> Perimortem::Utility::
      Result<image_object, Perimortem::Core::View::Bytes> {
    image_object output = {};
    const auto error = api.operations->apply(api.source, overlay, &output);
    if (error.size) {
      return Perimortem::Core::View::Bytes(error.data, error.size);
    }

    return output;
  }

 private:
  Api api;
};

}  // namespace Godot::Demo::Imaging::Contracts
