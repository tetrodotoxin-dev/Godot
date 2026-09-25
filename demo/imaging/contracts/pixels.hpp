// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/utility/result.hpp"

#include "demo/imaging/contracts/image.h"

namespace Godot::Demo::Imaging::Contracts {

// A compositor can receive an overlay from a different provider, so reading
// its pixels must use the same public protocol as the Godot host. Pixels makes
// that observation through Data Flow and returns independently owned bytes.
// Neither caller needs the producer's native Image class or dependency graph.
class Pixels {
 public:
  static auto read(image_object image) -> Perimortem::Utility::
      Result<Perimortem::Memory::Dynamic::Bytes, Perimortem::Core::View::Bytes>;

  static auto validate(
      U32 width,
      U32 height,
      Perimortem::Core::View::Bytes pixels) -> Perimortem::Core::View::Bytes;
};

}  // namespace Godot::Demo::Imaging::Contracts
