// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "adapters/imaging/scripts/factory.hpp"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/object.hpp"

#include "adapters/imaging/scripts/image.hpp"
#include "adapters/imaging/scripts/invocation.hpp"
#include "imaging/contracts/pixels.hpp"

using namespace Godot;
using namespace Perimortem;

Adapters::Imaging::Scripts::Factory::Factory(
    U8* allocation,
    const godot::Ref<godot::RefCounted>& object)
    : allocation(allocation),
      object(object),
      creation(object.ptr(), "create_image") {}

auto Adapters::Imaging::Scripts::Factory::create(
    const godot::Ref<godot::RefCounted>& object)
    -> Utility::Result<image_provider, Core::View::Bytes> {
  if (object.is_null() || !object->has_method("create_image")) {
    return "GDScript factory requires a RefCounted object with create_image."_view;
  }

  static const Core::Object<>::Descriptor descriptor(
      sizeof(Factory), alignof(Factory),
      [](U8* value) { reinterpret_cast<Factory*>(value)->~Factory(); });
  auto storage = Core::Object<>::create(descriptor).get_payload();
  auto* factory =
      new (storage, Core::Placement::Construct) Factory(storage, object);
  static const image_provider_operations operations = {
    [](const void* source) {
      Core::Object<>(static_cast<const Factory*>(source)->allocation).release();
    },
    [](const void*) -> image_provider_statistics { return {}; },
    [](const void* source, U32 width, U32 height, const U8* data, Count size,
       image_object* output) -> image_error {
      const auto& factory = *static_cast<const Factory*>(source);
      const auto invalid = Godot::Imaging::Contracts::Pixels::validate(
          width, height, {data, size});
      if (!invalid.is_empty()) {
        return {invalid.get_data(), invalid.get_size()};
      }

      // The C input is borrowed for this call. The script may retain its
      // argument, so it receives a Godot owned array with an independent
      // life.
      godot::PackedByteArray pixels;
      pixels.resize(size);
      Core::Data::copy(pixels.ptrw(), data, size);
      return Invocation::call(
                 factory.creation, factory.errors, width, height, pixels)
          .visit(
              [&](const godot::Variant& result) -> image_error {
                if (result.get_type() != godot::Variant::OBJECT) {
                  constexpr auto error =
                      "GDScript factory did not return an image."_view;
                  return {error.get_data(), error.get_size()};
                }

                const godot::Ref<godot::RefCounted> object = result;
                return Image::create(width, height, object)
                    .visit(
                        [&](Image& image) -> image_error {
                          *output = image.get_abi();
                          return {};
                        },
                        [](Core::View::Bytes error) -> image_error {
                          return {error.get_data(), error.get_size()};
                        });
              },
              [](Core::View::Bytes error) -> image_error {
                return {error.get_data(), error.get_size()};
              });
    },
  };

  return image_provider{factory, &operations};
}

auto Adapters::Imaging::Scripts::Factory::open(
    const godot::String& path,
    Memory::Allocator::Arena& errors)
    -> Utility::Result<image_provider, Core::View::Bytes> {
  const auto resource =
      godot::ResourceLoader::get_singleton()->load(path, "Script");
  if (resource.is_null()) {
    return "GDScript provider could not load its factory script."_view;
  }

  Memory::Dynamic::Bytes failure;
  return Invocation::call(godot::Callable(resource.ptr(), "new"), failure)
      .visit(
          [&](const godot::Variant& result)
              -> Utility::Result<image_provider, Core::View::Bytes> {
            if (result.get_type() != godot::Variant::OBJECT) {
              return "GDScript provider could not construct its factory."_view;
            }

            const godot::Ref<godot::RefCounted> object = result;
            return create(object);
          },
          [&](Core::View::Bytes error)
              -> Utility::Result<image_provider, Core::View::Bytes> {
            return errors.proxy(error);
          });
}
