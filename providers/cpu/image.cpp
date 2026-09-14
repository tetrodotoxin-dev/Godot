// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "providers/cpu/image.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "contracts/composite.hpp"
#include "contracts/convolve.hpp"
#include "contracts/invert.hpp"
#include "contracts/pixels.hpp"
#include "providers/cpu/composite.hpp"
#include "providers/cpu/convolve.hpp"
#include "providers/cpu/invert.hpp"

using namespace Godot;
using namespace Perimortem;

static auto allocate() -> U8* {
  using Image = Providers::Cpu::Image;
  static const Core::Object<>::Descriptor descriptor(
      sizeof(Image), alignof(Image),
      [](U8* bytes) { reinterpret_cast<Image*>(bytes)->~Image(); });
  return Core::Object<>::create(descriptor).get_payload();
}

Providers::Cpu::Image::Image(
    U8* allocation,
    U32 w,
    U32 h,
    Memory::Dynamic::Bytes&& pixels)
    : Providers::Image(allocation, w, h), data(Core::Data::take(pixels)) {}

Providers::Cpu::Image::Image(
    U8* allocation,
    const Image& source,
    Memory::Dynamic::Bytes&& pixels)
    : Providers::Image(allocation, source), data(Core::Data::take(pixels)) {}

auto Providers::Cpu::Image::create(U32 w, U32 h, Core::View::Bytes pixels)
    -> Utility::Result<Image&, Core::View::Bytes> {
  const auto invalid = Contracts::Pixels::validate(w, h, pixels);
  if (!invalid.is_empty()) {
    return invalid;
  }

  const auto allocation = allocate();
  return *new (allocation, Core::Placement::Construct)
      Image(allocation, w, h, Memory::Dynamic::Bytes(pixels));
}

auto Providers::Cpu::Image::derive(Memory::Dynamic::Bytes&& pixels) const
    -> Image& {
  const auto allocation = allocate();
  return *new (allocation, Core::Placement::Construct)
      Image(allocation, *this, Core::Data::take(pixels));
}

auto Providers::Cpu::Image::read_pixels(Core::Access::Bytes target) const
    -> Core::View::Bytes {
  Core::Data::copy(
      target.get_data(), data.get_view().get_data(), target.get_size());
  return Core::View::Bytes();
}

auto Providers::Cpu::Image::fulfill(System::Uuid contract) const -> Utility::
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
