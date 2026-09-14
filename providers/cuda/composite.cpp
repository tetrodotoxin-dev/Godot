// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "providers/cuda/composite.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "contracts/pixels.hpp"

using namespace Godot;
using namespace Perimortem;

static auto blend(const Providers::Cuda::Image& image, Core::View::Bytes pixels)
    -> Utility::Result<Providers::Cuda::Allocation, Core::View::Bytes> {
  auto& runtime = image.get_runtime();
  // Only the foreign overlay crosses into this context. The receiver and its
  // cached ancestors stay resident, including when a new overlay is published.
  auto uploaded = runtime.upload(pixels);
  return uploaded.visit(
      [&](CUdeviceptr overlay)
          -> Utility::Result<Providers::Cuda::Allocation, Core::View::Bytes> {
        const Providers::Cuda::Allocation foreground(runtime, overlay);
        auto allocated = runtime.allocate(pixels.get_size());
        return allocated.visit(
            [&](CUdeviceptr output)
                -> Utility::Result<
                    Providers::Cuda::Allocation, Core::View::Bytes> {
              Providers::Cuda::Allocation pending(runtime, output);
              auto background = image.get_buffer();
              U32 count = image.get_width() * image.get_height();
              void* arguments[] = {&background, &overlay, &output, &count};
              auto error = runtime.get_program().launch(
                  "composite_rgba8", count, arguments);
              if (error.is_empty()) {
                error = runtime.finish();
              }

              if (!error.is_empty()) {
                return error;
              }

              return Core::Data::take(pending);
            },
            [](Core::View::Bytes error)
                -> Utility::Result<
                    Providers::Cuda::Allocation, Core::View::Bytes> {
              return error;
            });
      },
      [](Core::View::Bytes error)
          -> Utility::Result<Providers::Cuda::Allocation, Core::View::Bytes> {
        return error;
      });
}

auto Providers::Cuda::Composite::apply(const Image& image, image_object overlay)
    -> Utility::Result<Image&, Core::View::Bytes> {
  const auto dimensions = overlay.operations->dimensions(overlay.source);
  if (dimensions.width != image.get_width() ||
      dimensions.height != image.get_height()) {
    return "Composite requires equally sized images."_view;
  }

  return Contracts::Pixels::read(overlay).visit(
      [&](Memory::Dynamic::Bytes& front)
          -> Utility::Result<Image&, Core::View::Bytes> {
        return blend(image, front.get_view())
            .visit(
                [&](Allocation& pixels)
                    -> Utility::Result<Image&, Core::View::Bytes> {
                  return image.derive(Core::Data::take(pixels));
                },
                [](Core::View::Bytes error)
                    -> Utility::Result<Image&, Core::View::Bytes> {
                  return error;
                });
      },
      [](Core::View::Bytes error)
          -> Utility::Result<Image&, Core::View::Bytes> { return error; });
}
