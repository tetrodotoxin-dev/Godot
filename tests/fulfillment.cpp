// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tests/fulfillment.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "images/image.hpp"
#include "tests/echo.hpp"

using namespace Godot;
using namespace Perimortem;

static void require(Bool value, Core::View::Bytes message) {
  if (!value) {
    Core::Diagnostics::Log::fatal(message);
  }
}

void Tests::Fulfillment::check(
    Core::View::Bytes module,
    const Images::Vocabulary& vocabulary) {
  Memory::Allocator::Arena errors;
  auto& provider = Images::Provider::open(module, vocabulary, errors)
                       .visit(
                           [](Images::Provider& provider) -> Images::Provider& {
                             return provider;
                           },
                           [](Core::View::Bytes error) -> Images::Provider& {
                             Core::Diagnostics::Log::fatal(error);
                           });
  const auto operation = Images::Operation::unary<Echo>("echo"_view);

  for (U8 outcome = 0; outcome <= TTX_BINDING_REJECTED; ++outcome) {
    const U8 pixels[] = {outcome, 2, 3, 255};
    auto image =
        Images::Image::create(provider, 1, 1, {pixels, 4}, errors)
            .visit(
                [](Images::Image& image) { return Core::Data::take(image); },
                [](Core::View::Bytes error) -> Images::Image {
                  Core::Diagnostics::Log::fatal(error);
                });

    if (outcome) {
      image.bind_operation(operation).visit(
          [](const Ttx::Semantic::Binding&) {
            require(False, "Provider refusal was bypassed."_view);
          },
          [&](Ttx::Semantic::Binding::Failure failure) {
            require(
                static_cast<U8>(failure) == outcome,
                "Native provider lost its refusal status."_view);
          });
      continue;
    }

    // The production Image consumer must use Echo's captured Handle, even
    // though Godot describes this method and invert with the same input kind.
    image.apply(operation, nullptr, nullptr, errors)
        .visit(
            [&](Images::Image& output) {
              require(
                  output.get_width() == 1 && output.get_height() == 1,
                  "Typed operation publication returned an invalid image."_view);
            },
            [](Core::View::Bytes error) {
              Core::Diagnostics::Log::fatal(error);
            });
  }

  provider.release();
  Core::Diagnostics::Log::info(
      "PASS: typed operation identity and native refusal propagation"_view);
}
