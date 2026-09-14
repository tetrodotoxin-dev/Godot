// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "providers/gdscript/image.hpp"

#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>

#include "perimortem/core/null_terminated.hpp"

#include "contracts/composite.hpp"
#include "contracts/convolve.hpp"
#include "contracts/invert.hpp"
#include "contracts/kernel.hpp"
#include "contracts/pixels.hpp"

using namespace Godot;
using namespace Perimortem;

static auto validate(const godot::Ref<godot::RefCounted>& object)
    -> Core::View::Bytes {
  if (object.is_null()) {
    return "GDScript must return a RefCounted image."_view;
  }

  if (!object->has_method("fulfill") || !object->has_method("read_pixels")) {
    return "GDScript image requires fulfill and read_pixels."_view;
  }

  return Core::View::Bytes();
}

static auto allocate() -> U8* {
  using Image = Providers::Gdscript::Image;
  static const Core::Object<>::Descriptor descriptor(
      sizeof(Image), alignof(Image),
      [](U8* storage) { reinterpret_cast<Image*>(storage)->~Image(); });
  return Core::Object<>::create(descriptor).get_payload();
}

static auto reply(
    Utility::Result<Providers::Gdscript::Image&, Core::View::Bytes> result,
    image_object* output) -> image_error {
  return result.visit(
      [&](Providers::Gdscript::Image& image) -> image_error {
        *output = image.get_abi();
        return {};
      },
      [](Core::View::Bytes error) -> image_error {
        return {error.get_data(), error.get_size()};
      });
}

Providers::Gdscript::Image::Image(
    U8* allocation,
    U32 width,
    U32 height,
    const godot::Ref<godot::RefCounted>& object)
    : Providers::Image(allocation, width, height),
      object(object),
      publication(object.ptr(), "fulfill"),
      pixels(object.ptr(), "read_pixels") {}

Providers::Gdscript::Image::Image(
    U8* allocation,
    const Image& source,
    const godot::Ref<godot::RefCounted>& object)
    : Providers::Image(allocation, source),
      object(object),
      publication(object.ptr(), "fulfill"),
      pixels(object.ptr(), "read_pixels") {}

auto Providers::Gdscript::Image::create(
    U32 width,
    U32 height,
    const godot::Ref<godot::RefCounted>& object)
    -> Utility::Result<Image&, Core::View::Bytes> {
  const auto invalid = validate(object);
  if (!invalid.is_empty()) {
    return invalid;
  }

  const auto storage = allocate();
  return *new (storage, Core::Placement::Construct)
      Image(storage, width, height, object);
}

auto Providers::Gdscript::Image::select(
    System::Uuid contract,
    godot::Callable& slot) const
    -> Core::Option<Ttx::Semantic::Binding::Failure> {
  if (!slot.is_null()) {
    return {};
  }

  // This spelling is only the script boundary for the same UUID. It does not
  // become another registry or replace the caller's contract identity. A
  // successful publication keeps its Callable for the image's entire lifetime.
  const auto identity = contract.serialize();
  const auto name = godot::String::utf8(
      reinterpret_cast<const char*>(identity.get_data()), identity.get_size());
  return Invocation::call(publication, errors, name)
      .visit(
          [&](const godot::Variant& answer)
              -> Core::Option<Ttx::Semantic::Binding::Failure> {
            if (answer.get_type() == godot::Variant::INT) {
              const int64_t status = answer;
              if (status == TTX_BINDING_UNSUPPORTED ||
                  status == TTX_BINDING_PENDING ||
                  status == TTX_BINDING_REJECTED) {
                return static_cast<Ttx::Semantic::Binding::Failure>(status);
              }
            }

            if (answer.get_type() == godot::Variant::CALLABLE) {
              const godot::Callable selected = answer;
              if (selected.is_valid()) {
                slot = selected;
                return {};
              }
            }

            return Ttx::Semantic::Binding::Failure::Rejected;
          },
          [](Core::View::Bytes)
              -> Core::Option<Ttx::Semantic::Binding::Failure> {
            return Ttx::Semantic::Binding::Failure::Rejected;
          });
}

auto Providers::Gdscript::Image::fulfill(System::Uuid contract) const
    -> Utility::
        Result<Ttx::Semantic::Binding, Ttx::Semantic::Binding::Failure> {
  if (contract == Contracts::Invert::contract_id) {
    if (auto failed = select(contract, inversion)) {
      return *failed;
    }

    static const image_invert_operations table = {
      [](const void* source, image_object* output) {
        return reply(static_cast<const Image*>(source)->invert(), output);
      },
    };

    return Ttx::Semantic::Binding::provide<Contracts::Invert>(this, table);
  }

  if (contract == Contracts::Convolve::contract_id) {
    if (auto failed = select(contract, convolution)) {
      return *failed;
    }

    static const image_convolve_operations table = {
      [](const void* source, image_kernel kernel, image_object* output) {
        return reply(
            static_cast<const Image*>(source)->convolve(kernel), output);
      },
    };

    return Ttx::Semantic::Binding::provide<Contracts::Convolve>(this, table);
  }

  if (contract == Contracts::Composite::contract_id) {
    if (auto failed = select(contract, composition)) {
      return *failed;
    }

    static const image_composite_operations table = {
      [](const void* source, image_object overlay, image_object* output) {
        return reply(
            static_cast<const Image*>(source)->composite(overlay), output);
      },
    };

    return Ttx::Semantic::Binding::provide<Contracts::Composite>(this, table);
  }

  return Ttx::Semantic::Binding::Failure::Unsupported;
}

auto Providers::Gdscript::Image::receive(
    Utility::Result<godot::Variant, Core::View::Bytes> result) const
    -> Utility::Result<Image&, Core::View::Bytes> {
  return result.visit(
      [&](const godot::Variant& value)
          -> Utility::Result<Image&, Core::View::Bytes> {
        if (value.get_type() != godot::Variant::OBJECT) {
          return "GDScript operation did not return an image."_view;
        }

        const godot::Ref<godot::RefCounted> image = value;
        const auto invalid = validate(image);
        if (!invalid.is_empty()) {
          return invalid;
        }

        const auto storage = allocate();
        return *new (storage, Core::Placement::Construct)
            Image(storage, *this, image);
      },
      [](Core::View::Bytes error)
          -> Utility::Result<Image&, Core::View::Bytes> { return error; });
}

auto Providers::Gdscript::Image::invert() const
    -> Utility::Result<Image&, Core::View::Bytes> {
  return receive(Invocation::call(inversion, errors));
}

auto Providers::Gdscript::Image::convolve(image_kernel kernel) const
    -> Utility::Result<Image&, Core::View::Bytes> {
  const auto invalid = Contracts::Kernel::validate(
      kernel.width, kernel.height, {kernel.values, kernel.count});
  if (!invalid.is_empty()) {
    return invalid;
  }

  // The script can retain its arguments, while the C contract lends this
  // coefficient array for one call. A Godot owned array closes that lifetime
  // boundary without exposing a caller's native storage to the script.
  godot::PackedFloat32Array weights;
  weights.resize(kernel.count);
  Core::Data::copy(
      reinterpret_cast<U8*>(weights.ptrw()), kernel.values, kernel.count);
  return receive(
      Invocation::call(
          convolution, errors, weights, kernel.width, kernel.height));
}

auto Providers::Gdscript::Image::composite(image_object overlay) const
    -> Utility::Result<Image&, Core::View::Bytes> {
  const auto dimensions = overlay.operations->dimensions(overlay.source);
  if (dimensions.width != get_width() || dimensions.height != get_height()) {
    return "Composite requires equally sized images."_view;
  }

  // Observe a foreign image through its Data contract before entering the
  // script. Passing a retained Godot array lets the script keep that
  // observation without accidentally retaining a borrowed native image or its
  // module.
  return Contracts::Pixels::read(overlay).visit(
      [&](Memory::Dynamic::Bytes& pixels)
          -> Utility::Result<Image&, Core::View::Bytes> {
        godot::PackedByteArray values;
        values.resize(pixels.get_size());
        Core::Data::copy(
            values.ptrw(), pixels.get_view().get_data(), pixels.get_size());
        return receive(Invocation::call(composition, errors, values));
      },
      [](Core::View::Bytes error)
          -> Utility::Result<Image&, Core::View::Bytes> { return error; });
}

auto Providers::Gdscript::Image::read_pixels(Core::Access::Bytes target) const
    -> Core::View::Bytes {
  return Invocation::call(pixels, errors)
      .visit(
          [&](const godot::Variant& result) -> Core::View::Bytes {
            if (result.get_type() != godot::Variant::PACKED_BYTE_ARRAY) {
              return "GDScript pixel observation must return PackedByteArray."_view;
            }

            const godot::PackedByteArray bytes = result;
            if (Count(bytes.size()) != target.get_size()) {
              return "GDScript pixel observation has the wrong extent."_view;
            }

            Core::Data::copy(target.get_data(), bytes.ptr(), target.get_size());
            return Core::View::Bytes();
          },
          [](Core::View::Bytes error) { return error; });
}
