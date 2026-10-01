// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "godot_ttx/contracts/scalar.h"
#include "ttx/concept/abstract.hpp"
#include "ttx/data/form/representation.hpp"

namespace Godot::Extension::Contracts {

// Publishes a Godot field descriptor from a label and value role. The exporter
// uses the role to select its Variant conversion and copies the label while
// observing the descriptor. The author keeps the label alive for that call.
class Scalar {
 public:
  enum class Kind : U8 {
    Empty = GODOT_SCALAR_EMPTY,
    Boolean = GODOT_SCALAR_BOOLEAN,
    Integer = GODOT_SCALAR_INTEGER,
    Real = GODOT_SCALAR_REAL,
    Text = GODOT_SCALAR_TEXT,
    Buffer = GODOT_SCALAR_BUFFER,
  };

  Scalar(Perimortem::Core::View::Bytes name, Kind kind)
      : value(
            perimortem_view_bytes(name.get_data(), name.get_size()),
            static_cast<U8>(kind)) {}

  auto get_abstract() const -> Ttx::Concept::Abstract {
    return Ttx::Concept::Abstract(godot_scalar_abstract(&value));
  }

  static auto get_representation(Kind kind)
      -> const Ttx::Data::Form::Representation& {
    return *godot_scalar_representation(static_cast<U8>(kind));
  }

 private:
  godot_scalar value;
};

}  // namespace Godot::Extension::Contracts
