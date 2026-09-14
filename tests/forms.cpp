// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tests/forms.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "images/image.hpp"

using namespace Godot;
using namespace Perimortem;

static void require(Bool condition, Core::View::Bytes message) {
  if (!condition) {
    Core::Diagnostics::Log::fatal(message);
  }
}

template <typename Value>
static auto accepted(Utility::Result<Value, Core::View::Bytes> result)
    -> Value {
  return result.visit(
      [](Value& value) -> Value { return static_cast<Value&&>(value); },
      [](Core::View::Bytes error) -> Value {
        Core::Diagnostics::Log::fatal(error);
      });
}

static auto representation(const Images::Image& image)
    -> const Ttx::Data::Form::Representation& {
  const auto object = image.get_abi();
  return *object.operations->representation(object.source);
}

static void sharing(Images::Provider& provider, Bool device) {
  const auto live_images = provider.statistics().live_images;
  Memory::Allocator::Arena errors;
  const U8 pixels[] = {1, 2, 3, 255};
  const U8 inverted[] = {254, 253, 252, 255};
  const auto* operation = provider.get_vocabulary().find("invert"_view);
  require(operation != nullptr, "Form check requires inversion."_view);
  Core::Option<Images::Image> retained;
  const U8* bytes = nullptr;
  {
    auto source = accepted(
        Images::Image::create(
            provider, 1, 1, Core::View::Bytes(pixels, 4), errors));
    retained = accepted(source.apply(*operation, nullptr, nullptr, errors));
    bytes = representation(source).get_bytes().get_data();

    // This pointer comparison checks an allocation reuse decision, not semantic
    // identity. Each Representation still describes the exact public pixel
    // bytes, and independent publications remain valid through byte agreement.
    require(
        bytes == representation(*retained).get_bytes().get_data(),
        "Derived image compiled another form."_view);
    require(
        accepted(source.read_pixels(errors)).get_view() ==
            Core::View::Bytes(pixels, 4),
        "Derivation changed source pixels."_view);
  }

  // The originating image has ended. Publish a different shape while the old
  // derivative still needs its form, then derive again from that survivor.
  // Retaining the source image would also keep CUDA's source allocation alive.
  // Only the derivative should remain before the next publication starts.
  if (device) {
    require(
        provider.statistics().live_images == live_images + 1,
        "Sharing a form retained its source device image."_view);
  }

  const U8 other_pixels[] = {9, 8, 7, 255, 6, 5, 4, 255};
  auto other = accepted(
      Images::Image::create(
          provider, 2, 1, Core::View::Bytes(other_pixels, 8), errors));
  require(
      !representation(*retained).compatible(representation(other)),
      "Different pixel extents reused the same form."_view);
  require(
      representation(*retained).get_extent() == 4 &&
          representation(other).get_extent() == 8,
      "Another publication changed the retained form."_view);
  require(
      accepted(retained->read_pixels(errors)).get_view() ==
          Core::View::Bytes(inverted, 4),
      "Retained derivative lost its pixels or form."_view);

  auto next = accepted(retained->apply(*operation, nullptr, nullptr, errors));
  retained = {};
  require(
      representation(next).get_bytes().get_data() == bytes,
      "Second derivation lost form sharing."_view);
  require(
      accepted(next.read_pixels(errors)).get_view() ==
          Core::View::Bytes(pixels, 4),
      "Second derivative did not survive source release."_view);
}

void Tests::Forms::check(Images::Provider& provider, Bool device) {
  // Bibliotheca retains free pool blocks for reuse. Compare active allocation
  // bytes so growing that pool cannot be mistaken for a surviving form owner.
  const auto allocated = Core::Bibliotheca::allocated_memory();
  const auto live_images = provider.statistics().live_images;
  sharing(provider, device);
  require(
      Core::Bibliotheca::allocated_memory() == allocated,
      "Form storage remained allocated after its final image ended."_view);
  require(
      provider.statistics().live_images == live_images,
      "Form lifetime check leaked a device image."_view);
  Core::Diagnostics::Log::info(
      "PASS: shared pixel forms and independent image lifetimes"_view);
}
