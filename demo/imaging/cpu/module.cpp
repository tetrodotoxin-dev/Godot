// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/imaging/render/module.hpp"

#include "demo/imaging/contracts/provider.h"
#include "demo/imaging/cpu/image.hpp"
#include "demo/sampling/cpu/samples.h"
#include "demo/sampling/publication.hpp"
#include "ttx/concept/modules/module.h"

using namespace Godot::Demo;
using namespace Perimortem;

static auto images(image_provider* output) -> image_error {
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

static auto samples(ttx_publication* output) -> ttx_data_status {
  static const sample_operations operations = {godot_cpu_samples};
  static const Sampling::Publication publication(
      nullptr, operations, [](const void*) {});
  *output = publication.get_publication();
  return TTX_DATA_SUCCESS;
}

PERIMORTEM_C __attribute__((visibility("default"))) ttx_data_status
    ttx_module_open(ttx_semantic_query, ttx_module_acquisition* output) {
  return Imaging::Render::Module::open(images, samples, output);
}
