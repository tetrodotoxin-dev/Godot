// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/variant.hpp>

#include "gdextension/contracts/scalar.hpp"

namespace Gdextension::Values {

// Field is the host's prepared conversion for one encountered argument policy.
// Its name and selected meaning survive graph release. Conversion knows Godot,
// while the provider knows only the value contract and the frame coordinate.
class Field {
 public:
  // Conversion can lend temporary text or buffer storage to the provider.
  // The caller keeps these owners through output conversion so returned views
  // may refer to the input observation without being copied prematurely.
  struct Borrow {
    godot::CharString text;
    godot::PackedByteArray bytes;
  };

  static auto compile(Ttx::Concept::Abstract subject, Count offset)
      -> Perimortem::Utility::Result<Field, Ttx::Data::Status>;
  auto get_info() const -> const godot::PropertyInfo& { return info; }
  auto get_offset() const -> Count { return offset; }
  auto get_representation() const -> const Ttx::Data::Form::Representation& {
    return *representation;
  }
  auto read(const void* source, bool variant, U8* frame, Borrow& borrow) const
      -> void;
  auto value(const U8* frame) const -> godot::Variant;
  auto put(const U8* frame, void* destination, bool variant) const -> void;

 private:
  Field(
      Perimortem::Core::View::Bytes name,
      Count offset,
      const Ttx::Data::Form::Representation& representation,
      godot::Variant::Type type);
  Count offset;
  const Ttx::Data::Form::Representation* representation;
  godot::PropertyInfo info;
};

}  // namespace Gdextension::Values
