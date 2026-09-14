// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "extension/ttx_image.hpp"

#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "contracts/kernel.hpp"
#include "contracts/pixels.hpp"
#include "images/kernel.hpp"
#include "images/observation.hpp"
#include "images/vocabulary.hpp"
#include "operations/standard.hpp"
#include "providers/gdscript/factory.hpp"

using namespace Godot;
using namespace Perimortem;

static auto host_string(Core::View::Bytes bytes) -> godot::String {
  return godot::String::utf8(
      reinterpret_cast<const char*>(bytes.get_data()), bytes.get_size());
}

// The engine copies registration metadata. Only the exact TTX definition is
// retained as method data, so there is no second operation inventory to update.
static void bind_operation(
    const Images::Operation& definition,
    GDExtensionClassMethodCall invoke) {
  godot::StringName name(host_string(definition.get_name()));
  godot::StringName class_name("TtxImage");
  Memory::Dynamic::Vector<godot::PropertyInfo> properties;
  if (definition.get_input() == Images::Operation::Input::Image) {
    properties.insert(
        godot::PropertyInfo(
            godot::Variant::OBJECT, "image", godot::PROPERTY_HINT_RESOURCE_TYPE,
            "TtxImage"));
  } else if (definition.get_input() == Images::Operation::Input::Kernel) {
    properties.insert(
        godot::PropertyInfo(godot::Variant::PACKED_FLOAT32_ARRAY, "weights"));
    properties.insert(godot::PropertyInfo(godot::Variant::INT, "width"));
    properties.insert(godot::PropertyInfo(godot::Variant::INT, "height"));
  }

  Memory::Dynamic::Vector<GDExtensionPropertyInfo> arguments;
  Memory::Dynamic::Vector<GDExtensionClassMethodArgumentMetadata> metadata;
  for (Count index = 0; index < properties.get_size(); ++index) {
    const auto& property = properties[index];
    arguments.insert(
        {static_cast<GDExtensionVariantType>(property.type),
         property.name._native_ptr(), property.class_name._native_ptr(),
         property.hint, property.hint_string._native_ptr(), property.usage});
    metadata.insert(GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE);
  }

  godot::StringName empty;
  godot::String hint("TtxImage");
  GDExtensionPropertyInfo returned = {
    GDEXTENSION_VARIANT_TYPE_OBJECT,
    empty._native_ptr(),
    class_name._native_ptr(),
    godot::PROPERTY_HINT_RESOURCE_TYPE,
    hint._native_ptr(),
    godot::PROPERTY_USAGE_DEFAULT};
  GDExtensionClassMethodInfo method = {};
  method.name = name._native_ptr();
  method.method_userdata = const_cast<Images::Operation*>(&definition);
  method.call_func = invoke;
  method.method_flags =
      GDEXTENSION_METHOD_FLAG_NORMAL | GDEXTENSION_METHOD_FLAG_VARARG;
  method.has_return_value = true;
  method.return_value_info = &returned;
  method.argument_count = arguments.get_size();
  method.arguments_info = arguments.get_access().get_data();
  method.arguments_metadata = metadata.get_access().get_data();
  godot::internal::
      gdextension_interface_classdb_register_extension_class_method(
          godot::internal::library, class_name._native_ptr(), &method);
}

void Extension::TtxImage::_bind_methods() {
  godot::ClassDB::bind_method(
      godot::D_METHOD("set_provider", "provider"), &TtxImage::set_provider);
  godot::ClassDB::bind_method(
      godot::D_METHOD("get_provider"), &TtxImage::get_provider);
  godot::ClassDB::bind_method(
      godot::D_METHOD("set_provider_object", "object"),
      &TtxImage::set_provider_object);
  godot::ClassDB::bind_method(
      godot::D_METHOD("get_provider_object"), &TtxImage::get_provider_object);
  ADD_PROPERTY(
      godot::PropertyInfo(godot::Variant::STRING, "provider"), "set_provider",
      "get_provider");
  godot::ClassDB::bind_method(
      godot::D_METHOD("set_rgba8", "width", "height", "pixels"),
      &TtxImage::set_rgba8);
  godot::ClassDB::bind_method(godot::D_METHOD("snapshot"), &TtxImage::snapshot);
  godot::ClassDB::bind_method(
      godot::D_METHOD("read_pixels"), &TtxImage::read_pixels);
  godot::ClassDB::bind_method(
      godot::D_METHOD("get_width"), &TtxImage::get_width);
  godot::ClassDB::bind_method(
      godot::D_METHOD("get_height"), &TtxImage::get_height);
  godot::ClassDB::bind_method(
      godot::D_METHOD("get_error"), &TtxImage::get_error);
  godot::ClassDB::bind_method(
      godot::D_METHOD("get_provider_statistics"),
      &TtxImage::get_provider_statistics);
  for (const auto* operation :
       Operations::Standard::get_vocabulary().get_operations()) {
    bind_operation(*operation, call_operation);
  }
}

Extension::TtxImage::~TtxImage() {
  if (expression) {
    expression->release();
  }

  if (supply) {
    supply->release();
  }
}

void Extension::TtxImage::set_provider(const godot::String& value) {
  if (Images::Observation::active()) {
    error = "Provider selection cannot change during image observation";
    return;
  }

  const auto selected = value == "default" ? godot::String() : value;
  if (selected == provider && provider_object.is_null()) {
    return;
  }

  Images::Observation observation;
  auto previous_object = provider_object;
  auto* previous = supply;
  supply = nullptr;
  provider_object.unref();
  provider = selected;
  if (previous) {
    previous->release();
  }
}

void Extension::TtxImage::set_provider_object(
    const godot::Ref<godot::RefCounted>& object) {
  if (Images::Observation::active()) {
    error = "Provider selection cannot change during image observation";
    return;
  }

  Images::Observation observation;
  auto previous_object = provider_object;
  auto* previous = supply;
  supply = nullptr;
  provider_object = object;
  provider = godot::String();
  if (previous) {
    previous->release();
  }
}

auto Extension::TtxImage::get_provider_object() const
    -> godot::Ref<godot::RefCounted> {
  return provider_object;
}

auto Extension::TtxImage::get_provider() const -> godot::String {
  if (provider_object.is_valid()) {
    return "object";
  }

  return provider.is_empty() ? godot::String("default") : provider;
}

auto Extension::TtxImage::get_error() const -> godot::String {
  return error;
}

auto Extension::TtxImage::open_provider(Memory::Allocator::Arena& errors)
    -> bool {
  if (supply) {
    return true;
  }

  Images::Observation observation;
  const auto accept = [&](image_provider factory) {
    supply = &Images::Provider::adopt(
        factory, Operations::Standard::get_vocabulary());
    return true;
  };

  const auto reject = [&](Core::View::Bytes message) {
    error = host_string(message);
    return false;
  };

  // An existing script factory already has a Godot lifetime. Admission retains
  // that object without manufacturing a shared library or bypassing the normal
  // image contracts. Paths remain a source configuration convenience.
  if (provider_object.is_valid()) {
    return Providers::Gdscript::Factory::create(provider_object)
        .visit(accept, reject);
  }

  auto selected = provider;
  if (selected.is_empty()) {
    selected =
        godot::OS::get_singleton()->get_environment("TTX_IMAGE_PROVIDER");
  }

  if (selected.is_empty()) {
    selected = "cpu";
  }

  if (selected == "gdscript") {
    selected = "res://addons/godot_ttx/gdscript/provider.gd";
  }

  if (selected.ends_with(".gd") || selected.ends_with(".gdc")) {
    return Providers::Gdscript::Factory::open(selected, errors)
        .visit(accept, reject);
  }

  godot::String path;
  godot::internal::gdextension_interface_get_library_path(
      godot::internal::library, path._native_ptr());
  path = selected.contains("/")
             ? selected
             : path.get_base_dir().path_join("lib" + selected + "_provider.so");
  const auto encoded =
      godot::ProjectSettings::get_singleton()->globalize_path(path).utf8();
  auto result = Images::Provider::open(
      {reinterpret_cast<const U8*>(encoded.get_data()),
       Count(encoded.length())},
      Operations::Standard::get_vocabulary(), errors);
  return result.visit(
      [&](Images::Provider& provider) {
        supply = &provider;
        return true;
      },
      [&](Core::View::Bytes message) {
        error = host_string(message);
        return false;
      });
}

auto Extension::TtxImage::set_rgba8(
    int64_t w,
    int64_t h,
    const godot::PackedByteArray& pixels) -> bool {
  if (Images::Observation::active()) {
    error = "Source publication is not allowed during image observation";
    return false;
  }

  error = godot::String();
  if (expression && !source) {
    error = "Operation results cannot publish source pixels";
    return false;
  }

  if (w <= 0 || h <= 0 || w > 4096 || h > 4096) {
    error = "Image dimensions must be positive and at most 4096";
    return false;
  }

  const Core::View::Bytes bytes(pixels.ptr(), pixels.size());
  const auto invalid = Contracts::Pixels::validate(w, h, bytes);
  if (!invalid.is_empty()) {
    error = host_string(invalid);
    return false;
  }

  Memory::Allocator::Arena errors;
  if (!open_provider(errors)) {
    return false;
  }

  auto result = Images::Image::create(*supply, w, h, bytes, errors);
  return result.visit(
      [&](Images::Image& image) {
        if (source) {
          if (!source->publish(Core::Data::take(image))) {
            error =
                "Source publication is not allowed during image observation";
            return false;
          }
        } else {
          source = &Images::Source::create(Core::Data::take(image));
          expression = source;
        }

        emit_changed();
        return true;
      },
      [&](Core::View::Bytes message) {
        error = host_string(message);
        return false;
      });
}

auto Extension::TtxImage::evaluate(Memory::Allocator::Arena& errors)
    -> const Images::Image* {
  error = godot::String();
  if (!expression) {
    error = "Image has no source pixels";
    return nullptr;
  }

  auto result = expression->evaluate(errors);
  return result.visit(
      [](const Images::Image& image) { return &image; },
      [&](Core::View::Bytes message) -> const Images::Image* {
        error = host_string(message);
        return nullptr;
      });
}

auto Extension::TtxImage::get_provider_statistics() -> godot::Dictionary {
  Images::Observation observation;
  Memory::Allocator::Arena errors;
  const auto* image = evaluate(errors);
  godot::Dictionary result;
  if (!image) {
    return result;
  }

  const auto statistics = image->get_provider().statistics();
  result["uploads"] = int64_t(statistics.uploads);
  result["downloads"] = int64_t(statistics.downloads);
  result["live_images"] = int64_t(statistics.live_images);
  result["plan_builds"] = int64_t(statistics.plan_builds);
  return result;
}

auto Extension::TtxImage::get_width() -> int64_t {
  Memory::Allocator::Arena errors;
  const auto image = evaluate(errors);
  return image ? image->get_width() : 0;
}

auto Extension::TtxImage::get_height() -> int64_t {
  Memory::Allocator::Arena errors;
  const auto image = evaluate(errors);
  return image ? image->get_height() : 0;
}

// A snapshot starts an independent source from the current value. It keeps
// history explicitly while ordinary operation results follow their
// dependencies.
auto Extension::TtxImage::snapshot() -> godot::Ref<TtxImage> {
  Memory::Allocator::Arena errors;
  const auto image = evaluate(errors);
  if (!image) {
    return {};
  }

  godot::Ref<TtxImage> output;
  output.instantiate();
  output->source = &Images::Source::create(*image);
  output->expression = output->source;
  output->provider = provider;
  output->provider_object = provider_object;
  return output;
}

auto Extension::TtxImage::read_pixels() -> godot::PackedByteArray {
  Memory::Allocator::Arena errors;
  const auto image = evaluate(errors);
  if (!image) {
    return {};
  }

  auto result = image->read_pixels(errors);
  return result.visit(
      [](Memory::Dynamic::Bytes& pixels) {
        godot::PackedByteArray output;
        output.resize(pixels.get_size());
        Core::Data::copy(
            output.ptrw(), pixels.get_view().get_data(), pixels.get_size());
        return output;
      },
      [&](Core::View::Bytes message) -> godot::PackedByteArray {
        error = host_string(message);
        return {};
      });
}

auto Extension::TtxImage::apply(
    const Images::Operation& operation,
    const GDExtensionConstVariantPtr* values,
    GDExtensionInt count) -> godot::Ref<TtxImage> {
  error = godot::String();
  if (!expression) {
    error = "Image has no source pixels";
    return {};
  }

  // Godot's Variants live only for this call. Retain image dependencies by
  // their native nodes and copy kernel coefficients into the Call's Arena,
  // so later invalidation can reuse the authored inputs after this stack ends.
  Core::Option<Memory::Allocator::Arena> constants;
  Memory::Dynamic::Vector<Images::Call::Argument> arguments;
  GDExtensionInt index = 0;
  if (operation.get_input() == Images::Operation::Input::Image) {
    if (index >= count) {
      error = "Image argument is missing";
      return {};
    }

    const godot::Variant value(values[index++]);
    const godot::Ref<TtxImage> input = value;
    if (input.is_null() || !input->expression) {
      error = "Image argument has no source pixels";
      return {};
    }

    arguments.insert({input->expression, nullptr});
  } else if (operation.get_input() == Images::Operation::Input::Kernel) {
    if (index + 3 > count) {
      error = "Kernel requires weights, width and height";
      return {};
    }

    const godot::Variant coefficients(values[index++]), width(values[index++]),
        height(values[index++]);
    if (coefficients.get_type() != godot::Variant::PACKED_FLOAT32_ARRAY ||
        width.get_type() != godot::Variant::INT ||
        height.get_type() != godot::Variant::INT) {
      error = "Kernel requires float32 weights and integer dimensions";
      return {};
    }

    const godot::PackedFloat32Array weights = coefficients;
    const int64_t w = width, h = height;
    if (w <= 0 || h <= 0 || w > 1023 || h > 1023) {
      error = "Kernel dimensions must be positive and at most 1023";
      return {};
    }

    const auto invalid = Contracts::Kernel::validate(
        w, h, {weights.ptr(), Count(weights.size())});
    if (!invalid.is_empty()) {
      error = host_string(invalid);
      return {};
    }

    if (!constants) {
      constants = Memory::Allocator::Arena();
    }

    const auto stored = constants->proxy(
        {reinterpret_cast<const U8*>(weights.ptr()),
         Count(weights.size()) * sizeof(R32)});
    const auto& kernel = constants->construct<Images::Kernel>(
        w, h,
        Core::View::Vector<R32>(
            reinterpret_cast<const R32*>(stored.get_data()), weights.size()));
    arguments.insert({nullptr, &kernel});
  }

  if (index != count) {
    error = "Unexpected image operation arguments";
    return {};
  }

  // Evaluate before exposing a Resource, preserving the existing script
  // contract that invalid inputs return no image. The retained node then
  // answers later observations and refreshes only when dependencies change.
  auto& call = Images::Call::create(
      *expression, operation, arguments.get_view(),
      Core::Data::take(constants));
  Memory::Allocator::Arena errors;
  auto result = call.evaluate(errors);
  const bool success = result.visit(
      [](const Images::Image&) { return true; },
      [&](Core::View::Bytes message) {
        error = host_string(message);
        return false;
      });
  if (!success) {
    call.release();
    return {};
  }

  godot::Ref<TtxImage> output;
  output.instantiate();
  output->expression = &call;
  output->provider = provider;
  output->provider_object = provider_object;
  return output;
}

void Extension::TtxImage::call_operation(
    void* definition,
    GDExtensionClassInstancePtr instance,
    const GDExtensionConstVariantPtr* arguments,
    GDExtensionInt count,
    GDExtensionVariantPtr returned,
    GDExtensionCallError* error) {
  error->error = GDEXTENSION_CALL_OK;
  auto& resource = *reinterpret_cast<TtxImage*>(instance);
  const auto output = resource.apply(
      *static_cast<const Images::Operation*>(definition), arguments, count);

  // Godot supplies the return slot. Construct its object Variant there while
  // the Ref still owns the Resource, so that Variant acquires its own lifetime
  // without first copying and then destroying a temporary Variant.
  static const auto construct =
      godot::internal::gdextension_interface_get_variant_from_type_constructor(
          GDEXTENSION_VARIANT_TYPE_OBJECT);
  GDExtensionObjectPtr object = output.is_valid() ? output->_owner : nullptr;
  construct(returned, &object);
}
