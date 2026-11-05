// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "coherence/compilation/compiler.hpp"
#include "godot_cpp/core/property_info.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"
#include "godot_cpp/variant/variant.hpp"

namespace Godot::Extension::Values {

// Field supplies Godot conversions for values described by Coherence Type.
// Its compiled Representation fixes the byte extent and alignment of local
// call storage.
// Integers and reals become Godot numbers. Represented records
// become generated objects whose methods use the Type's member declarations.
// The application keeps referenced state accessible and follows the supplied
// value contract's explicit release rules. A copied record keeps that existing
// access duration.
// Before calling the provider, the bridge must match the complete input and
// output descriptor bytes. That comparison includes any referenced callable
// tables, so a matching record size alone cannot validate an operation table.
class Field {
 public:
  // The registration owner supplies these host conversion operations and
  // keeps their context accessible until its generated classes are removed.
  struct Materializer {
    godot::StringName class_name;
    void* context = nullptr;
    GDExtensionObjectPtr (*create)(void* context, const U8* bytes) = nullptr;
    bool (*copy)(void* context, GDExtensionObjectPtr object, U8* output) =
        nullptr;
  };

  struct Resolver {
    void* context;
    Materializer (*resolve)(
        void* context,
        Ttx::Graph::Abstract type,
        const Ttx::Data::Form::Representation& form);
  };

  static auto compile(Ttx::Graph::Abstract declaration, Resolver resolver)
      -> Perimortem::Core::Storage::Option<Field>;
  static auto result(Ttx::Graph::Abstract type, Resolver resolver)
      -> Perimortem::Core::Storage::Option<Field>;
  auto get_info() const -> const godot::PropertyInfo& { return info; }
  auto get_offset() const -> Count { return offset; }
  auto set_offset(Count value) -> void { offset = value; }
  auto uses_materializer(const void* context) const -> bool {
    return materializer.context == context;
  }

  auto get_representation() const -> const Ttx::Data::Form::Representation&;
  auto read(
      const void* source,
      bool variant,
      U8* frame,
      godot::PackedByteArray& bytes) const -> bool;
  auto put(const U8* frame, void* output, bool variant) const -> bool;

 private:
  Field(godot::String name, godot::Variant::Type type) : info(type, name) {}
  static auto compile_type(
      Ttx::Graph::Abstract type,
      const godot::String& name,
      Resolver resolver) -> Perimortem::Core::Storage::Option<Field>;
  Count offset = 0;
  godot::PropertyInfo info;
  Ttx::Data::Form::Representation form;
  Materializer materializer;
};

}  // namespace Godot::Extension::Values
