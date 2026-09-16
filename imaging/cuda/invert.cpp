// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "imaging/cuda/invert.hpp"

using namespace Godot;
using namespace Perimortem;

auto Imaging::Cuda::Invert::apply(const Image& image)
    -> Utility::Result<Image&, Core::View::Bytes> {
  auto& runtime = image.get_runtime();
  U64 count = Count(image.get_width()) * image.get_height() * 4;
  return runtime.allocate(count).visit(
      [&](CUdeviceptr output) -> Utility::Result<Image&, Core::View::Bytes> {
        Allocation pending(runtime, output);
        auto input = image.get_buffer();
        void* arguments[] = {&input, &output, &count};
        auto error =
            runtime.get_program().launch("invert_rgba8", count, arguments);
        if (error.is_empty()) {
          error = runtime.finish();
        }

        if (!error.is_empty()) {
          return error;
        }

        return image.derive(Core::Data::take(pending));
      },
      [](Core::View::Bytes error)
          -> Utility::Result<Image&, Core::View::Bytes> { return error; });
}
