// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "perimortem/memory/global/vector.hpp"

#include "coherence/type_system/callable.hpp"
#include "godot/values/field.hpp"
#include "godot_cpp/core/memory.hpp"

namespace Godot::Extension::Values {

// Frame owns Godot conversion metadata and one Representation compiled from a
// Coherence Callable. Member frames include the receiver as their first hidden
// Field. Explicit arguments follow in authored order. Frame recomputes their
// local offsets and accepts them only when their extent and alignment agree
// with the compiler's complete form.
class Frame {
 public:
  static auto arguments(
      Coherence::TypeSystem::Callable::Contract callable,
      const Ttx::Graph::Abstract* receiver,
      const Ttx::Data::Form::Representation& form,
      Field::Resolver resolver) -> Perimortem::Core::Storage::Option<Frame>;
  static auto result(
      Coherence::TypeSystem::Callable::Contract callable,
      const Ttx::Data::Form::Representation& form,
      Field::Resolver resolver) -> Perimortem::Core::Storage::Option<Frame>;
  auto get_representation() const -> const Ttx::Data::Form::Representation& {
    return form;
  }

  auto get_fields() const -> Perimortem::Core::View::Vector<Field> {
    return fields.get_view();
  }

  auto get_extent() const -> Count { return form.get_extent(); }
  auto get_alignment() const -> Count { return form.get_alignment(); }

 private:
  static auto compile(
      Perimortem::Memory::Global::Vector<Field> fields,
      const Ttx::Data::Form::Representation& form)
      -> Perimortem::Core::Storage::Option<Frame>;
  Frame(
      Ttx::Data::Form::Representation form,
      Perimortem::Memory::Global::Vector<Field> fields)
      : form(Perimortem::Core::Data::take(form)),
        fields(Perimortem::Core::Data::take(fields)) {}
  Ttx::Data::Form::Representation form;
  Perimortem::Memory::Global::Vector<Field> fields;
};

}  // namespace Godot::Extension::Values
