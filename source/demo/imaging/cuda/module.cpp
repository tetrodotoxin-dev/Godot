// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "cuda/runtime/program.hpp"
#include "demo/imaging/contracts/provider.h"
#include "demo/imaging/cuda/image.hpp"
#include "demo/imaging/render/runtime.hpp"
#include "demo/sampling/cuda/samples.hpp"
#include "ttx/semantic/negotiation/library.h"

using namespace Godot::Demo;
using namespace Perimortem;

static auto images(image_provider* output) -> image_error {
  Imaging::Cuda::Runtime* runtime = nullptr;
  image_error error = {};
  Imaging::Cuda::Runtime::create().visit(
      [&](Imaging::Cuda::Runtime& value) { runtime = &value; },
      [&](Core::View::Bytes message) {
        error = {message.get_data(), message.get_size()};
      });
  if (error.size) {
    return error;
  }

  static const image_provider_operations operations = {
    [](const void* source) {
      const_cast<Imaging::Cuda::Runtime*>(
          static_cast<const Imaging::Cuda::Runtime*>(source))
          ->release();
    },
    [](const void* source) -> image_provider_statistics {
      const auto& runtime = *static_cast<const Imaging::Cuda::Runtime*>(source);
      return {
        runtime.get_uploads(), runtime.get_downloads(),
        runtime.get_live_images(), runtime.get_plan_builds()};
    },
    [](const void* source, U32 w, U32 h, const U8* data, Count size,
       image_object* output) -> image_error {
      return Imaging::Cuda::Image::create(
                 *const_cast<Imaging::Cuda::Runtime*>(
                     static_cast<const Imaging::Cuda::Runtime*>(source)),
                 w, h, Core::View::Bytes(data, size))
          .visit(
              [&](Imaging::Cuda::Image& image) -> image_error {
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

static auto samples(ttx_query_receiver receive) -> ttx_binding_status {
  return Sampling::Cuda::Samples::create(receive);
}

PERIMORTEM_C __attribute__((visibility("default"))) ttx_binding_status
    ttx_query(ttx_semantic_query, ttx_query_receiver receive) {
  return Imaging::Render::Runtime::visit(
      images, samples, receive, ::Cuda::Runtime::Program::compiler());
}
