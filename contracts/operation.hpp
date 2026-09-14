// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/data/form/compiled.hpp"

namespace Godot::Contracts {

// Each current callable contract publishes one executable entry. Its UUID
// distinguishes the signature and behavior, while this description establishes
// where the pointer sits in the agreed table. Image filters and scalar sampling
// can share that carrier without sharing their result or ownership contracts.
// Native providers need no host Abstract graph to negotiate either one.
class Operation {
 public:
  // Each current family has one executable pointer. The UUID fixes its exact
  // signature. The prepared Data description checks the table's byte carrier.
  static auto get_representation() -> const Ttx::Data::Form::Representation& {
    using Ttx::Data::Form::Schema;
    static constexpr auto pointer = Schema::primitive(Schema::Value::Pointer);
    static constexpr Schema::Position entry(pointer, 0);
    static constexpr auto schema = Schema::composite({&entry, 1}, 8, 8);
    return Ttx::Data::Form::Compiled<schema>::get_representation();
  }
};

}  // namespace Godot::Contracts
