// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include "images/call.hpp"
#include "images/source.hpp"

namespace Godot::Extension {

// The Resource exposed to Godot owns a dependency node, allowing existing game
// code to keep a result while its inputs change. Its provider choice applies
// when source pixels are published. Derived operations follow the receiver's
// provider through the native graph. GDScript therefore uses the same methods
// for native and script providers without acquiring their object
// representation.
//
// This class owns Godot conversion and error presentation. Image owns native
// values and module lifetime, while Source and Call own publication and cache
// validity. Keeping those responsibilities separate lets the same graph run in
// the native acceptance executable without linking the engine.
class TtxImage : public godot::Resource {
  GDCLASS(TtxImage, godot::Resource)
 public:
  ~TtxImage() override;
  void set_provider(const godot::String&);
  auto get_provider() const -> godot::String;
  // An existing factory is source configuration just like a module path.
  // Derivation and snapshots preserve that choice for later publications.
  // Provider changes are rejected while an observation borrows current state.
  void set_provider_object(const godot::Ref<godot::RefCounted>& object);
  auto get_provider_object() const -> godot::Ref<godot::RefCounted>;
  auto set_rgba8(int64_t width, int64_t height, const godot::PackedByteArray&)
      -> bool;
  auto snapshot() -> godot::Ref<TtxImage>;
  auto read_pixels() -> godot::PackedByteArray;
  auto get_width() -> int64_t;
  auto get_height() -> int64_t;
  auto get_error() const -> godot::String;
  // Diagnostic observations belong to the lab, separately from image identity.
  // They let script tests observe real device transfers and resource reuse.
  auto get_provider_statistics() -> godot::Dictionary;

 protected:
  static void _bind_methods();

 private:
  // Godot lends this exact registered Resource to its method callback. These
  // helpers keep its error, source and expression updates together. Exposing
  // those mutable fields would make the host lifetime harder to audit.
  static void call_operation(
      void*,
      GDExtensionClassInstancePtr,
      const GDExtensionConstVariantPtr*,
      GDExtensionInt,
      GDExtensionVariantPtr,
      GDExtensionCallError*);
  auto apply(
      const Images::Operation&,
      const GDExtensionConstVariantPtr*,
      GDExtensionInt) -> godot::Ref<TtxImage>;
  auto evaluate(Perimortem::Memory::Allocator::Arena&) -> const Images::Image*;
  auto open_provider(Perimortem::Memory::Allocator::Arena&) -> bool;
  godot::Ref<godot::RefCounted> provider_object;
  godot::String provider;
  godot::String error;
  Images::Expression* expression = nullptr;
  Images::Source* source = nullptr;
  Images::Provider* supply = nullptr;
};

}  // namespace Godot::Extension
