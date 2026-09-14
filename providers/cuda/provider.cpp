// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "contracts/provider.h"

#include "providers/cuda/image.hpp"

using namespace Godot;
using namespace Perimortem;

PERIMORTEM_C __attribute__((visibility("default"))) image_error
    godot_image_provider_open_v2(image_provider* output) {
  Providers::Cuda::Runtime* runtime = nullptr;
  image_error error = {};
  Providers::Cuda::Runtime::create().visit(
      [&](Providers::Cuda::Runtime& value) { runtime = &value; },
      [&](Core::View::Bytes message) {
        error = {message.get_data(), message.get_size()};
      });
  if (error.size) {
    return error;
  }

  static const image_provider_operations operations = {
    [](const void* source) {
      const_cast<Providers::Cuda::Runtime*>(
          static_cast<const Providers::Cuda::Runtime*>(source))
          ->release();
    },
    [](const void* source) -> image_provider_statistics {
      const auto& runtime =
          *static_cast<const Providers::Cuda::Runtime*>(source);
      return {
        runtime.get_uploads(), runtime.get_downloads(),
        runtime.get_live_images(), runtime.get_plan_builds()};
    },
    [](const void* source, U32 w, U32 h, const U8* data, Count size,
       image_object* output) -> image_error {
      return Providers::Cuda::Image::create(
                 *const_cast<Providers::Cuda::Runtime*>(
                     static_cast<const Providers::Cuda::Runtime*>(source)),
                 w, h, Core::View::Bytes(data, size))
          .visit(
              [&](Providers::Cuda::Image& image) -> image_error {
                *output = image.get_abi();
                return {};
              },
              [](Core::View::Bytes error) -> image_error {
                return {error.get_data(), error.get_size()};
              });
    },
  };

  *output = {runtime, &operations};
  return {};
}
