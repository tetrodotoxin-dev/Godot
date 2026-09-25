// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/sampling/class/contracts.h"

#include "extension/contracts/scalar.hpp"
#include "ttx/data/form/compiled.hpp"

using namespace Perimortem;
using namespace Godot::Extension::Contracts;

PERIMORTEM_C const ttx_representation* sampler_input_representation(
    U32 method) {
  if (method != 1) {
    return &Scalar::get_representation(
        method == 0 ? Scalar::Kind::Text : Scalar::Kind::Empty);
  }

  using Ttx::Data::Form::Schema;
  static constexpr auto integer = Schema::primitive(Schema::Value::S64);
  static constexpr Schema::Position fields[] = {
    Schema::Position(integer, __builtin_offsetof(sampler_count_input, seed)),
    Schema::Position(integer, __builtin_offsetof(sampler_count_input, first)),
    Schema::Position(integer, __builtin_offsetof(sampler_count_input, size)),
  };
  static constexpr auto schema = Schema::composite(
      fields, sizeof(sampler_count_input), alignof(sampler_count_input));
  return &Ttx::Data::Form::Compiled<schema>::get_representation();
}

PERIMORTEM_C const ttx_representation* sampler_output_representation(
    U32 method) {
  const Scalar::Kind kinds[] = {
    Scalar::Kind::Boolean, Scalar::Kind::Integer, Scalar::Kind::Text};
  return &Scalar::get_representation(kinds[method]);
}
