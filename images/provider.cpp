// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "images/provider.hpp"

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/object.hpp"

using namespace Godot;
using namespace Perimortem;

Images::Provider::Provider(
    image_provider factory,
    const Vocabulary& contract,
    Core::Option<Modules::Library> library)
    : library(Core::Data::take(library)),
      factory(factory),
      contract(contract) {}

Images::Provider::~Provider() {
  factory.operations->release(factory.source);
}

auto Images::Provider::adopt(
    image_provider owned,
    const Vocabulary& vocabulary,
    Core::Option<Modules::Library> library) -> Provider& {
  static const Core::Object<>::Descriptor descriptor(
      sizeof(Provider), alignof(Provider),
      [](U8* value) { reinterpret_cast<Provider*>(value)->~Provider(); });
  auto storage = Core::Object<>::create(descriptor).get_payload();
  return *new (storage, Core::Placement::Construct)
      Provider(owned, vocabulary, Core::Data::take(library));
}

auto Images::Provider::statistics() const -> image_provider_statistics {
  return factory.operations->statistics(factory.source);
}

void Images::Provider::retain() {
  Core::Object<>(reinterpret_cast<U8*>(this)).retain();
}

void Images::Provider::release() {
  Core::Object<>(reinterpret_cast<U8*>(this)).release();
}

auto Images::Provider::create(
    U32 w,
    U32 h,
    Core::View::Bytes pixels,
    image_object& output) const -> image_error {
  return factory.operations->create(
      factory.source, w, h, pixels.get_data(), pixels.get_size(), &output);
}

auto Images::Provider::open(
    Core::View::Bytes path,
    const Vocabulary& vocabulary,
    Memory::Allocator::Arena& errors)
    -> Utility::Result<Provider&, Core::View::Bytes> {
  return Modules::Library::open(path, errors)
      .visit(
          [&](Modules::Library& library)
              -> Utility::Result<Provider&, Core::View::Bytes> {
            return library.symbol("godot_image_provider_open_v2"_view, errors)
                .visit(
                    [&](void* address)
                        -> Utility::Result<Provider&, Core::View::Bytes> {
                      auto entry =
                          reinterpret_cast<image_provider_open>(address);
                      image_provider factory = {};
                      const auto failed = entry(&factory);
                      if (failed.size) {
                        return errors.proxy(
                            Core::View::Bytes(failed.data, failed.size));
                      }

                      return adopt(
                          factory, vocabulary, Core::Data::take(library));
                    },
                    [](Core::View::Bytes error)
                        -> Utility::Result<Provider&, Core::View::Bytes> {
                      return error;
                    });
          },
          [](Core::View::Bytes error)
              -> Utility::Result<Provider&, Core::View::Bytes> {
            return error;
          });
}
