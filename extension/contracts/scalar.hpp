// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "extension/contracts/scalar.h"
#include "ttx/concept/abstract.hpp"
#include "ttx/data/form/representation.hpp"

namespace Godot::Extension::Contracts {

// Scalar publishes the small value vocabulary shared by this lab's examples.
// It owns no host conversion. A GDClass policy can recognize these markers,
// while an unrelated host can implement a different observation of them.
class Scalar {
 public:
  enum class Kind : U8 {
    Empty = LAB_SCALAR_EMPTY,
    Boolean = LAB_SCALAR_BOOLEAN,
    Integer = LAB_SCALAR_INTEGER,
    Real = LAB_SCALAR_REAL,
    Text = LAB_SCALAR_TEXT,
  };

  Scalar(Perimortem::Core::View::Bytes name, Kind kind)
      : value{{name.get_data(), name.get_size()}, static_cast<U8>(kind)} {}

  auto get_abstract() const -> Ttx::Concept::Abstract {
    return Ttx::Concept::Abstract(lab_scalar_abstract(&value));
  }

  static auto get_representation(Kind kind)
      -> const Ttx::Data::Form::Representation& {
    return *lab_scalar_representation(static_cast<U8>(kind));
  }

 private:
  lab_scalar value;
};

}  // namespace Godot::Extension::Contracts
