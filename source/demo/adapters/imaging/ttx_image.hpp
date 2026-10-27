// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/resource.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"
#include "godot_cpp/variant/packed_float32_array.hpp"

namespace Godot::Demo::Adapters::Imaging {

class ImageNode;

// TtxImage keeps the lab's live dependency expression. Derived resources follow
// source revisions, while snapshot keeps the current retained image. Provider
// choice belongs to source publication; operations use contracts supplied by
// the encountered image, including images implemented in GDScript.
//
// Godot owns these Resources. Each expression independently keeps its inputs,
// retained result and loaded provider code available through its final use.
// Calls run on Godot's scene thread. Source publication requires evaluation
// to have finished, including callbacks made by script providers.
class TtxImage : public godot::Resource {
  GDCLASS(TtxImage, godot::Resource)
 public:
  ~TtxImage() override;
  void set_provider(const godot::String& value);
  auto get_provider() const -> godot::String;
  void set_provider_object(const godot::Ref<godot::RefCounted>& object);
  auto get_provider_object() const -> godot::Ref<godot::RefCounted>;
  auto set_rgba8(GDExtensionInt width, GDExtensionInt height, const godot::PackedByteArray& pixels)
      -> bool;
  auto snapshot() -> godot::Ref<TtxImage>;
  auto read_pixels() -> godot::PackedByteArray;
  auto get_width() -> GDExtensionInt;
  auto get_height() -> GDExtensionInt;
  auto get_error() const -> godot::String { return error; }
  auto get_provider_statistics() -> godot::Dictionary;
  auto get_offers() -> godot::Array;
  auto admit(const godot::String& operation, GDExtensionInt width = 0, GDExtensionInt height = 0)
      -> godot::Dictionary;
  auto invoke(
      const godot::String& operation,
      const godot::Array& arguments = {}) -> godot::Ref<TtxImage>;
  auto invert() -> godot::Ref<TtxImage>;
  auto convolve(const godot::PackedFloat32Array& weights, GDExtensionInt width, GDExtensionInt height)
      -> godot::Ref<TtxImage>;
  auto composite(const godot::Ref<TtxImage>& overlay) -> godot::Ref<TtxImage>;

 protected:
  static void _bind_methods();

 private:
  auto evaluate() -> bool;
  ImageNode* node = nullptr;
  godot::String provider;
  godot::Ref<godot::RefCounted> provider_object;
  godot::String error;
};

}  // namespace Godot::Demo::Adapters::Imaging
