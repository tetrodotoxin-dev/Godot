// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "providers/cuda/image.hpp"

#include "contracts/composite.hpp"
#include "contracts/convolve.hpp"
#include "contracts/invert.hpp"
#include "contracts/pixels.hpp"
#include "providers/cuda/composite.hpp"
#include "providers/cuda/convolve.hpp"
#include "providers/cuda/invert.hpp"

using namespace Godot;
using namespace Perimortem;

static auto allocate() -> U8* {
  using Image = Providers::Cuda::Image;
  static const Core::Object<>::Descriptor descriptor(
      sizeof(Image), alignof(Image),
      [](U8* bytes) { reinterpret_cast<Image*>(bytes)->~Image(); });
  return Core::Object<>::create(descriptor).get_payload();
}

Providers::Cuda::Image::Image(
    U8* allocation,
    Runtime& runtime,
    U32 w,
    U32 h,
    Allocation&& pixels)
    : Providers::Image(allocation, w, h),
      runtime(runtime),
      data(Core::Data::take(pixels)) {}

Providers::Cuda::Image::Image(
    U8* allocation,
    const Image& source,
    Allocation&& pixels)
    : Providers::Image(allocation, source),
      runtime(source.runtime),
      data(Core::Data::take(pixels)) {}

auto Providers::Cuda::Image::create(
    Runtime& runtime,
    U32 w,
    U32 h,
    Core::View::Bytes pixels) -> Utility::Result<Image&, Core::View::Bytes> {
  const auto invalid = Contracts::Pixels::validate(w, h, pixels);
  if (!invalid.is_empty()) {
    return invalid;
  }

  return runtime.upload(pixels).visit(
      [&](CUdeviceptr buffer) -> Utility::Result<Image&, Core::View::Bytes> {
        const auto allocation = allocate();
        return *new (allocation, Core::Placement::Construct)
            Image(allocation, runtime, w, h, Allocation(runtime, buffer));
      },
      [](Core::View::Bytes error)
          -> Utility::Result<Image&, Core::View::Bytes> { return error; });
}

auto Providers::Cuda::Image::derive(Allocation&& pixels) const -> Image& {
  const auto allocation = allocate();
  return *new (allocation, Core::Placement::Construct)
      Image(allocation, *this, Core::Data::take(pixels));
}

auto Providers::Cuda::Image::read_pixels(Core::Access::Bytes target) const
    -> Core::View::Bytes {
  return runtime.read(data.get(), target);
}

auto Providers::Cuda::Image::fulfill(System::Uuid contract) const -> Utility::
    Result<Ttx::Semantic::Binding, Ttx::Semantic::Binding::Failure> {
  if (contract == Contracts::Invert::contract_id) {
    static const image_invert_operations table = {
      [](const void* source, image_object* output) -> image_error {
        return Invert::apply(*static_cast<const Image*>(source))
            .visit(
                [&](Image& image) -> image_error {
                  *output = image.get_abi();
                  return {};
                },
                [](Core::View::Bytes error) -> image_error {
                  return {error.get_data(), error.get_size()};
                });
      },
    };

    return Ttx::Semantic::Binding::provide<Contracts::Invert>(this, table);
  }

  if (contract == Contracts::Convolve::contract_id) {
    static const image_convolve_operations table = {
      [](const void* source, image_kernel kernel,
         image_object* output) -> image_error {
        return Convolve::apply(*static_cast<const Image*>(source), kernel)
            .visit(
                [&](Image& image) -> image_error {
                  *output = image.get_abi();
                  return {};
                },
                [](Core::View::Bytes error) -> image_error {
                  return {error.get_data(), error.get_size()};
                });
      },
    };

    return Ttx::Semantic::Binding::provide<Contracts::Convolve>(this, table);
  }

  if (contract == Contracts::Composite::contract_id) {
    static const image_composite_operations table = {
      [](const void* source, image_object overlay,
         image_object* output) -> image_error {
        return Composite::apply(*static_cast<const Image*>(source), overlay)
            .visit(
                [&](Image& image) -> image_error {
                  *output = image.get_abi();
                  return {};
                },
                [](Core::View::Bytes error) -> image_error {
                  return {error.get_data(), error.get_size()};
                });
      },
    };

    return Ttx::Semantic::Binding::provide<Contracts::Composite>(this, table);
  }

  return Ttx::Semantic::Binding::Failure::Unsupported;
}
