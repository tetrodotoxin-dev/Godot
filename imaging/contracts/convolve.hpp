// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "imaging/contracts/image.hpp"
#include "ttx/semantic/negotiation/query.hpp"

TTX_DATA_RECORD(
    image_convolve_operations,
    TTX_DATA_MEMBER(image_convolve_operations, apply));

TTX_DATA_RECORD(
    image_convolve,
    TTX_DATA_MEMBER(image_convolve, source),
    TTX_DATA_MEMBER(image_convolve, operations));

namespace Godot::Imaging::Contracts {

// Convolution accepts float32 coefficients in row order for a centered,
// RGB filter with zero padding. Alpha remains the source alpha. The kernel has
// odd dimensions up to 1023, with finite coefficients of magnitude at most 1e6.
// Coefficients are borrowed only until this synchronous call returns. Keeping
// the capability separate lets providers choose FFT or spatial implementations
// without adding methods to every image owner.
class Convolve {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    GODOT_IMAGE_CONVOLVE_ID_HIGH,
    GODOT_IMAGE_CONVOLVE_ID_LOW,
  };

  using Api = image_convolve;
  using Operations = image_convolve_operations;

  static auto get_representation() -> const Ttx::Data::Form::Representation& {
    return Ttx::Semantic::Negotiation::Binding::representation<Convolve>();
  }

  // The caller retains the receiver and its module through this borrowed view.
  // Success transfers one image reference for the enclosing image owner to
  // adopt. Error bytes remain borrowed until the next provider call.
  explicit constexpr Convolve(Api api) : api(api) {}
  auto apply(image_kernel kernel) const -> Perimortem::Utility::
      Result<image_object, Perimortem::Core::View::Bytes> {
    image_object output = {};
    const auto error = api.operations->apply(api.source, kernel, &output);
    if (error.size) {
      return Perimortem::Core::View::Bytes(error.data, error.size);
    }

    return output;
  }

 private:
  Api api;
};

}  // namespace Godot::Imaging::Contracts
