// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "toolchain/memory/lifetime.hpp"

#include "perimortem/core/view/null_terminated.hpp"

#include "ttx/graph/abstract.hpp"
#include "ttx/graph/capabilities/retain.h"
#include "ttx/graph/policies/none.h"

#include "demo/imaging/contracts/offers.h"
#include "demo/imaging/cpu/image.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;
using namespace Godot::Demo::Imaging::Cpu;
namespace Form = Ttx::Data::Form;

static image_provider_statistics statistics{};

static auto same(perimortem_uuid id, U64 high, U64 low) -> bool {
  return id.high == high && id.low == low;
}

static auto failure(Core::View::Bytes message) -> image_error {
  return {message.get_data(), message.get_size()};
}

static auto graph(void* context) -> ttx_abstract {
  return Core::Data::cast<Image>(context)->graph();
}

static auto release(void* context) -> ttx_release_status {
  return Core::Data::cast<Image>(context)->release();
}

static const ttx_retained_ops retained_operations{graph, release};

Image::Image(U32 width, U32 height, Memory::Global::Bytes pixels)
    : width(width), height(height), pixels(Core::Data::take(pixels)) {
  ++statistics.live_images;
}

Image::~Image() {
  --statistics.live_images;
}

auto Image::create(U32 width, U32 height, Memory::Global::Bytes pixels)
    -> ttx_retained {
  auto allocation = Memory::Allocator::Global::allocate(sizeof(Image));
  auto& image = Toolchain::Memory::Lifetime::construct<Image>(
      allocation.get_data(),
      [&] { return Image(width, height, Core::Data::take(pixels)); });
  return {&image, &retained_operations};
}

auto Image::acquire() -> ttx_retained {
  ++references;
  return {this, &retained_operations};
}

auto Image::release() -> ttx_release_status {
  if (--references == 0) {
    this->~Image();
    Memory::Allocator::Global::release(Core::Data::cast<U8>(this));
  }

  return TTX_RELEASE_SUCCESS;
}

static auto retain(void* context, ttx_retained* output)
    -> ttx_negotiation_status {
  *output = Core::Data::cast<Image>(context)->acquire();
  return TTX_NEGOTIATION_SATISFIED;
}

static auto dimensions(void* context) -> compute_image_dimensions {
  auto& image = *Core::Data::cast<Image>(context);
  return {image.get_width(), image.get_height()};
}

static auto read(void* context, U8* output, Count capacity)
    -> compute_image_status {
  const auto pixels = Core::Data::cast<Image>(context)->get_pixels();
  if (!output || capacity < pixels.get_size()) {
    return COMPUTE_IMAGE_INVALID;
  }

  __builtin_memcpy(output, pixels.get_data(), pixels.get_size());
  return COMPUTE_IMAGE_SUCCESS;
}

static auto invert(void* context, ttx_retained* output) -> image_error {
  auto& image = *Core::Data::cast<Image>(context);
  Memory::Global::Bytes pixels(image.get_pixels());
  for (Count i = 0; i < pixels.get_size(); i += 4) {
    for (Count c = 0; c < 3; ++c) {
      pixels.get_data()[i + c] ^= 255;
    }
  }

  *output = Image::create(
      image.get_width(), image.get_height(), Core::Data::take(pixels));
  return {};
}

static auto convolution(
    void* context,
    image_kernel kernel,
    ttx_retained* output) -> image_error {
  if (!kernel.values || !kernel.width || !kernel.height ||
      kernel.width > 1023 || kernel.height > 1023 || !(kernel.width & 1) ||
      !(kernel.height & 1) ||
      kernel.count != Count(kernel.width) * kernel.height) {
    return failure(
        "Kernel dimensions must be odd and match the coefficient count."_view);
  }

  for (Count i = 0; i < kernel.count; ++i) {
    if (!__builtin_isfinite(kernel.values[i]) ||
        __builtin_fabsf(kernel.values[i]) > 1000000) {
      return failure(
          "Kernel coefficients must be finite with magnitude at most one million."_view);
    }
  }

  auto& image = *Core::Data::cast<Image>(context);
  return convolve(image, kernel)
      .visit(
          [&](Memory::Global::Bytes& pixels) -> image_error {
            *output = Image::create(
                image.get_width(), image.get_height(),
                Core::Data::take(pixels));
            return {};
          },

          [](Core::View::Bytes message) { return failure(message); });
}

static auto composite(void* context, ttx_abstract overlay, ttx_retained* output)
    -> image_error {
  auto& image = *Core::Data::cast<Image>(context);
  compute_pixels api{};
  const perimortem_uuid id{COMPUTE_PIXELS_ID_HIGH, COMPUTE_PIXELS_ID_LOW};
  if (overlay.operations->bind(overlay.context, id, &api) !=
      TTX_NEGOTIATION_SATISFIED) {
    return failure("Overlay does not supply RGBA8 Pixels."_view);
  }

  const auto size = api.operations->dimensions(api.context);
  if (size.width != image.get_width() || size.height != image.get_height()) {
    return failure("Overlay dimensions must match the source."_view);
  }

  Memory::Global::Bytes foreground;
  foreground.forgetful_resize(image.get_pixels().get_size());
  if (api.operations->read(
          api.context, foreground.get_data(), foreground.get_size()) !=
      COMPUTE_IMAGE_SUCCESS) {
    return failure("Overlay pixels could not be read."_view);
  }

  Memory::Global::Bytes pixels(image.get_pixels());
  for (Count i = 0; i < pixels.get_size(); i += 4) {
    const U32 front = foreground.get_data()[i + 3],
              back = pixels.get_data()[i + 3];
    const U32 alpha = front * 255 + back * (255 - front);
    for (Count c = 0; c < 3; ++c) {
      const U32 color = foreground.get_data()[i + c] * front * 255 +
                        pixels.get_data()[i + c] * back * (255 - front);
      pixels.get_data()[i + c] = alpha ? (color + alpha / 2) / alpha : 0;
    }

    pixels.get_data()[i + 3] = (alpha + 127) / 255;
  }

  *output = Image::create(
      image.get_width(), image.get_height(), Core::Data::take(pixels));
  return {};
}

static auto offers(void* context, image_offer_visitor visitor) -> void {
  const image_offer values[]{
    {{GODOT_IMAGE_INVERT_ID_HIGH, GODOT_IMAGE_INVERT_ID_LOW},
     "invert"_view,
     IMAGE_INPUT_NONE,
     0,
     0,
     0},

    {{GODOT_IMAGE_CONVOLVE_ID_HIGH, GODOT_IMAGE_CONVOLVE_ID_LOW},
     "convolve"_view,
     IMAGE_INPUT_KERNEL,
     1,
     1023,
     2},

    {{GODOT_IMAGE_COMPOSITE_ID_HIGH, GODOT_IMAGE_COMPOSITE_ID_LOW},
     "composite"_view,
     IMAGE_INPUT_IMAGE,
     0,
     0,
     0}};

  for (const auto& offer : values) {
    visitor.callback(visitor.callback_context, offer);
  }
}

static auto admission(void* context, perimortem_uuid id, U32 width, U32 height)
    -> image_admission {
  if (same(id, GODOT_IMAGE_CONVOLVE_ID_HIGH, GODOT_IMAGE_CONVOLVE_ID_LOW)) {
    if (!width || !height || width > 1023 || height > 1023 || !(width & 1) ||
        !(height & 1)) {
      return {
        TTX_NEGOTIATION_REJECTED,
        "Kernel dimensions must be odd and between one and 1023."_view};
    }
  } else if (
      !same(id, GODOT_IMAGE_INVERT_ID_HIGH, GODOT_IMAGE_INVERT_ID_LOW) &&
      !same(id, GODOT_IMAGE_COMPOSITE_ID_HIGH, GODOT_IMAGE_COMPOSITE_ID_LOW)) {
    return {TTX_NEGOTIATION_UNKNOWN, "Operation is not offered."_view};
  }

  return {TTX_NEGOTIATION_SATISFIED, {}};
}

static auto supports(void* context, perimortem_uuid id)
    -> ttx_negotiation_status {
  if (id.high == 0 && id.low <= 1) {
    return TTX_NEGOTIATION_REJECTED;
  }

  if (same(id, TTX_ABSTRACT_ID_HIGH, TTX_ABSTRACT_ID_LOW) ||
      same(id, TTX_RETAIN_ID_HIGH, TTX_RETAIN_ID_LOW) ||
      same(id, COMPUTE_PIXELS_ID_HIGH, COMPUTE_PIXELS_ID_LOW) ||
      same(id, GODOT_IMAGE_INVERT_ID_HIGH, GODOT_IMAGE_INVERT_ID_LOW) ||
      same(id, GODOT_IMAGE_CONVOLVE_ID_HIGH, GODOT_IMAGE_CONVOLVE_ID_LOW) ||
      same(id, GODOT_IMAGE_COMPOSITE_ID_HIGH, GODOT_IMAGE_COMPOSITE_ID_LOW) ||
      same(id, GODOT_IMAGE_OFFERS_ID_HIGH, GODOT_IMAGE_OFFERS_ID_LOW)) {
    return TTX_NEGOTIATION_SATISFIED;
  }

  return TTX_NEGOTIATION_UNKNOWN;
}

static auto bind(void* context, perimortem_uuid id, void* output)
    -> ttx_negotiation_status {
  const auto status = supports(context, id);
  if (status != TTX_NEGOTIATION_SATISFIED) {
    return status;
  }

  static const ttx_retain_ops retain_ops{graph, retain};
  static const compute_pixels_ops pixels_ops{graph, dimensions, read};
  static const image_invert_ops invert_ops{graph, invert};
  static const image_convolve_ops convolve_ops{graph, convolution};
  static const image_composite_ops composite_ops{graph, composite};
  static const image_offers_ops offers_ops{graph, offers, admission};
  if (same(id, TTX_RETAIN_ID_HIGH, TTX_RETAIN_ID_LOW)) {
    *Core::Data::cast<ttx_retain>(output) = {context, &retain_ops};
  } else if (same(id, COMPUTE_PIXELS_ID_HIGH, COMPUTE_PIXELS_ID_LOW)) {
    *Core::Data::cast<compute_pixels>(output) = {context, &pixels_ops};
  } else if (same(id, GODOT_IMAGE_INVERT_ID_HIGH, GODOT_IMAGE_INVERT_ID_LOW)) {
    *Core::Data::cast<image_invert>(output) = {context, &invert_ops};
  } else if (
      same(id, GODOT_IMAGE_CONVOLVE_ID_HIGH, GODOT_IMAGE_CONVOLVE_ID_LOW)) {
    *Core::Data::cast<image_convolve>(output) = {context, &convolve_ops};
  } else if (
      same(id, GODOT_IMAGE_COMPOSITE_ID_HIGH, GODOT_IMAGE_COMPOSITE_ID_LOW)) {
    *Core::Data::cast<image_composite>(output) = {context, &composite_ops};
  } else if (same(id, GODOT_IMAGE_OFFERS_ID_HIGH, GODOT_IMAGE_OFFERS_ID_LOW)) {
    *Core::Data::cast<image_offers>(output) = {context, &offers_ops};
  } else {
    *Core::Data::cast<ttx_abstract>(output) = graph(context);
  }

  return status;
}

static auto route(void* context, perimortem_view_bytes path) -> ttx_abstract {
  return ttx_none();
}

static auto visit(void* context, ttx_graph_visitor visitor) -> void {}
static auto state(void* context) -> ttx_abstract_state {
  return TTX_ABSTRACT_PRESENT;
}

auto Image::graph() -> ttx_abstract {
  static const ttx_abstract_ops operations{supports, bind, route, visit, state};
  return {this, &operations};
}

static auto factory_graph(void* context) -> ttx_abstract;
static auto provider_statistics(void* context) -> image_provider_statistics {
  return statistics;
}

static auto create(
    void* context,
    U32 width,
    U32 height,
    const U8* pixels,
    Count size,
    ttx_retained* output) -> image_error {
  if (!width || !height || width > 4096 || height > 4096 || !pixels ||
      size != Count(width) * height * 4) {
    return failure("RGBA8 dimensions and pixel count must agree."_view);
  }

  *output = Image::create(
      width, height, Memory::Global::Bytes(Core::View::Bytes(*pixels, size)));
  return {};
}

static auto factory_supports(void* context, perimortem_uuid id)
    -> ttx_negotiation_status {
  if (id.high == 0 && id.low <= 1) {
    return TTX_NEGOTIATION_REJECTED;
  }

  return same(id, GODOT_IMAGE_PROVIDER_ID_HIGH, GODOT_IMAGE_PROVIDER_ID_LOW) ||
                 same(id, TTX_ABSTRACT_ID_HIGH, TTX_ABSTRACT_ID_LOW)
             ? TTX_NEGOTIATION_SATISFIED
             : TTX_NEGOTIATION_UNKNOWN;
}

static auto factory_bind(void* context, perimortem_uuid id, void* output)
    -> ttx_negotiation_status {
  const auto status = factory_supports(context, id);
  if (status != TTX_NEGOTIATION_SATISFIED) {
    return status;
  }

  static const image_provider_ops operations{
    factory_graph, provider_statistics, create};

  if (same(id, GODOT_IMAGE_PROVIDER_ID_HIGH, GODOT_IMAGE_PROVIDER_ID_LOW)) {
    *Core::Data::cast<image_provider>(output) = {context, &operations};
  } else {
    *Core::Data::cast<ttx_abstract>(output) = factory_graph(context);
  }

  return status;
}

static auto factory_graph(void* context) -> ttx_abstract {
  static const ttx_abstract_ops operations{
    factory_supports, factory_bind, route, visit, state};

  return {context, &operations};
}

C_LINKAGE HIDDEN constinit const ttx_representation image_query_representation =
    Form::Compiled<
        Form::Native<ttx_semantic_query>::reference>::get_representation()
        .get_abi();

C_LINKAGE HIDDEN auto image_factory() -> ttx_abstract {
  return factory_graph(&statistics);
}
