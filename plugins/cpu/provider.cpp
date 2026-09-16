// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "plugins/cpu/provider.hpp"

#include "imaging/cpu/image.hpp"
#include "plugins/render/module.hpp"
#include "sampling/cpu/samples.h"
#include "sampling/publication.hpp"
#include "ttx/concept/modules/module.h"

using namespace Godot;
using namespace Perimortem;

auto Plugins::Cpu::Provider::images(image_provider* output) -> image_error {
  static const image_provider_operations operations = {
    [](const void*) {},
    [](const void*) -> image_provider_statistics { return {}; },
    [](const void* source, U32 w, U32 h, const U8* data, Count size,
       image_object* output) -> image_error {
      return Imaging::Cpu::Image::create(w, h, Core::View::Bytes(data, size))
          .visit(
              [&](Imaging::Cpu::Image& image) -> image_error {
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

PERIMORTEM_C __attribute__((visibility("default"))) ttx_data_status
    ttx_module_open(ttx_semantic_query, ttx_module_acquisition* output) {
  return Plugins::Render::Module::open(
      Plugins::Cpu::Provider::images, Plugins::Cpu::Provider::samples, output);
}

auto Plugins::Cpu::Provider::samples(ttx_publication* output)
    -> ttx_data_status {
  static const sample_operations operations = {godot_cpu_samples};
  static const Sampling::Publication publication(
      nullptr, operations, [](const void*) {});
  *output = publication.get_publication();
  return TTX_DATA_SUCCESS;
}
