// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "contracts/image.h"
#include "contracts/operation.hpp"
#include "ttx/semantic/bound.hpp"
#include "ttx/semantic/thunk.hpp"

namespace Godot::Contracts {

// Inversion complements RGB bytes and preserves alpha while leaving the
// source unchanged. A provider may return another device allocation, so
// acquiring or invoking this capability does not promise host pixel storage.
class Invert {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    GODOT_IMAGE_INVERT_ID_HIGH,
    GODOT_IMAGE_INVERT_ID_LOW,
  };

  static constexpr auto convention =
      Ttx::Semantic::Thunk::Convention::SystemVAMD64;
  using Operations = image_invert_operations;

  static auto get_representation() -> const Ttx::Data::Form::Representation& {
    return Operation::get_representation();
  }

  // The caller retains the receiver and its module through this borrowed view.
  // Success transfers one image reference for the enclosing image owner to
  // adopt. Error bytes remain borrowed until the next provider call.
  class Handle : public Ttx::Semantic::Bound<Operations> {
   public:
    using Bound::Bound;
    auto apply() const -> Perimortem::Utility::
        Result<image_object, Perimortem::Core::View::Bytes> {
      image_object output = {};
      const auto error = operations.apply(source, &output);
      if (error.size) {
        return Perimortem::Core::View::Bytes(error.data, error.size);
      }

      return output;
    }
  };
};

}  // namespace Godot::Contracts
