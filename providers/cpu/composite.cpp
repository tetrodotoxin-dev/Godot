// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "providers/cpu/composite.hpp"

#include "perimortem/core/null_terminated.hpp"

using namespace Godot;
using namespace Perimortem;

#include "contracts/pixels.hpp"

auto Providers::Cpu::Composite::apply(const Image& image, image_object overlay)
    -> Utility::Result<Image&, Core::View::Bytes> {
  const auto dimensions = overlay.operations->dimensions(overlay.source);
  if (dimensions.width != image.get_width() ||
      dimensions.height != image.get_height()) {
    return "Composite requires equally sized images."_view;
  }

  return Contracts::Pixels::read(overlay).visit(
      [&](Memory::Dynamic::Bytes& front)
          -> Utility::Result<Image&, Core::View::Bytes> {
        Memory::Dynamic::Bytes output(image.get_pixels());
        auto pixels = output.get_access().get_data();
        const auto other = front.get_view();
        // Keeping alpha as a numerator avoids division until the final color.
        // All terms fit U32 and use the same rounding rule on either processor.
        for (Count i = 0; i < output.get_size(); i += 4) {
          const U32 a = other[i + 3], b = pixels[i + 3];
          const U32 alpha = a * 255 + b * (255 - a);
          for (U32 c = 0; c < 3; ++c) {
            const U32 color =
                other[i + c] * a * 255 + pixels[i + c] * b * (255 - a);
            pixels[i + c] = alpha ? (color + alpha / 2) / alpha : 0;
          }

          pixels[i + 3] = (alpha + 127) / 255;
        }

        return image.derive(Core::Data::take(output));
      },
      [](Core::View::Bytes error)
          -> Utility::Result<Image&, Core::View::Bytes> { return error; });
}
