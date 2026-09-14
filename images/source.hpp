// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "images/expression.hpp"

namespace Godot::Images {

// Source is the publication point for an image supplied by the application.
// Replacing its value advances the dependency revision without changing the
// node that existing Calls reference. The old Image keeps its own native owner,
// so a snapshot can survive the edit without keeping an obsolete graph alive.
class Source : public Expression {
 public:
  static auto create(Image image) -> Source&;

  // Replacing a borrowed answer during observation would invalidate active
  // provider calls. Rejection preserves the current value and revision.
  auto publish(Image image) -> Bool;

  auto resolve_concept(Perimortem::Core::View::Bytes) const
      -> const Abstract& override;
  void visit_concepts(Visitor) const override;

 private:
  auto evaluate_value(Perimortem::Memory::Allocator::Arena&, U64)
      -> Perimortem::Utility::
          Result<const Image&, Perimortem::Core::View::Bytes> override;
  Source(U8* allocation, Image image);
  Image image;
};

}  // namespace Godot::Images
