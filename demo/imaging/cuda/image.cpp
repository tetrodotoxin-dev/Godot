// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/imaging/cuda/image.hpp"

#include "demo/imaging/contracts/composite.hpp"
#include "demo/imaging/contracts/convolve.hpp"
#include "demo/imaging/contracts/invert.hpp"
#include "demo/imaging/contracts/pixels.hpp"
#include "demo/imaging/cuda/composite.hpp"
#include "demo/imaging/cuda/convolve.hpp"
#include "demo/imaging/cuda/invert.hpp"

using namespace Godot::Demo;
using namespace Perimortem;

static auto allocate() -> U8* {
  using Image = Imaging::Cuda::Image;
  static const Core::Object<>::Descriptor descriptor(
      sizeof(Image), alignof(Image),
      [](U8* bytes) { reinterpret_cast<Image*>(bytes)->~Image(); });
  return Core::Object<>::create(descriptor).get_payload();
}

Imaging::Cuda::Image::Image(
    U8* allocation,
    Runtime& runtime,
    U32 w,
    U32 h,
    Allocation&& pixels)
    : Imaging::Publication::Image(allocation, w, h),
      runtime(runtime),
      data(Core::Data::take(pixels)) {}

Imaging::Cuda::Image::Image(
    U8* allocation,
    const Image& source,
    Allocation&& pixels)
    : Imaging::Publication::Image(allocation, source),
      runtime(source.runtime),
      data(Core::Data::take(pixels)) {}

auto Imaging::Cuda::Image::create(
    Runtime& runtime,
    U32 w,
    U32 h,
    Core::View::Bytes pixels) -> Utility::Result<Image&, Core::View::Bytes> {
  const auto invalid = Imaging::Contracts::Pixels::validate(w, h, pixels);
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

auto Imaging::Cuda::Image::derive(Allocation&& pixels) const -> Image& {
  const auto allocation = allocate();
  return *new (allocation, Core::Placement::Construct)
      Image(allocation, *this, Core::Data::take(pixels));
}

auto Imaging::Cuda::Image::read_pixels(Core::Access::Bytes target) const
    -> Core::View::Bytes {
  return runtime.read(data.get(), target);
}

auto Imaging::Cuda::Image::supports(System::Uuid contract) const
    -> Ttx::Semantic::Negotiation::Binding::Status {
  using Ttx::Semantic::Negotiation::Binding::Status;
  return contract == Imaging::Contracts::Invert::contract_id ||
                 contract == Imaging::Contracts::Convolve::contract_id ||
                 contract == Imaging::Contracts::Composite::contract_id
             ? Status::Satisfied
             : Status::Unsupported;
}

auto Imaging::Cuda::Image::fulfill(
    System::Uuid contract,
    Ttx::Data::Form::Storage requested) const
    -> Ttx::Semantic::Negotiation::Binding::Status {
  if (contract == Imaging::Contracts::Invert::contract_id) {
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

    return Ttx::Semantic::Negotiation::Binding::provide<
        Imaging::Contracts::Invert>(
        Imaging::Contracts::Invert::Api(this, &table), requested);
  }

  if (contract == Imaging::Contracts::Convolve::contract_id) {
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

    return Ttx::Semantic::Negotiation::Binding::provide<
        Imaging::Contracts::Convolve>(
        Imaging::Contracts::Convolve::Api(this, &table), requested);
  }

  if (contract == Imaging::Contracts::Composite::contract_id) {
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

    return Ttx::Semantic::Negotiation::Binding::provide<
        Imaging::Contracts::Composite>(
        Imaging::Contracts::Composite::Api(this, &table), requested);
  }

  return Ttx::Semantic::Negotiation::Binding::Status::Unsupported;
}
