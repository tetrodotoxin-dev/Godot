// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "perimortem/memory/global/vector.hpp"

#include "ttx/graph/capabilities/callable.hpp"

#include "godot/values/field.hpp"

#include "godot_cpp/core/memory.hpp"

namespace Godot::Extension::Values {

// Frame owns copied argument metadata and a canonical Representation of the
// bridge's call storage. That form records every field's offset and alignment. Before each call, the bridge compares this complete form with
// the provider's Invocation. Discovery therefore selects conversions without
// assuming that visitation order establishes native offsets.
class Frame {
 public:
  static auto arguments(Ttx::Graph::Capabilities::Callable callable)
      -> Perimortem::Core::Storage::Option<Frame>;
  static auto result(Ttx::Graph::Capabilities::Callable callable)
      -> Perimortem::Core::Storage::Option<Frame>;
  auto get_representation() const -> const Ttx::Data::Form::Representation& {
    return form;
  }

  auto get_fields() const -> Perimortem::Core::View::Vector<Field> {
    return fields.get_view();
  }

  auto get_extent() const -> Count { return form.get_extent(); }
  auto get_alignment() const -> Count { return form.get_alignment(); }

 private:
  static auto compile(Perimortem::Memory::Global::Vector<Field> fields)
      -> Perimortem::Core::Storage::Option<Frame>;
  Frame(
      Ttx::Data::Form::Representation form,
      Perimortem::Memory::Global::Vector<Field> fields)
      : form(Perimortem::Core::Data::take(form)),
        fields(Perimortem::Core::Data::take(fields)) {}
  Ttx::Data::Form::Representation form;
  Perimortem::Memory::Global::Vector<Field> fields;
};

}  // namespace Godot::Extension::Values
