// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/imaging/contracts/provider.h"
#include "demo/imaging/cpu/image.hpp"
#include "demo/imaging/render/runtime.hpp"
#include "demo/sampling/cpu/samples.h"
#include "demo/sampling/provider.hpp"
#include "ttx/semantic/negotiation/library.h"

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

static auto samples(ttx_query_receiver receive) -> ttx_binding_status {
  static const sample_operations operations = {godot_cpu_samples};
  static const Sampling::Provider publication(
      nullptr, operations, [](const void*) {}, [](const void*) {});
  return receive.receive(receive.source, publication.get_query());
}

PERIMORTEM_C __attribute__((visibility("default"))) ttx_binding_status
    ttx_query(ttx_semantic_query, ttx_query_receiver receive) {
  return Imaging::Render::Runtime::visit(images, samples, receive);
}
