// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "demo/imaging/graph/expression.hpp"
#include "demo/imaging/graph/kernel.hpp"
#include "demo/imaging/graph/operation.hpp"

namespace Godot::Demo::Imaging::Graph {

// An operation result depends on its receiver and authored inputs, even when
// the application has dropped the Resources that originally supplied them.
// Call retains those edges and pulls them before checking their revisions.
// A changed input replaces the cached result. An unchanged branch keeps its
// image and device resources without reconstructing an invocation.
//
// The callable binding follows the receiver's lifetime, while result validity
// follows all dependencies. An overlay edit can therefore reuse the bound
// compositor and blurred receiver. Replacing the receiver requires a fresh
// binding. Stored kernel bytes belong to the optional transferred Arena.
// Construction links existing nodes only, preserving an acyclic graph.
class Call : public Expression {
 public:
  struct Argument {
    Expression* expression = nullptr;
    const Kernel* constant = nullptr;
  };

  static auto create(
      Expression& receiver,
      const Operation& operation,
      Perimortem::Core::View::Vector<Argument> arguments,
      Perimortem::Core::Option<Perimortem::Memory::Allocator::Arena> constants =
          {}) -> Call&;
  ~Call() override;
  auto resolve_concept(Perimortem::Core::View::Bytes) const
      -> Ttx::Concept::Abstract override;
  void visit_concepts(Ttx::Concept::Abstract::Visitor) const override;
  auto get_evaluations() const -> U64 { return evaluations; }

 private:
  auto evaluate_value(Perimortem::Memory::Allocator::Arena& errors, U64 pull)
      -> Perimortem::Utility::
          Result<const Image&, Perimortem::Core::View::Bytes> override;
  struct Input {
    Argument argument;
    U64 revision = 0;
  };

  Call(
      U8*,
      Expression&,
      const Operation&,
      Perimortem::Core::View::Vector<Argument>,
      Perimortem::Core::Option<Perimortem::Memory::Allocator::Arena>);
  // These helpers operate on the same retained evaluation state. Keeping them
  // with Call makes its reentrancy and publication order explicit without
  // exposing mutable cache fields to free functions or another state owner.
  auto cached(Perimortem::Memory::Allocator::Arena&) -> Perimortem::Utility::
      Result<const Image&, Perimortem::Core::View::Bytes>;
  Expression& receiver;
  const Operation& operation;
  Perimortem::Core::Option<Perimortem::Memory::Allocator::Arena> constants;
  Perimortem::Memory::Dynamic::Vector<Input> inputs;
  Perimortem::Core::Option<Image> output;
  Perimortem::Core::Option<Invocation> binding;
  Perimortem::Memory::Dynamic::Bytes error;
  U64 receiver_revision = 0;
  U64 evaluations = 0;
  U64 last_pull = 0;
  bool evaluating = false;
};

}  // namespace Godot::Demo::Imaging::Graph
