// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "contracts/image.h"
#include "contracts/operation.hpp"
#include "ttx/semantic/bound.hpp"
#include "ttx/semantic/thunk.hpp"

namespace Godot::Contracts {

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

  static constexpr auto convention =
      Ttx::Semantic::Thunk::Convention::SystemVAMD64;
  using Operations = image_composite_operations;

  static auto get_representation() -> const Ttx::Data::Form::Representation& {
    return Operation::get_representation();
  }

  // The caller retains the receiver and its module through this borrowed view.
  // Success transfers one image reference for the enclosing image owner to
  // adopt. Error bytes remain borrowed until the next provider call.
  class Handle : public Ttx::Semantic::Bound<Operations> {
   public:
    using Bound::Bound;
    auto apply(image_object overlay) const -> Perimortem::Utility::
        Result<image_object, Perimortem::Core::View::Bytes> {
      image_object output = {};
      const auto error = operations.apply(source, overlay, &output);
      if (error.size) {
        return Perimortem::Core::View::Bytes(error.data, error.size);
      }

      return output;
    }
  };
};

}  // namespace Godot::Contracts
