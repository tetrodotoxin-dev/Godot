// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/imaging/render/module.hpp"

#include "cuda/runtime/program.hpp"
#include "demo/imaging/contracts/provider.h"
#include "demo/imaging/cuda/image.hpp"
#include "demo/sampling/cuda/samples.hpp"
#include "ttx/concept/modules/module.h"

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

static auto samples(ttx_publication* output) -> ttx_data_status {
  return Sampling::Cuda::Samples::create().visit(
      [&](ttx_publication provider) -> ttx_data_status {
        *output = provider;
        return TTX_DATA_SUCCESS;
      },
      [](Ttx::Data::Status error) {
        return static_cast<ttx_data_status>(error);
      });
}

PERIMORTEM_C __attribute__((visibility("default"))) ttx_data_status
    ttx_module_open(ttx_semantic_query, ttx_module_acquisition* output) {
  return Imaging::Render::Module::open(
      images, samples, output, ::Cuda::Runtime::Program::compiler());
}
