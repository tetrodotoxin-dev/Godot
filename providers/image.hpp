// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/object.hpp"

#include "contracts/image.h"
#include "ttx/data/form/representation.hpp"
#include "ttx/semantic/query.hpp"

namespace Godot::Providers {

// Native providers share the mechanics of lending an owned image across the C
// boundary: retention, dimensions and a pixel publication. This optional C++
// helper implements those mechanics while each backend owns its actual storage
// and operation tables. A provider written in another language can supply the
// same C contract directly without inheriting or loading this class.
//
// The helper belongs to provider support, not the host's dependency graph.
// Its only concrete data description is the promised RGBA8 pixel surface.
// Deriving a CPU buffer or a device allocation remains the backend's decision.
class Image {
 public:
  Image(U8* allocation, U32 width, U32 height);
  virtual ~Image() = default;

  auto get_width() const -> U32 { return dimensions.width; }
  auto get_height() const -> U32 { return dimensions.height; }
  auto get_abi() const -> image_object;
  auto get_query() const -> Ttx::Semantic::Query;
  void release() const { Perimortem::Core::Object<>(allocation).release(); }

 protected:
  // An image operation can change every pixel while preserving the same form.
  // Retain the compiled bytes in that case, so derivation pays neither another
  // compilation nor another descriptor allocation. This owns no source pixels
  // and does not keep the source image alive.
  Image(U8* allocation, const Image& source);

  virtual auto fulfill(Perimortem::System::Uuid contract) const
      -> Perimortem::Utility::
          Result<Ttx::Semantic::Binding, Ttx::Semantic::Binding::Failure> = 0;
  virtual auto read_pixels(Perimortem::Core::Access::Bytes target) const
      -> Perimortem::Core::View::Bytes = 0;

 private:
  U8* allocation;
  image_dimensions dimensions;
  Perimortem::Core::Object<U8> description;
  Ttx::Data::Form::Representation representation;
};

}  // namespace Godot::Providers
