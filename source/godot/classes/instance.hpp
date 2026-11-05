// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "godot/classes/class.hpp"

namespace Godot::Extension::Classes {

// Instance stores either an empty factory receiver or a represented value.
// Represented values use their Type's member descriptions and pass their
// complete record as the receiver. The bridge interprets only this public call
// storage. The supplied functions interpret the provider's opaque context.
//
// GDScript explicitly releases acquisitions carried by represented values
// according to their typed contracts. Keeping a Godot reference preserves the
// record bytes, while their referenced state follows those obligations.
class Instance {
 public:
  // A Class reference alone selects the empty receiver used by factory methods.
  // Explicit construction prevents an unrelated Class argument from creating
  // that receiver through an implicit conversion.
  explicit Instance(Class& type) : type(type) {}
  Instance(Class& type, Perimortem::Core::View::Bytes value)
      : type(type), value(value) {}
  ~Instance();
  Instance(const Instance&) = delete;
  auto operator=(const Instance&) -> Instance& = delete;
  auto invoke(const Method& method, const void* input, void* output)
      -> Ttx::Data::Status;
  auto get_type() const -> const Class& { return type; }
  auto get_value() const -> Perimortem::Core::View::Bytes {
    return value.get_view();
  }

 private:
  Class& type;
  Perimortem::Memory::Global::Bytes value;
};

}  // namespace Godot::Extension::Classes
