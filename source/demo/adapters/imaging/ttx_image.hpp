// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include "demo/imaging/graph/call.hpp"
#include "demo/imaging/graph/source.hpp"

namespace Godot::Demo::Adapters::Imaging {

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
  auto get_offers() -> godot::Array;
  auto admit(
      const godot::String& operation,
      int64_t width = 0,
      int64_t height = 0) -> godot::Dictionary;
  auto invoke(
      const godot::String& operation,
      const godot::Array& arguments = {}) -> godot::Ref<TtxImage>;

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
      const Godot::Demo::Imaging::Graph::Operation&,
      const GDExtensionConstVariantPtr*,
      GDExtensionInt,
      Perimortem::Core::Option<Perimortem::Memory::Allocator::Arena> constants =
          {}) -> godot::Ref<TtxImage>;
  auto evaluate(Perimortem::Memory::Allocator::Arena&)
      -> const Godot::Demo::Imaging::Graph::Image*;
  auto open_provider(Perimortem::Memory::Allocator::Arena&) -> bool;
  godot::Ref<godot::RefCounted> provider_object;
  godot::String provider;
  godot::String error;
  Godot::Demo::Imaging::Graph::Expression* expression = nullptr;
  Godot::Demo::Imaging::Graph::Source* source = nullptr;
  Godot::Demo::Imaging::Graph::Provider* supply = nullptr;
};

}  // namespace Godot::Demo::Adapters::Imaging
