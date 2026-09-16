// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "imaging/graph/image.hpp"
#include "ttx/concept/abstract.hpp"

namespace Godot::Imaging::Graph {

// A live result must follow source edits while an old snapshot must keep its
// original pixels. Expression provides the stable dependency node for that
// distinction: its revision describes the current answer, and the Image it
// returns owns one immutable publication. Dependents compare revisions after
// pulling their parents instead of treating a retained pointer as fresh data.
//
// Evaluation and publication share one worker. An evaluated reference remains
// borrowed until the next change to that node. Retaining Image is the explicit
// way to preserve the answer beyond that boundary.
class Expression {
 public:
  virtual ~Expression() = default;
  virtual auto resolve_concept(Perimortem::Core::View::Bytes) const
      -> Ttx::Concept::Abstract {
    return Ttx::Concept::Abstract(ttx_none());
  }
  virtual auto visit_concepts(Ttx::Concept::Abstract::Visitor) const -> void {}
  virtual constexpr auto get_data() const -> Perimortem::Core::View::Bytes {
    return "Expression"_view;
  }

  void retain();
  void release();
  auto evaluate(Perimortem::Memory::Allocator::Arena& errors) -> Perimortem::
      Utility::Result<const Image&, Perimortem::Core::View::Bytes>;
  auto get_revision() const -> U64 { return revision; }

 protected:
  explicit Expression(U8* allocation) : allocation(allocation) {}
  void advance();
  virtual auto evaluate_value(Perimortem::Memory::Allocator::Arena&, U64 pull)
      -> Perimortem::Utility::
          Result<const Image&, Perimortem::Core::View::Bytes> = 0;

 private:
  U8* allocation;
  U64 revision = 0;
};

}  // namespace Godot::Imaging::Graph
