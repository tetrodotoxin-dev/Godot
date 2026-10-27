// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "ttx/graph/capabilities/type.hpp"

#include "godot_cpp/core/property_info.hpp"
#include "godot_cpp/variant/variant.hpp"

namespace Godot::Extension::Values {

// Field copies the metadata needed for one Godot numeric conversion. Its
// value form comes from Type, independently of the declaration's name. The
// bridge accepts complete known forms before using their payload storage.
class Field {
 public:
  static auto compile(Ttx::Graph::Abstract declaration)
      -> Perimortem::Core::Storage::Option<Field>;
  static auto result(Ttx::Graph::Abstract type)
      -> Perimortem::Core::Storage::Option<Field>;
  auto get_info() const -> const godot::PropertyInfo& { return info; }
  auto get_offset() const -> Count { return offset; }
  auto set_offset(Count value) -> void { offset = value; }
  auto get_representation() const -> const Ttx::Data::Form::Representation&;
  auto read(const void* source, bool variant, U8* frame) const -> void;
  auto put(const U8* frame, void* output, bool variant) const -> void;

 private:
  Field(godot::String name, godot::Variant::Type type) : info(type, name) {}
  static auto compile_type(Ttx::Graph::Abstract type, const godot::String& name)
      -> Perimortem::Core::Storage::Option<Field>;
  Count offset = 0;
  godot::PropertyInfo info;
};

}  // namespace Godot::Extension::Values
