// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/adapters/imaging/scripts/bridge.hpp"

#include "toolchain/memory/lifetime.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/memory/allocator/global.hpp"
#include "perimortem/serialization/formats/uuid.hpp"

#include "ttx/graph/capabilities/retain.h"
#include "ttx/graph/policies/none.h"

#include "demo/imaging/contracts/offers.h"

#include "godot_cpp/variant/callable.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/variant/packed_float32_array.hpp"

using namespace Perimortem;
using namespace godot;
using namespace Godot::Demo::Adapters::Imaging::Scripts;

// ScriptImage keeps the Godot object accessible through its retained image
// acquisitions. Its published operations recover this private context and
// invoke the script while its strong Godot reference remains valid.
struct ScriptOperation;

struct ScriptImage {
  Count references = 1;
  Ref<RefCounted> object;
  U32 width;
  U32 height;
  CharString error;
  ScriptOperation* operations = nullptr;
};

struct ScriptOperation {
  ScriptImage* image;
  Callable callable;
  ScriptOperation* next;
};

static auto graph(void* context) -> ttx_abstract;

static auto operation_graph(void* context) -> ttx_abstract {
  return graph(Core::Data::cast<ScriptOperation>(context)->image);
}

static auto wrap(Ref<RefCounted> object, U32 width, U32 height) -> ttx_retained;

static auto text(perimortem_uuid id) -> String {
  const auto bytes =
      Serialization::Formats::Uuid::serialize(Core::Identity::Uuid(id));
  return String::utf8(
      Core::Data::cast<const char>(bytes.get_view().get_data()),
      bytes.get_view().get_size());
}

static auto uuid(const String& value, perimortem_uuid& output) -> bool {
  const auto bytes = value.utf8();
  const Core::View::Bytes view(
      perimortem_view_bytes{
        Core::Data::cast<const U8>(bytes.get_data()), Count(bytes.length())});

  if (view.get_size() != 36 || !Serialization::Formats::Uuid::is_valid(view)) {
    return false;
  }

  Core::Storage::Bytes<36> stored;
  __builtin_memcpy(stored.get_access().get_data(), view.get_data(), 36);
  output = Serialization::Formats::Uuid::deserialize(stored);
  return true;
}

static auto fail(ScriptImage& image, const String& error) -> image_error {
  image.error = error.utf8();
  return {
    Core::Data::cast<const U8>(image.error.get_data()),
    Count(image.error.length())};
}

static auto release(void* context) -> ttx_release_status {
  auto& image = *Core::Data::cast<ScriptImage>(context);
  if (--image.references == 0) {
    while (image.operations) {
      auto* operation = image.operations;
      image.operations = operation->next;
      operation->~ScriptOperation();
      Perimortem::Memory::Allocator::Global::release(
          Core::Data::cast<U8>(operation));
    }

    image.~ScriptImage();
    Perimortem::Memory::Allocator::Global::release(
        Core::Data::cast<U8>(&image));
  }

  return TTX_RELEASE_SUCCESS;
}

static const ttx_retained_ops lifetime{graph, release};

static auto retain(void* context, ttx_retained* output)
    -> ttx_negotiation_status {
  ++Core::Data::cast<ScriptImage>(context)->references;
  *output = {context, &lifetime};
  return TTX_NEGOTIATION_SATISFIED;
}

static auto wrap(Ref<RefCounted> object, U32 width, U32 height)
    -> ttx_retained {
  auto allocation =
      Perimortem::Memory::Allocator::Global::allocate(sizeof(ScriptImage));
  auto& image = Toolchain::Memory::Lifetime::construct<ScriptImage>(
      allocation.get_data(),
      [&] { return ScriptImage{1, object, width, height, {}, nullptr}; });
  return {&image, &lifetime};
}

static auto dimensions(void* context) -> compute_image_dimensions {
  auto& image = *Core::Data::cast<ScriptImage>(context);
  return {image.width, image.height};
}

static auto read(void* context, U8* output, Count capacity)
    -> compute_image_status {
  auto& image = *Core::Data::cast<ScriptImage>(context);
  const Count size = Count(image.width) * image.height * 4;
  if (!output || capacity < size) {
    return COMPUTE_IMAGE_INVALID;
  }

  const Variant value = image.object->call("read_pixels");
  if (value.get_type() != Variant::PACKED_BYTE_ARRAY) {
    return COMPUTE_IMAGE_FAILED;
  }

  const PackedByteArray pixels = value;
  if (Count(pixels.size()) != size) {
    return COMPUTE_IMAGE_FAILED;
  }

  __builtin_memcpy(output, pixels.ptr(), size);
  return COMPUTE_IMAGE_SUCCESS;
}

static auto complete(
    ScriptImage& image,
    const Variant& value,
    ttx_retained* output) -> image_error {
  if (value.get_type() == Variant::STRING) {
    return fail(image, value);
  }

  const Ref<RefCounted> object = value;
  if (object.is_null() || !object->has_method("read_pixels") ||
      !object->has_method("offers")) {
    return fail(image, "Script operation did not return an image.");
  }

  *output = wrap(object, image.width, image.height);
  return {};
}

static auto invert(void* context, ttx_retained* output) -> image_error {
  auto& supplied = *Core::Data::cast<ScriptOperation>(context);
  auto& image = *supplied.image;
  const Callable operation = supplied.callable;
  return complete(image, operation.call(), output);
}

static auto convolve(void* context, image_kernel kernel, ttx_retained* output)
    -> image_error {
  auto& supplied = *Core::Data::cast<ScriptOperation>(context);
  auto& image = *supplied.image;
  if (!kernel.values || !kernel.width || !kernel.height ||
      kernel.width > 1023 || kernel.height > 1023 || !(kernel.width & 1) ||
      !(kernel.height & 1) ||
      kernel.count != Count(kernel.width) * kernel.height) {
    return fail(image, "Kernel dimensions and coefficient count must agree.");
  }

  PackedFloat32Array weights;
  weights.resize(kernel.count);
  for (Count i = 0; i < kernel.count; ++i) {
    if (!__builtin_isfinite(kernel.values[i]) ||
        __builtin_fabsf(kernel.values[i]) > 1000000) {
      return fail(image, "Kernel coefficient is outside the supported range.");
    }

    weights.set(i, kernel.values[i]);
  }

  const Callable operation = supplied.callable;
  return complete(
      image, operation.call(weights, kernel.width, kernel.height), output);
}

static auto composite(void* context, ttx_abstract overlay, ttx_retained* output)
    -> image_error {
  auto& supplied = *Core::Data::cast<ScriptOperation>(context);
  auto& image = *supplied.image;
  const Callable operation = supplied.callable;
  compute_pixels api{};
  if (overlay.operations->bind(
          overlay.context, {COMPUTE_PIXELS_ID_HIGH, COMPUTE_PIXELS_ID_LOW},
          &api) != TTX_NEGOTIATION_SATISFIED) {
    return fail(image, "Overlay does not provide Pixels.");
  }

  const auto extent = api.operations->dimensions(api.context);
  if (extent.width != image.width || extent.height != image.height) {
    return fail(image, "Overlay dimensions must match the source.");
  }

  PackedByteArray pixels;
  pixels.resize(Count(image.width) * image.height * 4);
  if (api.operations->read(api.context, pixels.ptrw(), pixels.size()) !=
      COMPUTE_IMAGE_SUCCESS) {
    return fail(image, "Overlay pixels could not be read.");
  }

  return complete(image, operation.call(pixels), output);
}

static auto offers(void* context, image_offer_visitor visitor) -> void {
  auto& image = *Core::Data::cast<ScriptImage>(context);
  const Variant result = image.object->call("offers");
  if (result.get_type() != Variant::ARRAY) {
    return;
  }

  const Array values = result;
  for (GDExtensionInt i = 0; i < values.size(); ++i) {
    if (values[i].get_type() != Variant::DICTIONARY) {
      continue;
    }

    const Dictionary value = values[i];
    perimortem_uuid id{};
    if (!uuid(value.get("contract", ""), id)) {
      continue;
    }

    const GDExtensionInt input = value.get("input", -1);
    const GDExtensionInt minimum = value.get("minimum", 0),
              maximum = value.get("maximum", 0), step = value.get("step", 0);
    if (input < 0 || input > 2 || minimum < 0 || maximum < 0 || step < 0 ||
        minimum > 1023 || maximum > 1023 || step > 1023) {
      continue;
    }

    const auto name = String(value.get("name", "")).utf8();
    const image_offer offered{
      id,
      {Core::Data::cast<const U8>(name.get_data()), Count(name.length())},
      U8(input),
      U32(minimum),
      U32(maximum),
      U32(step)};

    visitor.callback(visitor.callback_context, offered);
  }
}

static auto admit(void* context, perimortem_uuid id, U32 width, U32 height)
    -> image_admission {
  auto& image = *Core::Data::cast<ScriptImage>(context);
  const Variant result = image.object->call("admit", text(id), width, height);
  if (result.get_type() != Variant::DICTIONARY) {
    const auto error =
        fail(image, "Script admission must return a dictionary.");
    return {TTX_NEGOTIATION_REJECTED, {error.data, error.size}};
  }

  const Dictionary value = result;
  const GDExtensionInt status = value.get("status", 1);
  const auto error = fail(image, value.get("reason", ""));
  return {
    status == 0   ? TTX_NEGOTIATION_SATISFIED
    : status == 1 ? TTX_NEGOTIATION_UNKNOWN
                  : TTX_NEGOTIATION_REJECTED,
    {error.data, error.size}};
}

static auto same(perimortem_uuid id, U64 high, U64 low) -> bool {
  return id.high == high && id.low == low;
}

static auto supports(void* context, perimortem_uuid id)
    -> ttx_negotiation_status {
  if (!id.high && id.low <= 1) {
    return TTX_NEGOTIATION_REJECTED;
  }

  if (same(id, TTX_ABSTRACT_ID_HIGH, TTX_ABSTRACT_ID_LOW) ||
      same(id, TTX_RETAIN_ID_HIGH, TTX_RETAIN_ID_LOW) ||
      same(id, COMPUTE_PIXELS_ID_HIGH, COMPUTE_PIXELS_ID_LOW) ||
      same(id, GODOT_IMAGE_OFFERS_ID_HIGH, GODOT_IMAGE_OFFERS_ID_LOW)) {
    return TTX_NEGOTIATION_SATISFIED;
  }

  auto& image = *Core::Data::cast<ScriptImage>(context);
  const Variant status = image.object->call("supports", text(id));
  return status.get_type() == Variant::INT && GDExtensionInt(status) == 0
             ? TTX_NEGOTIATION_SATISFIED
             : TTX_NEGOTIATION_UNKNOWN;
}

static auto bind(void* context, perimortem_uuid id, void* output)
    -> ttx_negotiation_status {
  const auto status = supports(context, id);
  if (status != TTX_NEGOTIATION_SATISFIED) {
    return status;
  }

  static const ttx_retain_ops retention{graph, retain};
  static const compute_pixels_ops pixels{graph, dimensions, read};
  static const image_offers_ops offered{graph, offers, admit};
  static const image_invert_ops invert_ops{operation_graph, invert};
  static const image_convolve_ops convolve_ops{operation_graph, convolve};
  static const image_composite_ops composite_ops{operation_graph, composite};
  if (same(id, TTX_ABSTRACT_ID_HIGH, TTX_ABSTRACT_ID_LOW)) {
    *Core::Data::cast<ttx_abstract>(output) = graph(context);
  } else if (same(id, TTX_RETAIN_ID_HIGH, TTX_RETAIN_ID_LOW)) {
    *Core::Data::cast<ttx_retain>(output) = {context, &retention};
  } else if (same(id, COMPUTE_PIXELS_ID_HIGH, COMPUTE_PIXELS_ID_LOW)) {
    *Core::Data::cast<compute_pixels>(output) = {context, &pixels};
  } else if (same(id, GODOT_IMAGE_OFFERS_ID_HIGH, GODOT_IMAGE_OFFERS_ID_LOW)) {
    *Core::Data::cast<image_offers>(output) = {context, &offered};
  } else {
    auto& image = *Core::Data::cast<ScriptImage>(context);
    struct Selection {
      perimortem_uuid id;
      int input = -1;
    } selection{id};

    offers(
        context,
        {&selection, [](void* context, image_offer offer) {
           auto& selected = *Core::Data::cast<Selection>(context);
           if (same(offer.contract, selected.id.high, selected.id.low)) {
             selected.input = offer.input;
           }
         }});

    const Variant supplied = image.object->call("fulfill", text(id));
    if (selection.input < 0 || supplied.get_type() != Variant::CALLABLE ||
        !Callable(supplied).is_valid()) {
      return TTX_NEGOTIATION_REJECTED;
    }

    auto allocation = Perimortem::Memory::Allocator::Global::allocate(
        sizeof(ScriptOperation));
    auto& operation = Toolchain::Memory::Lifetime::construct<ScriptOperation>(
        allocation.get_data(), [&] {
          return ScriptOperation{&image, Callable(supplied), image.operations};
        });

    image.operations = &operation;
    if (selection.input == 0) {
      *Core::Data::cast<image_invert>(output) = {&operation, &invert_ops};
    } else if (selection.input == 1) {
      *Core::Data::cast<image_convolve>(output) = {&operation, &convolve_ops};
    } else {
      *Core::Data::cast<image_composite>(output) = {&operation, &composite_ops};
    }
  }

  return status;
}

static auto route(void* context, perimortem_view_bytes path) -> ttx_abstract {
  return ttx_none();
}

static auto visit(void* context, ttx_graph_visitor callback) -> void {}
static auto state(void* context) -> ttx_abstract_state {
  return TTX_ABSTRACT_PRESENT;
}

static auto graph(void* context) -> ttx_abstract {
  static const ttx_abstract_ops operations{supports, bind, route, visit, state};
  return {context, &operations};
}

auto Bridge::create(
    const Ref<RefCounted>& factory,
    U32 width,
    U32 height,
    const PackedByteArray& pixels,
    ttx_retained& output,
    String& error) -> bool {
  if (factory.is_null() || !factory->has_method("create_image")) {
    error = "Script factory requires create_image.";
    return false;
  }

  const Variant value = factory->call("create_image", width, height, pixels);
  const Ref<RefCounted> image = value;
  if (image.is_null() || !image->has_method("read_pixels") ||
      !image->has_method("supports") || !image->has_method("fulfill") ||
      !image->has_method("offers") || !image->has_method("admit")) {
    error = value.get_type() == Variant::STRING
                ? String(value)
                : String("Script factory did not return an image.");
    return false;
  }

  output = wrap(image, width, height);
  return true;
}
