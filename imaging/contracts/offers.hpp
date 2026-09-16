// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once
#include "imaging/contracts/offers.h"
#include "ttx/concept/abstract.hpp"
#include "ttx/semantic/negotiation/query.hpp"

TTX_DATA_RECORD(
    image_offer,
    TTX_DATA_MEMBER(image_offer, contract),
    TTX_DATA_MEMBER(image_offer, name),
    TTX_DATA_MEMBER(image_offer, input),
    TTX_DATA_MEMBER(image_offer, minimum),
    TTX_DATA_MEMBER(image_offer, maximum),
    TTX_DATA_MEMBER(image_offer, step));
TTX_DATA_RECORD(
    image_offer_visitor,
    TTX_DATA_MEMBER(image_offer_visitor, source),
    TTX_DATA_MEMBER(image_offer_visitor, visit));
TTX_DATA_RECORD(
    image_admission,
    TTX_DATA_MEMBER(image_admission, status),
    TTX_DATA_MEMBER(image_admission, reason));
TTX_DATA_RECORD(
    image_offers,
    TTX_DATA_MEMBER(image_offers, source),
    TTX_DATA_MEMBER(image_offers, visit),
    TTX_DATA_MEMBER(image_offers, admit));

namespace Godot::Imaging::Contracts {

// Offers belongs to the image's policy surface. A caller can retain this view
// while retaining the image, but offered choices belong to the observation in
// which they were requested. A prepared operation keeps its own accepted
// inputs.
class Offers {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    GODOT_IMAGE_OFFERS_ID_HIGH,
    GODOT_IMAGE_OFFERS_ID_LOW,
  };
  using Api = image_offers;

  explicit constexpr Offers(Api api) : api(api) {}

  template <typename Visitor>
  auto visit(Visitor visitor) const -> void {
    api.visit(api.source, {&visitor, [](void* state, image_offer offer) {
                             (*static_cast<Visitor*>(state))(offer);
                           }});
  }

  auto admit(Perimortem::System::Uuid contract, U32 width = 0, U32 height = 0)
      const -> image_admission {
    return api.admit(api.source, contract, width, height);
  }

 private:
  Api api;
};

}  // namespace Godot::Imaging::Contracts
