// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "imaging/contracts/image.hpp"
#include "ttx/semantic/negotiation/query.hpp"

TTX_DATA_RECORD(
    image_invert_operations,
    TTX_DATA_MEMBER(image_invert_operations, apply));

TTX_DATA_RECORD(
    image_invert,
    TTX_DATA_MEMBER(image_invert, source),
    TTX_DATA_MEMBER(image_invert, operations));

namespace Godot::Imaging::Contracts {

// Inversion complements RGB bytes and preserves alpha while leaving the
// source unchanged. A provider may return another device allocation, so
// acquiring or invoking this capability does not promise host pixel storage.
class Invert {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    GODOT_IMAGE_INVERT_ID_HIGH,
    GODOT_IMAGE_INVERT_ID_LOW,
  };

  using Api = image_invert;
  using Operations = image_invert_operations;

  static auto get_representation() -> const Ttx::Data::Form::Representation& {
    return Ttx::Semantic::Negotiation::Binding::representation<Invert>();
  }

  // The caller retains the receiver and its module through this borrowed view.
  // Success transfers one image reference for the enclosing image owner to
  // adopt. Error bytes remain borrowed until the next provider call.
  explicit constexpr Invert(Api api) : api(api) {}
  auto apply() const -> Perimortem::Utility::
      Result<image_object, Perimortem::Core::View::Bytes> {
    image_object output = {};
    const auto error = api.operations->apply(api.source, &output);
    if (error.size) {
      return Perimortem::Core::View::Bytes(error.data, error.size);
    }

    return output;
  }

 private:
  Api api;
};

}  // namespace Godot::Imaging::Contracts
