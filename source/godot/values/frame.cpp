// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "godot/values/frame.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Data;
using namespace Ttx::Graph;
using namespace Godot::Extension;

auto Values::Frame::compile(
    Global::Vector<Field> fields,
    const Form::Representation& supplied) -> Storage::Option<Frame> {
  if (fields.is_empty()) {
    return supplied.get_extent() == 0
               ? Storage::Option<Frame>(
                     Frame(Form::Representation(supplied), Data::take(fields)))
               : Storage::Option<Frame>();
  }

  Count extent = 0, alignment = 1;
  for (Count index = 0; index < fields.get_size(); ++index) {
    auto& field = fields.get_data()[index];
    const auto& form = field.get_representation();
    const auto required = form.get_alignment();
    extent = (extent + required - 1) & ~(required - 1);
    field.set_offset(extent);
    extent += form.get_extent();
    if (alignment < required) {
      alignment = required;
    }
  }

  extent = (extent + alignment - 1) & ~(alignment - 1);
  if (extent > 65536 || alignment > 64 || extent != supplied.get_extent() ||
      alignment != supplied.get_alignment()) {
    return {};
  }

  return Frame(Form::Representation(supplied), Data::take(fields));
}

auto Values::Frame::arguments(
    Coherence::TypeSystem::Callable::Contract callable,
    const Abstract* receiver,
    const Form::Representation& form,
    Field::Resolver resolver) -> Storage::Option<Frame> {
  Global::Vector<Field> fields;
  Bool accepted = True;
  if (receiver) {
    Field::result(*receiver, resolver)
        .visit(
            [&](Field& field) { fields.insert(Data::take(field)); },
            [&] { accepted = False; });
  }

  auto visit = [&](Abstract argument) {
    if (!accepted) {
      return;
    }

    if (fields.get_size() == 64) {
      accepted = False;
      return;
    }

    Field::compile(argument, resolver)
        .visit(
            [&](Field& field) { fields.insert(Data::take(field)); },
            [&] { accepted = False; });
  };

  callable.visit_arguments(visit);
  if (!accepted) {
    return {};
  }

  return compile(Data::take(fields), form);
}

auto Values::Frame::result(
    Coherence::TypeSystem::Callable::Contract callable,
    const Form::Representation& form,
    Field::Resolver resolver) -> Storage::Option<Frame> {
  const auto type = callable.get_result();
  Global::Vector<Field> fields;
  if (type.get_state() == Abstract::State::None) {
    return compile(Data::take(fields), form);
  }

  return Field::result(type, resolver)
      .visit(
          [&](Field& field) -> Storage::Option<Frame> {
            fields.insert(Data::take(field));
            return compile(Data::take(fields), form);
          },

          []() -> Storage::Option<Frame> { return {}; });
}
