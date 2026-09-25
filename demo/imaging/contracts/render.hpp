// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/system/uuid.hpp"

#include "perimortem/utility/result.hpp"

#include "demo/imaging/contracts/provider.hpp"
#include "demo/imaging/contracts/render.h"

TTX_DATA_RECORD(
    render_provider_operations,
    TTX_DATA_MEMBER(render_provider_operations, open));

TTX_DATA_RECORD(
    render_api,
    TTX_DATA_MEMBER(render_api, source),
    TTX_DATA_MEMBER(render_api, operations));

namespace Godot::Demo::Imaging::Contracts {

// The emitted Render factory can serve both generated classes and the lab's
// persistent image graph. Opening the provider supplies its independent state,
// while the caller retains the native module through the last image release.
class Render {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    LAB_RENDER_PROVIDER_ID_HIGH,
    LAB_RENDER_PROVIDER_ID_LOW,
  };
  using Api = render_api;
  using Operations = render_provider_operations;

  // This view borrows the emitted factory. Success acquires a separate image
  // provider, so releasing that factory does not revoke the image operations.
  explicit constexpr Render(Api api) : api(api) {}
  auto open() const -> Perimortem::Utility::
      Result<image_provider, Perimortem::Core::View::Bytes> {
    image_provider output = {};
    const auto failed = api.operations->open(api.source, &output);
    if (failed.size) {
      return Perimortem::Core::View::Bytes(failed.data, failed.size);
    }
    return output;
  }

 private:
  Api api;
};

}  // namespace Godot::Demo::Imaging::Contracts
