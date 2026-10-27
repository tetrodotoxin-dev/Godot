// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/adapters/imaging/ttx_image.hpp"

#include "toolchain/memory/lifetime.hpp"

#include "perimortem/core/view/null_terminated.hpp"
#include "perimortem/memory/allocator/global.hpp"
#include "perimortem/serialization/formats/uuid.hpp"
#include "perimortem/system/process/library.hpp"

#include "ttx/data/transfer.h"
#include "ttx/graph/abstract.hpp"
#include "ttx/graph/policies/none.h"
#include "ttx/semantic/negotiation/library.h"

#include "demo/adapters/imaging/scripts/bridge.hpp"
#include "demo/imaging/contracts/offers.h"

#include "godot_cpp/classes/os.hpp"
#include "godot_cpp/classes/project_settings.hpp"
#include "godot_cpp/classes/resource_loader.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

using namespace Perimortem;
using namespace godot;
using namespace Godot::Demo::Adapters::Imaging;

static thread_local Count evaluating = 0;

// Evaluation can call a script provider, including its destruction callbacks.
// Keep source publication excluded until the entire observation has returned.
class Evaluation {
 public:
  Evaluation() { ++evaluating; }
  ~Evaluation() { --evaluating; }
};

static auto text(perimortem_view_bytes bytes) -> String {
  return String::utf8(Core::Data::cast<const char>(bytes.data), bytes.size);
}

static auto text(image_error error) -> String {
  return text(perimortem_view_bytes{error.data, error.size});
}

static auto identifier(perimortem_uuid id) -> String {
  const auto bytes =
      Serialization::Formats::Uuid::serialize(Core::Identity::Uuid(id));
  return text(bytes.get_view());
}

static auto parse(const String& value) -> perimortem_uuid {
  const auto bytes = value.utf8();
  const Core::View::Bytes view(
      perimortem_view_bytes{
        Core::Data::cast<const U8>(bytes.get_data()), Count(bytes.length())});

  if (view.get_size() != 36 || !Serialization::Formats::Uuid::is_valid(view)) {
    return {};
  }

  Core::Storage::Bytes<36> stored;
  __builtin_memcpy(stored.get_access().get_data(), view.get_data(), 36);
  return Serialization::Formats::Uuid::deserialize(stored);
}

static auto release(ttx_retained api) -> void {
  if (api.operations &&
      api.operations->release(api.context) != TTX_RELEASE_SUCCESS) {
    UtilityFunctions::push_error(
        "Image provider could not complete retained cleanup.");
  }
}

// A loaded factory and its code live through all image acquisitions created
// from it. Host references share this one loader lifetime; they imply nothing
// about the representation of provider state.
class ImageProvider {
 public:
  Count references = 1;
  Core::Storage::Option<System::Process::Library> library;
  image_provider api{};
  Ref<RefCounted> script;

  void retain() { ++references; }
  void release() {
    if (--references == 0) {
      this->~ImageProvider();
      Perimortem::Memory::Allocator::Global::release(
          Core::Data::cast<U8>(this));
    }
  }

  static auto open(String selected, Ref<RefCounted> object, String& error)
      -> ImageProvider*;
};

auto ImageProvider::open(String selected, Ref<RefCounted> object, String& error)
    -> ImageProvider* {
  auto allocation =
      Perimortem::Memory::Allocator::Global::allocate(sizeof(ImageProvider));
  auto& provider = Toolchain::Memory::Lifetime::construct<ImageProvider>(
      allocation.get_data());
  provider.script = object;
  if (object.is_valid()) {
    return &provider;
  }

  if (selected.is_empty() || selected == "default") {
    selected = OS::get_singleton()->get_environment("TTX_IMAGE_PROVIDER");
  }

  if (selected.is_empty()) {
    selected = "cpu";
  }

  if (selected == "gdscript") {
    selected = "res://addons/godot_ttx/gdscript/provider.gd";
  }

  if (selected.ends_with(".gd") || selected.ends_with(".gdc")) {
    const Ref<Resource> resource =
        ResourceLoader::get_singleton()->load(selected);
    if (resource.is_valid()) {
      provider.script = resource->call("new");
    }

    if (provider.script.is_valid()) {
      return &provider;
    }

    error = "Script factory could not be created: " + selected;
    provider.release();
    return nullptr;
  }

  const Variant configured =
      ProjectSettings::get_singleton()->get_setting_with_override(
          "ttx/image_modules");
  if (configured.get_type() == Variant::DICTIONARY) {
    const Dictionary paths = configured;
    if (paths.has(selected)) {
      selected = paths[selected];
    }
  }

#ifdef TOOLCHAIN_WASM

  const String location = selected.get_file();

#else

  const String location =
      selected.begins_with("res://")
          ? ProjectSettings::get_singleton()->globalize_path(selected)
          : selected;

#endif

  const auto encoded = location.utf8();
  const Core::View::Bytes path(
      perimortem_view_bytes{
        Core::Data::cast<const U8>(encoded.get_data()),
        Count(encoded.length())});

  Perimortem::Memory::Allocator::Arena errors;
  bool accepted = false;
  System::Process::Library::open(path, errors)
      .visit(
          [&](System::Process::Library& library) {
            library.symbol("ttx_entry"_view, errors)
                .visit(
                    [&](void* symbol) {
                      const auto& published =
                          *Core::Data::cast<const ttx_storage>(symbol);
                      ttx_semantic_query query{};
                      const ttx_storage target{
                        ttx_semantic_query_representation(),
                        Core::Data::cast<U8>(&query), sizeof(query)};

                      if (ttx_transfer(&published, &target) !=
                              TTX_DATA_SUCCESS ||
                          !query.bind || !query.supports) {
                        error = "Module does not supply a compatible Query.";
                        return;
                      }

                      Ttx::Semantic::Negotiation::Query(query)
                          .bind<Ttx::Graph::Abstract>()
                          .visit(
                              [&](Ttx::Graph::Abstract abstract) {
                                const auto api = abstract.get_abi();
                                accepted = api.operations->bind(
                                               api.context,
                                               {GODOT_IMAGE_PROVIDER_ID_HIGH,
                                                GODOT_IMAGE_PROVIDER_ID_LOW},

                                               &provider.api) ==
                                           TTX_NEGOTIATION_SATISFIED;
                                if (accepted) {
                                  provider.library = Core::Data::take(library);
                                } else {
                                  error =
                                      "Module does not supply the image "
                                      "factory contract.";
                                }
                              },

                              [&](Ttx::Semantic::Negotiation::Status status) {
                                error = "Module refused Abstract bootstrap.";
                              });
                    },

                    [&](Core::View::Bytes message) { error = text(message); });
          },

          [&](Core::View::Bytes message) { error = text(message); });
  if (accepted) {
    return &provider;
  }

  provider.release();
  return nullptr;
}

// Multiple host expressions can share one acquisition. Final host release
// consumes that acquisition before returning its provider's module reference.
class ImagePublication {
 public:
  Count references = 1;
  ttx_retained retained;
  ImageProvider* provider;
  compute_pixels pixels{};

  ImagePublication(ttx_retained retained, ImageProvider& provider)
      : retained(retained), provider(&provider) {
    provider.retain();
  }

  auto abstract() const -> ttx_abstract {
    return retained.operations->get_abstract(retained.context);
  }

  void retain() { ++references; }
  void release() {
    if (--references == 0) {
      ::release(retained);
      provider->release();
      this->~ImagePublication();
      Perimortem::Memory::Allocator::Global::release(
          Core::Data::cast<U8>(this));
    }
  }

  static auto accept(
      ttx_retained result,
      ImageProvider& provider,
      String& error) -> ImagePublication* {
    const auto abstract = result.operations->get_abstract(result.context);
    compute_pixels pixels{};
    if (abstract.operations->bind(
            abstract.context, {COMPUTE_PIXELS_ID_HIGH, COMPUTE_PIXELS_ID_LOW},
            &pixels) != TTX_NEGOTIATION_SATISFIED) {
      error = "Created image does not supply RGBA8 Pixels.";
      ::release(result);
      return nullptr;
    }

    auto allocation = Perimortem::Memory::Allocator::Global::allocate(
        sizeof(ImagePublication));
    auto& publication =
        Toolchain::Memory::Lifetime::construct<ImagePublication>(
            allocation.get_data(),
            [&] { return ImagePublication(result, provider); });
    publication.pixels = pixels;
    return &publication;
  }
};

class Godot::Demo::Adapters::Imaging::ImageNode {
 public:
  Count references = 1;
  ImageNode* input = nullptr;
  ImageNode* overlay = nullptr;
  image_offer operation{};
  PackedFloat32Array weights;
  U32 width = 0, height = 0;
  ImagePublication* publication = nullptr;
  U64 revision = 0, input_revision = 0, overlay_revision = 0;
  bool running = false;
  String error;

  static auto create() -> ImageNode* {
    auto allocation =
        Perimortem::Memory::Allocator::Global::allocate(sizeof(ImageNode));
    return &Toolchain::Memory::Lifetime::construct<ImageNode>(
        allocation.get_data());
  }

  void retain() { ++references; }
  void release() {
    if (--references == 0) {
      if (publication) {
        publication->release();
      }

      if (overlay) {
        overlay->release();
      }

      if (input) {
        input->release();
      }

      this->~ImageNode();
      Perimortem::Memory::Allocator::Global::release(
          Core::Data::cast<U8>(this));
    }
  }

  void publish(ImagePublication* next) {
    auto* previous = publication;
    publication = next;
    ++revision;
    if (previous) {
      previous->release();
    }
  }

  auto evaluate() -> bool {
    if (!input) {
      return publication != nullptr;
    }

    if (running) {
      error = "Image evaluation reentered the same expression.";
      return false;
    }

    running = true;

    bool valid = input->evaluate();
    if (!valid) {
      error = input->error;
    }

    if (overlay && !overlay->evaluate()) {
      valid = false;
      error = overlay->error;
    }

    const bool changed = revision == 0 || input_revision != input->revision ||
                         (overlay && overlay_revision != overlay->revision);
    if (!changed) {
      running = false;
      return valid && publication;
    }

    input_revision = input->revision;
    overlay_revision = overlay ? overlay->revision : 0;

    ttx_retained output{};
    if (valid) {
      const auto abstract = input->publication->abstract();
      image_error result{};
      ttx_negotiation_status status = TTX_NEGOTIATION_UNKNOWN;
      if (operation.input == IMAGE_INPUT_NONE) {
        image_invert api{};
        status = abstract.operations->bind(
            abstract.context, operation.contract, &api);
        if (status == TTX_NEGOTIATION_SATISFIED) {
          result = api.operations->apply(api.context, &output);
        }
      } else if (operation.input == IMAGE_INPUT_KERNEL) {
        image_convolve api{};
        status = abstract.operations->bind(
            abstract.context, operation.contract, &api);
        if (status == TTX_NEGOTIATION_SATISFIED) {
          result = api.operations->apply(
              api.context,
              {width, height, weights.ptr(), Count(weights.size())}, &output);
        }
      } else {
        image_composite api{};
        status = abstract.operations->bind(
            abstract.context, operation.contract, &api);
        if (status == TTX_NEGOTIATION_SATISFIED) {
          result = api.operations->apply(
              api.context, overlay->publication->abstract(), &output);
        }
      }

      if (status != TTX_NEGOTIATION_SATISFIED) {
        error = "Image does not supply the requested operation.";
        valid = false;
      } else if (result.size) {
        error = text(result);
        valid = false;
      }
    }

    auto* next = valid ? ImagePublication::accept(
                             output, *input->publication->provider, error)
                       : nullptr;
    publish(next);
    running = false;
    return next != nullptr;
  }
};

void TtxImage::_bind_methods() {
  ClassDB::bind_method(
      D_METHOD("set_provider", "provider"), &TtxImage::set_provider);
  ClassDB::bind_method(D_METHOD("get_provider"), &TtxImage::get_provider);
  ADD_PROPERTY(
      PropertyInfo(Variant::STRING, "provider"), "set_provider",
      "get_provider");
  ClassDB::bind_method(
      D_METHOD("set_provider_object", "object"),
      &TtxImage::set_provider_object);
  ClassDB::bind_method(
      D_METHOD("get_provider_object"), &TtxImage::get_provider_object);
  ClassDB::bind_method(
      D_METHOD("set_rgba8", "width", "height", "pixels"), &TtxImage::set_rgba8);
  ClassDB::bind_method(D_METHOD("snapshot"), &TtxImage::snapshot);
  ClassDB::bind_method(D_METHOD("read_pixels"), &TtxImage::read_pixels);
  ClassDB::bind_method(D_METHOD("get_width"), &TtxImage::get_width);
  ClassDB::bind_method(D_METHOD("get_height"), &TtxImage::get_height);
  ClassDB::bind_method(D_METHOD("get_error"), &TtxImage::get_error);
  ClassDB::bind_method(
      D_METHOD("get_provider_statistics"), &TtxImage::get_provider_statistics);
  ClassDB::bind_method(D_METHOD("get_offers"), &TtxImage::get_offers);
  ClassDB::bind_method(
      D_METHOD("admit", "operation", "width", "height"), &TtxImage::admit,
      DEFVAL(0), DEFVAL(0));
  ClassDB::bind_method(
      D_METHOD("invoke", "operation", "arguments"), &TtxImage::invoke,
      DEFVAL(Array()));
  ClassDB::bind_method(D_METHOD("invert"), &TtxImage::invert);
  ClassDB::bind_method(
      D_METHOD("convolve", "weights", "width", "height"), &TtxImage::convolve);
  ClassDB::bind_method(D_METHOD("composite", "overlay"), &TtxImage::composite);
}

TtxImage::~TtxImage() {
  Evaluation evaluation;
  if (node) {
    node->release();
  }
}

void TtxImage::set_provider(const String& value) {
  if (evaluating) {
    error = "Provider selection cannot change during evaluation.";
    return;
  }

  Evaluation evaluation;
  provider = value;
  provider_object.unref();
}

auto TtxImage::get_provider() const -> String {
  return provider_object.is_valid() ? String("object")
         : provider.is_empty()      ? String("default")
                                    : provider;
}

void TtxImage::set_provider_object(const Ref<RefCounted>& value) {
  if (evaluating) {
    error = "Provider selection cannot change during evaluation.";
    return;
  }

  Evaluation evaluation;
  provider_object = value;
  provider = String();
}

auto TtxImage::get_provider_object() const -> Ref<RefCounted> {
  return provider_object;
}

auto TtxImage::set_rgba8(GDExtensionInt width, GDExtensionInt height, const PackedByteArray& pixels)
    -> bool {
  if (evaluating) {
    error = "Source publication is not allowed during evaluation.";
    return false;
  }

  Evaluation evaluation;
  error = String();
  if (node && node->input) {
    error = "Operation results cannot publish source pixels.";
    return false;
  }

  if (width <= 0 || height <= 0 || width > 4096 || height > 4096 ||
      pixels.size() != width * height * 4) {
    error = "RGBA8 dimensions and pixel count must agree.";
    return false;
  }

  auto* supply = ImageProvider::open(provider, provider_object, error);
  if (!supply) {
    return false;
  }

  ttx_retained result{};
  bool accepted;
  if (supply->script.is_valid()) {
    accepted = Scripts::Bridge::create(
        supply->script, width, height, pixels, result, error);
  } else {
    const auto failure = supply->api.operations->create(
        supply->api.context, width, height, pixels.ptr(), pixels.size(),
        &result);
    accepted = failure.size == 0;
    if (!accepted) {
      error = text(failure);
    }
  }

  auto* publication =
      accepted ? ImagePublication::accept(result, *supply, error) : nullptr;
  supply->release();
  if (!publication) {
    return false;
  }

  if (!node) {
    node = ImageNode::create();
  }

  node->publish(publication);
  emit_changed();
  return true;
}

auto TtxImage::evaluate() -> bool {
  error = String();
  if (!node) {
    error = "Image has no source pixels.";
    return false;
  }

  if (!node->evaluate()) {
    error = node->error;
    return false;
  }

  return true;
}

auto TtxImage::read_pixels() -> PackedByteArray {
  Evaluation evaluation;
  PackedByteArray output;
  if (!evaluate()) {
    return output;
  }

  const auto pixels = node->publication->pixels;
  const auto size = pixels.operations->dimensions(pixels.context);
  if (!size.width || !size.height || size.width > 4096 || size.height > 4096) {
    error = "Image dimensions exceed the lab's limits.";
    return output;
  }

  output.resize(Count(size.width) * size.height * 4);
  if (pixels.operations->read(pixels.context, output.ptrw(), output.size()) !=
      COMPUTE_IMAGE_SUCCESS) {
    error = "Image pixels could not be read.";
    return {};
  }

  return output;
}

auto TtxImage::get_width() -> GDExtensionInt {
  Evaluation evaluation;
  return evaluate() ? node->publication->pixels.operations
                          ->dimensions(node->publication->pixels.context)
                          .width
                    : 0;
}

auto TtxImage::get_height() -> GDExtensionInt {
  Evaluation evaluation;
  return evaluate() ? node->publication->pixels.operations
                          ->dimensions(node->publication->pixels.context)
                          .height
                    : 0;
}

auto TtxImage::snapshot() -> Ref<TtxImage> {
  Evaluation evaluation;
  if (!evaluate()) {
    return {};
  }

  Ref<TtxImage> result;
  result.instantiate();
  result->node = ImageNode::create();
  node->publication->retain();
  result->node->publish(node->publication);
  result->provider = provider;
  result->provider_object = provider_object;
  return result;
}

auto TtxImage::get_provider_statistics() -> Dictionary {
  Evaluation evaluation;
  Dictionary result;
  if (!evaluate()) {
    return result;
  }

  auto& supply = *node->publication->provider;
  const auto statistics =
      supply.api.operations
          ? supply.api.operations->statistics(supply.api.context)
          : image_provider_statistics{};
  result["uploads"] = GDExtensionInt(statistics.uploads);
  result["downloads"] = GDExtensionInt(statistics.downloads);
  result["live_images"] = GDExtensionInt(statistics.live_images);
  result["plan_builds"] = GDExtensionInt(statistics.plan_builds);
  return result;
}

auto TtxImage::get_offers() -> Array {
  Evaluation evaluation;
  Array result;
  if (!evaluate()) {
    return result;
  }

  const auto abstract = node->publication->abstract();
  image_offers offers{};
  if (abstract.operations->bind(
          abstract.context,
          {GODOT_IMAGE_OFFERS_ID_HIGH, GODOT_IMAGE_OFFERS_ID_LOW},
          &offers) != TTX_NEGOTIATION_SATISFIED) {
    error = "Image does not supply operation offers.";
    return result;
  }

  offers.operations->visit(
      offers.context, {&result, [](void* context, image_offer offer) {
                         Dictionary value;
                         value["contract"] = identifier(offer.contract);
                         value["name"] = text(offer.name);
                         value["input"] = GDExtensionInt(offer.input);
                         value["minimum"] = GDExtensionInt(offer.minimum);
                         value["maximum"] = GDExtensionInt(offer.maximum);
                         value["step"] = GDExtensionInt(offer.step);
                         Core::Data::cast<Array>(context)->append(value);
                       }});

  return result;
}

auto TtxImage::admit(const String& operation, GDExtensionInt width, GDExtensionInt height)
    -> Dictionary {
  Evaluation evaluation;
  Dictionary result;
  result["status"] = 1;
  result["reason"] = "Operation is not offered.";
  if (width < 0 || height < 0 || width > U32(-1) || height > U32(-1)) {
    result["status"] = 3;
    result["reason"] = "Kernel dimensions are out of range.";
    return result;
  }

  const Array values = get_offers();
  for (GDExtensionInt i = 0; i < values.size(); ++i) {
    const Dictionary value = values[i];
    if (String(value["name"]) != operation &&
        String(value["contract"]) != operation) {
      continue;
    }

    const auto abstract = node->publication->abstract();
    image_offers offers{};
    if (abstract.operations->bind(
            abstract.context,
            {GODOT_IMAGE_OFFERS_ID_HIGH, GODOT_IMAGE_OFFERS_ID_LOW},
            &offers) != TTX_NEGOTIATION_SATISFIED) {
      return result;
    }

    const auto admission = offers.operations->admit(
        offers.context, parse(value["contract"]), width, height);
    result["status"] = GDExtensionInt(admission.status);
    result["reason"] = text(admission.reason);
    return result;
  }

  return result;
}

auto TtxImage::invoke(const String& name, const Array& arguments)
    -> Ref<TtxImage> {
  Evaluation evaluation;
  const Array values = get_offers();
  Dictionary selected;
  for (GDExtensionInt i = 0; i < values.size(); ++i) {
    const Dictionary value = values[i];
    if (String(value["name"]) == name || String(value["contract"]) == name) {
      selected = value;
      break;
    }
  }

  if (selected.is_empty()) {
    error = "Image operation is not offered.";
    return {};
  }

  const GDExtensionInt kind = selected["input"];
  Ref<TtxImage> overlay;
  PackedFloat32Array weights;
  GDExtensionInt width = 0, height = 0;
  if (kind == IMAGE_INPUT_NONE) {
    if (!arguments.is_empty()) {
      error = "Operation takes no arguments.";
      return {};
    }
  } else if (kind == IMAGE_INPUT_KERNEL) {
    if (arguments.size() != 3 ||
        arguments[0].get_type() != Variant::PACKED_FLOAT32_ARRAY ||
        arguments[1].get_type() != Variant::INT ||
        arguments[2].get_type() != Variant::INT) {
      error = "Kernel requires weights and integer dimensions.";
      return {};
    }

    weights = arguments[0];
    width = arguments[1];
    height = arguments[2];
    if (width <= 0 || height <= 0 || width > 1023 || height > 1023 ||
        !(width & 1) || !(height & 1) || weights.size() != width * height) {
      error = "Kernel dimensions and coefficient count must agree.";
      return {};
    }
  } else if (kind == IMAGE_INPUT_IMAGE) {
    if (arguments.size() != 1) {
      error = "Operation requires one image.";
      return {};
    }

    overlay = arguments[0];
    if (overlay.is_null() || !overlay->node) {
      error = "Overlay has no source pixels.";
      return {};
    }
  } else {
    error = "Operation signature is unsupported.";
    return {};
  }

  auto* expression = ImageNode::create();
  expression->input = node;
  node->retain();
  expression->overlay = overlay.is_valid() ? overlay->node : nullptr;
  if (expression->overlay) {
    expression->overlay->retain();
  }

  expression->operation.contract = parse(selected["contract"]);
  expression->operation.input = kind;
  expression->weights = weights.duplicate();
  expression->width = width;
  expression->height = height;
  if (!expression->evaluate()) {
    error = expression->error;
    expression->release();
    return {};
  }

  Ref<TtxImage> result;
  result.instantiate();
  result->node = expression;
  result->provider = provider;
  result->provider_object = provider_object;
  return result;
}

auto TtxImage::invert() -> Ref<TtxImage> {
  return invoke("invert");
}

auto TtxImage::convolve(
    const PackedFloat32Array& weights,
    GDExtensionInt width,
    GDExtensionInt height) -> Ref<TtxImage> {
  Array arguments;
  arguments.append(weights);
  arguments.append(width);
  arguments.append(height);
  return invoke("convolve", arguments);
}

auto TtxImage::composite(const Ref<TtxImage>& overlay) -> Ref<TtxImage> {
  Array arguments;
  arguments.append(overlay);
  return invoke("composite", arguments);
}
