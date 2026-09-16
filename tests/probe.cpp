// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/null_terminated.hpp"

#include "imaging/contracts/provider.h"
#include "imaging/publication/image.hpp"
#include "plugins/render/module.hpp"
#include "tests/echo.hpp"
#include "ttx/concept/modules/module.h"

using namespace Godot;
using namespace Perimortem;

// A separately loaded native provider exercises the real publication helper.
// The first supplied pixel selects a binding outcome. This state stays private
// to the fixture, while the host must preserve the Core result it receives.
class Probe : public Imaging::Publication::Image {
 public:
  static auto create(U8 outcome) -> Probe& {
    static const Core::Object<>::Descriptor descriptor(
        sizeof(Probe), alignof(Probe),
        [](U8* bytes) { reinterpret_cast<Probe*>(bytes)->~Probe(); });
    const auto allocation = Core::Object<>::create(descriptor).get_payload();
    return *new (allocation, Core::Placement::Construct)
        Probe(allocation, outcome);
  }

 protected:
  auto supports(System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status override {
    using Ttx::Semantic::Negotiation::Binding::Status;
    if (outcome) {
      return static_cast<Status>(outcome);
    }

    return id == Tests::Echo::contract_id ? Status::Satisfied
                                          : Status::Unsupported;
  }

  auto fulfill(System::Uuid id, Ttx::Data::Form::Storage requested) const
      -> Ttx::Semantic::Negotiation::Binding::Status override {
    if (outcome) {
      return static_cast<Ttx::Semantic::Negotiation::Binding::Status>(outcome);
    }

    if (id != Tests::Echo::contract_id) {
      return Ttx::Semantic::Negotiation::Binding::Status::Unsupported;
    }

    const Tests::Echo::Api api = {
      this,
      [](const void* source, U32 discriminator,
         image_object* output) -> image_error {
        if (discriminator != Tests::Echo::discriminator) {
          constexpr auto error =
              "Host selected the wrong callable signature."_view;
          return {error.get_data(), error.get_size()};
        }

        const auto image = static_cast<const Probe*>(source)->get_abi();
        image.operations->retain(image.source);
        *output = image;
        return {};
      },
    };

    return Ttx::Semantic::Negotiation::Binding::provide<Tests::Echo>(
        api, requested);
  }

  auto read_pixels(Core::Access::Bytes target) const
      -> Core::View::Bytes override {
    const U8 pixels[] = {outcome, 2, 3, 255};
    Core::Data::copy(target.get_data(), pixels, sizeof(pixels));
    return Core::View::Bytes();
  }

 private:
  Probe(U8* allocation, U8 outcome)
      : Imaging::Publication::Image(allocation, 1, 1), outcome(outcome) {}
  U8 outcome;
};

static auto open_images(image_provider* output) -> image_error {
  static const image_provider_operations operations = {
    [](const void*) {},
    [](const void*) -> image_provider_statistics { return {}; },
    [](const void*, U32 width, U32 height, const U8* pixels, Count size,
       image_object* output) -> image_error {
      if (width != 1 || height != 1 || size != 4) {
        constexpr auto error = "Probe requires one RGBA8 pixel."_view;
        return {error.get_data(), error.get_size()};
      }

      *output = Probe::create(pixels[0]).get_abi();
      return {};
    },
  };

  *output = {nullptr, &operations};
  return {};
}

PERIMORTEM_C __attribute__((visibility("default"))) ttx_data_status
    ttx_module_open(ttx_semantic_query, ttx_module_acquisition* output) {
  return Plugins::Render::Module::open(open_images, nullptr, output);
}
