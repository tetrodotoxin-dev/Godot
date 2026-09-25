// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "extension/values/field.hpp"
#include "ttx/concept/declarations/callable.hpp"

namespace Godot::Extension::Values {

// Frame owns the prepared conversions and a copy of their wire description.
// Copying the canonical bytes releases the declaration graph without compiling
// another schema. Its field list follows callable argument order, which need
// not be the physical order of the provider's record.
class Frame {
 public:
  static auto compile(Ttx::Concept::Declarations::Callable::Frame source)
      -> Perimortem::Utility::Result<Frame, Ttx::Data::Status>;
  auto get_representation() const -> Ttx::Data::Form::Representation {
    return Ttx::Data::Form::Representation(
        encoding.get_view().get_data(), encoding.get_size());
  }
  auto get_fields() const -> Perimortem::Core::View::Vector<Field> {
    return fields.get_view();
  }
  auto get_extent() const -> Count { return extent; }
  auto get_alignment() const -> Count { return alignment; }

 private:
  Frame(
      const Ttx::Data::Form::Representation& form,
      Perimortem::Memory::Dynamic::Vector<Field> fields)
      : encoding(form.get_bytes()),
        fields(Perimortem::Core::Data::take(fields)),
        extent(form.get_extent()),
        alignment(form.get_alignment()) {}
  Perimortem::Memory::Dynamic::Bytes encoding;
  Perimortem::Memory::Dynamic::Vector<Field> fields;
  Count extent;
  Count alignment;
};

}  // namespace Godot::Extension::Values
