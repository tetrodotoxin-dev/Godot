// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "adapters/imaging/scripts/image.hpp"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>

#include "perimortem/core/null_terminated.hpp"

#include "imaging/contracts/composite.hpp"
#include "imaging/contracts/convolve.hpp"
#include "imaging/contracts/invert.hpp"
#include "imaging/contracts/kernel.hpp"
#include "imaging/contracts/pixels.hpp"

using namespace Godot;
using namespace Perimortem;

static auto validate(const godot::Ref<godot::RefCounted>& object)
    -> Core::View::Bytes {
  if (object.is_null()) {
    return "GDScript must return a RefCounted image."_view;
  }

  if (!object->has_method("supports") || !object->has_method("fulfill") ||
      !object->has_method("read_pixels")) {
    return "GDScript image requires supports, fulfill and read_pixels."_view;
  }

  return Core::View::Bytes();
}

static auto allocate() -> U8* {
  using Image = Adapters::Imaging::Scripts::Image;
  static const Core::Object<>::Descriptor descriptor(
      sizeof(Image), alignof(Image),
      [](U8* storage) { reinterpret_cast<Image*>(storage)->~Image(); });
  return Core::Object<>::create(descriptor).get_payload();
}

static auto reply(
    Utility::Result<Adapters::Imaging::Scripts::Image&, Core::View::Bytes>
        result,
    image_object* output) -> image_error {
  return result.visit(
      [&](Adapters::Imaging::Scripts::Image& image) -> image_error {
        *output = image.get_abi();
        return {};
      },
      [](Core::View::Bytes error) -> image_error {
        return {error.get_data(), error.get_size()};
      });
}

Adapters::Imaging::Scripts::Image::Image(
    U8* allocation,
    U32 width,
    U32 height,
    const godot::Ref<godot::RefCounted>& object)
    : Godot::Imaging::Publication::Image(allocation, width, height),
      object(object),
      publication(object.ptr(), "fulfill"),
      pixels(object.ptr(), "read_pixels") {}

Adapters::Imaging::Scripts::Image::Image(
    U8* allocation,
    const Image& source,
    const godot::Ref<godot::RefCounted>& object)
    : Godot::Imaging::Publication::Image(allocation, source),
      object(object),
      publication(object.ptr(), "fulfill"),
      pixels(object.ptr(), "read_pixels") {}

auto Adapters::Imaging::Scripts::Image::create(
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

auto Adapters::Imaging::Scripts::Image::select(
    System::Uuid contract,
    godot::Callable& slot) const
    -> Core::Option<Ttx::Semantic::Negotiation::Binding::Failure> {
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
              -> Core::Option<Ttx::Semantic::Negotiation::Binding::Failure> {
            if (answer.get_type() == godot::Variant::INT) {
              const int64_t status = answer;
              if (status == TTX_BINDING_UNSUPPORTED ||
                  status == TTX_BINDING_PENDING ||
                  status == TTX_BINDING_REJECTED) {
                return static_cast<
                    Ttx::Semantic::Negotiation::Binding::Failure>(status);
              }
            }

            if (answer.get_type() == godot::Variant::CALLABLE) {
              const godot::Callable selected = answer;
              if (selected.is_valid()) {
                slot = selected;
                return {};
              }
            }

            return Ttx::Semantic::Negotiation::Binding::Failure::Rejected;
          },
          [](Core::View::Bytes)
              -> Core::Option<Ttx::Semantic::Negotiation::Binding::Failure> {
            return Ttx::Semantic::Negotiation::Binding::Failure::Rejected;
          });
}

auto Adapters::Imaging::Scripts::Image::supports(System::Uuid contract) const
    -> Ttx::Semantic::Negotiation::Binding::Status {
  using Ttx::Semantic::Negotiation::Binding::Status;
  const auto identity = contract.serialize();
  const auto name = godot::String::utf8(
      reinterpret_cast<const char*>(identity.get_data()), identity.get_size());
  // Ask the script's property operation without acquiring or caching its
  // Callable. The script can promise a contract even when fulfillment fails.
  return Invocation::call(
             godot::Callable(object.ptr(), "supports"), errors, name)
      .visit(
          [](const godot::Variant& answer) -> Status {
            if (answer.get_type() != godot::Variant::INT) {
              return Status::Rejected;
            }

            const int64_t status = answer;
            return status >= TTX_BINDING_SATISFIED &&
                           status <= TTX_BINDING_REJECTED
                       ? static_cast<Status>(status)
                       : Status::Rejected;
          },
          [](Core::View::Bytes) { return Status::Rejected; });
}

auto Adapters::Imaging::Scripts::Image::fulfill(
    System::Uuid contract,
    Ttx::Data::Form::Storage requested) const
    -> Ttx::Semantic::Negotiation::Binding::Status {
  if (contract == Godot::Imaging::Contracts::Invert::contract_id) {
    if (auto failed = select(contract, inversion)) {
      return static_cast<Ttx::Semantic::Negotiation::Binding::Status>(*failed);
    }

    static const image_invert_operations table = {
      [](const void* source, image_object* output) {
        return reply(static_cast<const Image*>(source)->invert(), output);
      },
    };

    return Ttx::Semantic::Negotiation::Binding::provide<
        Godot::Imaging::Contracts::Invert>(
        Godot::Imaging::Contracts::Invert::Api(this, &table), requested);
  }

  if (contract == Godot::Imaging::Contracts::Convolve::contract_id) {
    if (auto failed = select(contract, convolution)) {
      return static_cast<Ttx::Semantic::Negotiation::Binding::Status>(*failed);
    }

    static const image_convolve_operations table = {
      [](const void* source, image_kernel kernel, image_object* output) {
        return reply(
            static_cast<const Image*>(source)->convolve(
                kernel, static_cast<const Image*>(source)->convolution),
            output);
      },
    };

    return Ttx::Semantic::Negotiation::Binding::provide<
        Godot::Imaging::Contracts::Convolve>(
        Godot::Imaging::Contracts::Convolve::Api(this, &table), requested);
  }

  if (contract == Godot::Imaging::Contracts::Composite::contract_id) {
    if (auto failed = select(contract, composition)) {
      return static_cast<Ttx::Semantic::Negotiation::Binding::Status>(*failed);
    }

    static const image_composite_operations table = {
      [](const void* source, image_object overlay, image_object* output) {
        return reply(
            static_cast<const Image*>(source)->composite(
                overlay, static_cast<const Image*>(source)->composition),
            output);
      },
    };

    return Ttx::Semantic::Negotiation::Binding::provide<
        Godot::Imaging::Contracts::Composite>(
        Godot::Imaging::Contracts::Composite::Api(this, &table), requested);
  }

  // A project's UUID supplies the behavior. Its offer selects one of the
  // image argument forms this script boundary can translate, and binding still
  // checks the complete requested Representation. Selection retains the chosen
  // Callable so future execution needs neither discovery nor script lookup.
  U8 input = 255;
  auto find = [&](image_offer offer) {
    if (System::Uuid(offer.contract) == contract) {
      input = offer.input;
    }
  };
  visit_offers({&find, [](void* source, image_offer offer) {
                  (*static_cast<decltype(find)*>(source))(offer);
                }});
  if (input > IMAGE_INPUT_IMAGE) {
    return Ttx::Semantic::Negotiation::Binding::Status::Unsupported;
  }

  godot::Callable callable;
  if (auto failure = select(contract, callable)) {
    return static_cast<Ttx::Semantic::Negotiation::Binding::Status>(*failure);
  }

  Core::Object<Selection> storage(1);
  auto* value = storage.get_access().get_data();
  value->image = this;
  value->callable = callable;
  const auto publish = [&]() {
    using namespace Ttx::Semantic::Negotiation;
    switch (input) {
    case IMAGE_INPUT_NONE: {
      static const image_invert_operations operations = {
        [](const void* source, image_object* output) -> image_error {
          const auto& selected = *static_cast<const Selection*>(source);
          return reply(
              selected.image->receive(
                  Invocation::call(selected.callable, selected.image->errors)),
              output);
        }};
      return Binding::provide<Godot::Imaging::Contracts::Invert>(
          Godot::Imaging::Contracts::Invert::Api(value, &operations),
          requested);
    }
    case IMAGE_INPUT_KERNEL: {
      static const image_convolve_operations operations = {
        [](const void* source, image_kernel kernel, image_object* output) {
          const auto& selected = *static_cast<const Selection*>(source);
          return reply(
              selected.image->convolve(kernel, selected.callable), output);
        }};
      return Binding::provide<Godot::Imaging::Contracts::Convolve>(
          Godot::Imaging::Contracts::Convolve::Api(value, &operations),
          requested);
    }
    case IMAGE_INPUT_IMAGE: {
      static const image_composite_operations operations = {
        [](const void* source, image_object overlay, image_object* output) {
          const auto& selected = *static_cast<const Selection*>(source);
          return reply(
              selected.image->composite(overlay, selected.callable), output);
        }};
      return Binding::provide<Godot::Imaging::Contracts::Composite>(
          Godot::Imaging::Contracts::Composite::Api(value, &operations),
          requested);
    }
    }

    return Binding::Status::Unsupported;
  };
  const auto status = publish();
  if (status == Ttx::Semantic::Negotiation::Binding::Status::Satisfied) {
    selected.emplace(Core::Data::take(storage));
  }
  return status;
}

auto Adapters::Imaging::Scripts::Image::receive(
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

auto Adapters::Imaging::Scripts::Image::invert() const
    -> Utility::Result<Image&, Core::View::Bytes> {
  return receive(Invocation::call(inversion, errors));
}

auto Adapters::Imaging::Scripts::Image::convolve(
    image_kernel kernel,
    const godot::Callable& callable) const
    -> Utility::Result<Image&, Core::View::Bytes> {
  const auto invalid = Godot::Imaging::Contracts::Kernel::validate(
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
      Invocation::call(callable, errors, weights, kernel.width, kernel.height));
}

auto Adapters::Imaging::Scripts::Image::composite(
    image_object overlay,
    const godot::Callable& callable) const
    -> Utility::Result<Image&, Core::View::Bytes> {
  const auto dimensions = overlay.operations->dimensions(overlay.source);
  if (dimensions.width != get_width() || dimensions.height != get_height()) {
    return "Composite requires equally sized images."_view;
  }

  // Observe a foreign image through its Data contract before entering the
  // script. Passing a retained Godot array lets the script keep that
  // observation without accidentally retaining a borrowed native image or its
  // module.
  return Godot::Imaging::Contracts::Pixels::read(overlay).visit(
      [&](Memory::Dynamic::Bytes& pixels)
          -> Utility::Result<Image&, Core::View::Bytes> {
        godot::PackedByteArray values;
        values.resize(pixels.get_size());
        Core::Data::copy(
            values.ptrw(), pixels.get_view().get_data(), pixels.get_size());
        return receive(Invocation::call(callable, errors, values));
      },
      [](Core::View::Bytes error)
          -> Utility::Result<Image&, Core::View::Bytes> { return error; });
}

auto Adapters::Imaging::Scripts::Image::read_pixels(
    Core::Access::Bytes target) const -> Core::View::Bytes {
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

// GDScript owns the available choices. The native boundary only translates
// their C carriers, including the UUID used for subsequent admission and
// binding.
auto Adapters::Imaging::Scripts::Image::visit_offers(
    image_offer_visitor visitor) const -> void {
  if (!object->has_method("offers")) {
    return Godot::Imaging::Publication::Image::visit_offers(visitor);
  }
  Invocation::call(godot::Callable(object.ptr(), "offers"), errors)
      .visit(
          [&](const godot::Variant& value) {
            if (value.get_type() != godot::Variant::ARRAY) {
              return;
            }
            const godot::Array list = value;
            for (int64_t i = 0; i != list.size(); ++i) {
              const godot::Dictionary item = list[i];
              const auto uuid = godot::String(item.get("contract", "")).utf8();
              const auto name = godot::String(item.get("name", "")).utf8();
              if (uuid.length() != 36) {
                continue;
              }
              const System::Uuid contract(
                  Core::Static::Bytes<36>(
                      reinterpret_cast<const U8*>(uuid.get_data())));
              const image_offer offer{
                contract,
                {reinterpret_cast<const U8*>(name.get_data()),
                 Count(name.length())},
                U8(int64_t(item.get("input", 0))),
                U32(int64_t(item.get("minimum", 0))),
                U32(int64_t(item.get("maximum", 0))),
                U32(int64_t(item.get("step", 0)))};
              visitor.visit(visitor.source, offer);
            }
          },
          [](Core::View::Bytes) {});
}

auto Adapters::Imaging::Scripts::Image::admit(
    System::Uuid contract,
    U32 width,
    U32 height) const -> image_admission {
  if (!object->has_method("admit")) {
    return Godot::Imaging::Publication::Image::admit(contract, width, height);
  }
  const auto id = contract.serialize();
  const auto name = godot::String::utf8(
      reinterpret_cast<const char*>(id.get_data()), id.get_size());
  return Invocation::call(
             godot::Callable(object.ptr(), "admit"), errors, name, width,
             height)
      .visit(
          [&](const godot::Variant& value) -> image_admission {
            if (value.get_type() != godot::Variant::DICTIONARY) {
              return {TTX_BINDING_REJECTED, {nullptr, 0}};
            }
            const godot::Dictionary answer = value;
            const auto reason = godot::String(answer.get("reason", "")).utf8();
            errors = Memory::Dynamic::Bytes(
                {reinterpret_cast<const U8*>(reason.get_data()),
                 Count(reason.length())});
            const S64 status =
                int64_t(answer.get("status", TTX_BINDING_REJECTED));
            return {
              status >= 0 && status <= TTX_BINDING_REJECTED
                  ? U8(status)
                  : TTX_BINDING_REJECTED,
              {errors.get_view().get_data(), errors.get_size()}};
          },
          [&](Core::View::Bytes error) -> image_admission {
            return {TTX_BINDING_REJECTED, {error.get_data(), error.get_size()}};
          });
}
