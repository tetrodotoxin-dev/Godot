// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/imaging/cpu/image.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "demo/imaging/contracts/composite.hpp"
#include "demo/imaging/contracts/convolve.hpp"
#include "demo/imaging/contracts/invert.hpp"
#include "demo/imaging/contracts/pixels.hpp"
#include "demo/imaging/cpu/composite.hpp"
#include "demo/imaging/cpu/convolve.hpp"
#include "demo/imaging/cpu/invert.hpp"

using namespace Godot::Demo;
using namespace Perimortem;

static auto allocate() -> U8* {
  using Image = Imaging::Cpu::Image;
  static const Core::Object<>::Descriptor descriptor(
      sizeof(Image), alignof(Image),
      [](U8* bytes) { reinterpret_cast<Image*>(bytes)->~Image(); });
  return Core::Object<>::create(descriptor).get_payload();
}

Imaging::Cpu::Image::Image(
    U8* allocation,
    U32 w,
    U32 h,
    Memory::Dynamic::Bytes&& pixels)
    : Imaging::Publication::Image(allocation, w, h),
      data(Core::Data::take(pixels)) {}

Imaging::Cpu::Image::Image(
    U8* allocation,
    const Image& source,
    Memory::Dynamic::Bytes&& pixels)
    : Imaging::Publication::Image(allocation, source),
      data(Core::Data::take(pixels)) {}

auto Imaging::Cpu::Image::create(U32 w, U32 h, Core::View::Bytes pixels)
    -> Utility::Result<Image&, Core::View::Bytes> {
  const auto invalid = Imaging::Contracts::Pixels::validate(w, h, pixels);
  if (!invalid.is_empty()) {
    return invalid;
  }

  const auto allocation = allocate();
  return *new (allocation, Core::Placement::Construct)
      Image(allocation, w, h, Memory::Dynamic::Bytes(pixels));
}

auto Imaging::Cpu::Image::derive(Memory::Dynamic::Bytes&& pixels) const
    -> Image& {
  const auto allocation = allocate();
  return *new (allocation, Core::Placement::Construct)
      Image(allocation, *this, Core::Data::take(pixels));
}

auto Imaging::Cpu::Image::read_pixels(Core::Access::Bytes target) const
    -> Core::View::Bytes {
  Core::Data::copy(
      target.get_data(), data.get_view().get_data(), target.get_size());
  return Core::View::Bytes();
}

auto Imaging::Cpu::Image::supports(System::Uuid contract) const
    -> Ttx::Semantic::Negotiation::Binding::Status {
  using Ttx::Semantic::Negotiation::Binding::Status;
  return contract == Imaging::Contracts::Invert::contract_id ||
                 contract == Imaging::Contracts::Convolve::contract_id ||
                 contract == Imaging::Contracts::Composite::contract_id
             ? Status::Satisfied
             : Status::Unsupported;
}

auto Imaging::Cpu::Image::fulfill(
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
