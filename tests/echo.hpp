// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "contracts/image.h"
#include "contracts/operation.hpp"
#include "ttx/semantic/bound.hpp"
#include "ttx/semantic/thunk.hpp"

namespace Godot::Tests {

// Echo has no script arguments, but its native signature includes a fixed
// discriminator. This makes the fixture detect a host that chooses Invert's
// table from the Godot argument family instead of invoking the requested UUID.
class Echo {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    0x87ca69a1a5ce4c12,
    0x93621e91e6f7a762,
  };

  static constexpr auto convention =
      Ttx::Semantic::Thunk::Convention::SystemVAMD64;
  static constexpr U32 discriminator = 0x545458;

  struct Operations {
    image_error (*apply)(const void*, U32, image_object*);
  };

  static auto get_representation() -> const Ttx::Data::Form::Representation& {
    return Contracts::Operation::get_representation();
  }

  class Handle : public Ttx::Semantic::Bound<Operations> {
   public:
    using Bound::Bound;
    auto apply() const -> Perimortem::Utility::
        Result<image_object, Perimortem::Core::View::Bytes> {
      image_object image = {};
      const auto error = operations.apply(source, discriminator, &image);
      if (error.size) {
        return Perimortem::Core::View::Bytes(error.data, error.size);
      }

      return image;
    }
  };
};

}  // namespace Godot::Tests
