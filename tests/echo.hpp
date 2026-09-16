// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "imaging/contracts/image.hpp"
#include "ttx/semantic/negotiation/query.hpp"

namespace Godot::Tests {

// Echo has no script arguments, but its native signature includes a fixed
// discriminator. The inline function slot also differs from Invert's table
// pointer, so neither the script argument family nor record size can select
// this API. The registered host adapter must acquire its actual contract.
class Echo {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    0x87ca69a1a5ce4c12,
    0x93621e91e6f7a762,
  };
  static constexpr U32 discriminator = 0x545458;

  struct Api {
    const void* source;
    image_error (*apply)(const void*, U32, image_object*);
  };

  explicit constexpr Echo(Api api) : api(api) {}
  static auto accept(Api api) -> Bool { return api.apply != nullptr; }
  static auto get_representation() -> const Ttx::Data::Form::Representation&;

  auto apply() const -> Perimortem::Utility::
      Result<image_object, Perimortem::Core::View::Bytes> {
    image_object image = {};
    const auto error = api.apply(api.source, discriminator, &image);
    if (error.size) {
      return Perimortem::Core::View::Bytes(error.data, error.size);
    }

    return image;
  }

 private:
  Api api;
};

}  // namespace Godot::Tests

TTX_DATA_RECORD(
    Godot::Tests::Echo::Api,
    TTX_DATA_MEMBER(Godot::Tests::Echo::Api, source),
    TTX_DATA_MEMBER(Godot::Tests::Echo::Api, apply));

inline auto Godot::Tests::Echo::get_representation()
    -> const Ttx::Data::Form::Representation& {
  return Ttx::Semantic::Negotiation::Binding::representation<Echo>();
}
