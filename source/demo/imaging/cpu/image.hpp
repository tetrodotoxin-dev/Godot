// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "perimortem/core/storage/result.hpp"
#include "perimortem/memory/global/bytes.hpp"

#include "demo/imaging/contracts/provider.h"

namespace Godot::Demo::Imaging::Cpu {

// CPU images publish immutable RGBA8 snapshots through Graph. Each acquisition
// keeps the pixels alive independently. This provider interprets Image's
// context. Foreign images are read through Compute Pixels before composition.
class Image {
 public:
  Image(U32 width, U32 height, Perimortem::Memory::Global::Bytes pixels);
  ~Image();
  static auto create(
      U32 width,
      U32 height,
      Perimortem::Memory::Global::Bytes pixels) -> ttx_retained;
  auto get_width() const -> U32 { return width; }
  auto get_height() const -> U32 { return height; }
  auto get_pixels() const -> Perimortem::Core::View::Bytes {
    return pixels.get_view();
  }

  auto graph() -> ttx_abstract;
  auto acquire() -> ttx_retained;
  auto release() -> ttx_release_status;

 private:
  Count references = 1;
  U32 width;
  U32 height;
  Perimortem::Memory::Global::Bytes pixels;
};

// The FFT implementation owns its scratch and returns independent pixel bytes.
// Its caller validates the kernel before entering the transform.
auto convolve(const Image& image, image_kernel kernel)
    -> Perimortem::Core::Storage::Result<
        Perimortem::Memory::Global::Bytes,
        Perimortem::Core::View::Bytes>;

}  // namespace Godot::Demo::Imaging::Cpu
