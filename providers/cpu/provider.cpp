// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "contracts/provider.h"

#include "providers/cpu/image.hpp"

using namespace Godot;
using namespace Perimortem;

PERIMORTEM_C __attribute__((visibility("default"))) image_error
    godot_image_provider_open_v2(image_provider* output) {
  static const image_provider_operations operations = {
    [](const void*) {},
    [](const void*) -> image_provider_statistics { return {}; },
    [](const void* source, U32 w, U32 h, const U8* data, Count size,
       image_object* output) -> image_error {
      return Providers::Cpu::Image::create(w, h, Core::View::Bytes(data, size))
          .visit(
              [&](Providers::Cpu::Image& image) -> image_error {
                *output = image.get_abi();
                return {};
              },
              [](Core::View::Bytes error) -> image_error {
                return {error.get_data(), error.get_size()};
              });
    },
  };

  *output = {nullptr, &operations};
  return {};
}
