// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "providers/cpu/invert.hpp"

using namespace Godot;
using namespace Perimortem;

auto Providers::Cpu::Invert::apply(const Image& image)
    -> Utility::Result<Image&, Core::View::Bytes> {
  Memory::Dynamic::Bytes output(image.get_pixels());
  auto pixels = output.get_access();
  for (Count i = 0; i < pixels.get_size(); i += 4) {
    pixels.get_data()[i] ^= 255;
    pixels.get_data()[i + 1] ^= 255;
    pixels.get_data()[i + 2] ^= 255;
  }

  return image.derive(Core::Data::take(output));
}
