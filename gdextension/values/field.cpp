// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "gdextension/values/field.hpp"

#include <godot_cpp/godot.hpp>

#include "perimortem/core/data.hpp"

using namespace Perimortem;
using namespace Gdextension;

Gdextension::Values::Field::Field(
    Core::View::Bytes name,
    Count offset,
    const Ttx::Data::Form::Representation& representation,
    godot::Variant::Type type)
    : offset(offset),
      representation(&representation),
      info(
          type,
          godot::String::utf8(
              reinterpret_cast<const char*>(name.get_data()),
              name.get_size())) {}

auto Gdextension::Values::Field::compile(
    Ttx::Concept::Abstract subject,
    Count offset) -> Utility::Result<Field, Ttx::Data::Status> {
  using Kind = ::Gdextension::Contracts::Scalar::Kind;
  const U64 roles[] = {
    LAB_BOOLEAN_ID_LOW, LAB_INTEGER_ID_LOW, LAB_REAL_ID_LOW, LAB_TEXT_ID_LOW,
    LAB_BUFFER_ID_LOW};
  const Kind kinds[] = {
    Kind::Boolean, Kind::Integer, Kind::Real, Kind::Text, Kind::Text};
  const godot::Variant::Type types[] = {
    godot::Variant::BOOL, godot::Variant::INT, godot::Variant::FLOAT,
    godot::Variant::STRING, godot::Variant::PACKED_BYTE_ARRAY};
  for (Count i = 0; i != 5; ++i) {
    const auto answer = subject.get_query().supports(
        System::Uuid(LAB_SCALAR_ID_HIGH, roles[i]));
    const bool found =
        answer == Ttx::Semantic::Negotiation::Binding::Status::Satisfied;
    Ttx::Data::Status status = Ttx::Data::Status::Unsupported;
    if (answer == Ttx::Semantic::Negotiation::Binding::Status::Pending) {
      status = Ttx::Data::Status::Busy;
    } else if (
        answer == Ttx::Semantic::Negotiation::Binding::Status::Rejected) {
      status = Ttx::Data::Status::Denied;
    }
    if (found) {
      // The lab scalar role supplies a parameter label through get_data. Only
      // that admitted role gives these bytes a name interpretation. Generic
      // Abstract data has no such text or naming promise.
      return Field(
          subject.get_data(), offset,
          ::Gdextension::Contracts::Scalar::get_representation(kinds[i]),
          types[i]);
    }

    if (status != Ttx::Data::Status::Unsupported) {
      return status;
    }
  }

  return Ttx::Data::Status::Unsupported;
}

auto Gdextension::Values::Field::read(
    const void* source,
    bool variant,
    U8* frame,
    Borrow& borrow) const -> void {
  auto* target = frame + offset;
  switch (info.type) {
  case godot::Variant::BOOL: {
    const U8 value = variant ? bool(*static_cast<const godot::Variant*>(source))
                             : *static_cast<const bool*>(source);
    Core::Data::copy(target, &value);
    break;
  }
  case godot::Variant::INT: {
    const S64 value = variant
                          ? int64_t(*static_cast<const godot::Variant*>(source))
                          : *static_cast<const int64_t*>(source);
    Core::Data::copy(target, &value);
    break;
  }
  case godot::Variant::FLOAT: {
    const R64 value = variant
                          ? double(*static_cast<const godot::Variant*>(source))
                          : *static_cast<const double*>(source);
    Core::Data::copy(target, &value);
    break;
  }
  case godot::Variant::STRING: {
    borrow.text =
        (variant ? godot::String(*static_cast<const godot::Variant*>(source))
                 : *static_cast<const godot::String*>(source))
            .utf8();
    const perimortem_view_bytes value{
      reinterpret_cast<const U8*>(borrow.text.get_data()),
      Count(borrow.text.length())};
    Core::Data::copy(target, &value);
    break;
  }
  case godot::Variant::PACKED_BYTE_ARRAY: {
    borrow.bytes = variant
                       ? godot::PackedByteArray(
                             *static_cast<const godot::Variant*>(source))
                       : *static_cast<const godot::PackedByteArray*>(source);
    const perimortem_view_bytes value{
      borrow.bytes.ptr(), Count(borrow.bytes.size())};
    Core::Data::copy(target, &value);
    break;
  }
  default:
    break;
  }
}

auto Gdextension::Values::Field::value(const U8* frame) const
    -> godot::Variant {
  const auto* source = frame + offset;
  switch (info.type) {
  case godot::Variant::BOOL:
    return godot::Variant(bool(*source));
  case godot::Variant::INT: {
    S64 value;
    Core::Data::copy(reinterpret_cast<U8*>(&value), source, sizeof(value));
    return godot::Variant(int64_t(value));
  }
  case godot::Variant::FLOAT: {
    R64 value;
    Core::Data::copy(reinterpret_cast<U8*>(&value), source, sizeof(value));
    return godot::Variant(double(value));
  }
  case godot::Variant::STRING: {
    perimortem_view_bytes value;
    Core::Data::copy(reinterpret_cast<U8*>(&value), source, sizeof(value));
    return godot::Variant(
        godot::String::utf8(
            reinterpret_cast<const char*>(value.data), value.size));
  }
  case godot::Variant::PACKED_BYTE_ARRAY: {
    perimortem_view_bytes value;
    Core::Data::copy(reinterpret_cast<U8*>(&value), source, sizeof(value));
    godot::PackedByteArray bytes;
    bytes.resize(value.size);
    Core::Data::copy(bytes.ptrw(), value.data, value.size);
    return godot::Variant(bytes);
  }
  default:
    break;
  }

  return godot::Variant();
}

auto Gdextension::Values::Field::put(
    const U8* frame,
    void* destination,
    bool variant) const -> void {
  const auto result = value(frame);
  if (variant) {
    godot::internal::gdextension_interface_variant_new_copy(
        destination, result._native_ptr());
    return;
  }

  switch (info.type) {
  case godot::Variant::BOOL:
    *static_cast<bool*>(destination) = bool(result);
    break;
  case godot::Variant::INT:
    *static_cast<int64_t*>(destination) = int64_t(result);
    break;
  case godot::Variant::FLOAT:
    *static_cast<double*>(destination) = double(result);
    break;
  case godot::Variant::STRING:
    *static_cast<godot::String*>(destination) = godot::String(result);
    break;
  case godot::Variant::PACKED_BYTE_ARRAY:
    *static_cast<godot::PackedByteArray*>(destination) =
        godot::PackedByteArray(result);
    break;
  default:
    break;
  }
}
