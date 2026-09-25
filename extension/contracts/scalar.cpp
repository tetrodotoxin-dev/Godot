// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "extension/contracts/scalar.hpp"

#include "ttx/data/form/compiled.hpp"

using namespace Perimortem;
using namespace Godot::Extension::Contracts;

PERIMORTEM_C ttx_abstract lab_scalar_abstract(const lab_scalar* scalar) {
  static const ttx_abstract_ops operations = {
    [](const void* source, perimortem_uuid id) -> ttx_binding_status {
      if (id.high == TTX_ABSTRACT_ID_HIGH && id.low == TTX_ABSTRACT_ID_LOW) {
        return TTX_BINDING_SATISFIED;
      }

      const auto& scalar = *static_cast<const lab_scalar*>(source);
      const U64 roles[] = {
        0, LAB_BOOLEAN_ID_LOW, LAB_INTEGER_ID_LOW, LAB_REAL_ID_LOW,
        LAB_TEXT_ID_LOW};
      return scalar.kind != LAB_SCALAR_EMPTY &&
                     scalar.kind <= LAB_SCALAR_TEXT &&
                     id.high == LAB_SCALAR_ID_HIGH &&
                     id.low == roles[scalar.kind]
                 ? TTX_BINDING_SATISFIED
                 : TTX_BINDING_UNSUPPORTED;
    },
    [](const void* source, perimortem_uuid id,
       ttx_storage requested) -> ttx_binding_status {
      if (id.high == TTX_ABSTRACT_ID_HIGH && id.low == TTX_ABSTRACT_ID_LOW) {
        const ttx_abstract api{source, &operations};
        return ttx_binding_provide(
            ttx_abstract_representation(), &api, requested);
      }

      const auto& scalar = *static_cast<const lab_scalar*>(source);
      const U64 roles[] = {
        0, LAB_BOOLEAN_ID_LOW, LAB_INTEGER_ID_LOW, LAB_REAL_ID_LOW,
        LAB_TEXT_ID_LOW};
      if (scalar.kind <= LAB_SCALAR_TEXT && scalar.kind != LAB_SCALAR_EMPTY &&
          id.high == LAB_SCALAR_ID_HIGH && id.low == roles[scalar.kind]) {
        return ttx_binding_marker(requested);
      }

      return TTX_BINDING_UNSUPPORTED;
    },
    [](const void* source) {
      return static_cast<const lab_scalar*>(source)->name;
    },
    [](const void* source) -> ttx_abstract { return {source, &operations}; },
    [](const void*, perimortem_view_bytes) { return ttx_none(); },
    [](const void*, ttx_concept_visitor) {},
  };
  return {scalar, &operations};
}

PERIMORTEM_C const ttx_representation* lab_scalar_representation(
    lab_scalar_kind kind) {
  using Ttx::Data::Form::Schema;
  static constexpr auto boolean = Schema::primitive(Schema::Value::U8);
  static constexpr auto integer = Schema::primitive(Schema::Value::S64);
  static constexpr auto real = Schema::primitive(Schema::Value::R64);
  static constexpr auto pointer = Schema::primitive(Schema::Value::Pointer);
  static constexpr auto count = Schema::primitive(Schema::Value::U64);
  static constexpr Schema::Position positions[] = {
    Schema::Position(pointer, __builtin_offsetof(perimortem_view_bytes, data)),
    Schema::Position(count, __builtin_offsetof(perimortem_view_bytes, size)),
  };
  static constexpr auto text = Schema::composite(
      positions, sizeof(perimortem_view_bytes), alignof(perimortem_view_bytes));
  static constexpr auto empty = Schema::composite({}, 0, 1);
  switch (static_cast<Scalar::Kind>(kind)) {
  case Scalar::Kind::Boolean:
    return &Ttx::Data::Form::Compiled<boolean>::get_representation();
  case Scalar::Kind::Integer:
    return &Ttx::Data::Form::Compiled<integer>::get_representation();
  case Scalar::Kind::Real:
    return &Ttx::Data::Form::Compiled<real>::get_representation();
  case Scalar::Kind::Text:
    return &Ttx::Data::Form::Compiled<text>::get_representation();
  case Scalar::Kind::Empty:
    return &Ttx::Data::Form::Compiled<empty>::get_representation();
  }

  return nullptr;
}
