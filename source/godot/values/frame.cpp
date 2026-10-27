// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "godot/values/frame.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Data;
using namespace Ttx::Graph;
using namespace Godot::Extension;

auto Values::Frame::compile(Global::Vector<Field> fields)
    -> Storage::Option<Frame> {
  if (fields.get_size() == 1) {
    auto form = fields.get_data()[0].get_representation();
    return Frame(Data::take(form), Data::take(fields));
  }

  Count extent = 0, alignment = 1;
  Global::Vector<Form::Representation::Member> members;
  for (Count index = 0; index < fields.get_size(); ++index) {
    auto& field = fields.get_data()[index];
    const auto& form = field.get_representation();
    const auto required = form.get_alignment();
    extent = (extent + required - 1) & ~(required - 1);
    field.set_offset(extent);
    members.insert({&form.get_abi(), extent});
    extent += form.get_extent();
    if (alignment < required) {
      alignment = required;
    }
  }

  extent = (extent + alignment - 1) & ~(alignment - 1);
  return Form::Representation::compose(members.get_view(), extent, alignment)
      .visit(
          [&](Form::Representation& form) -> Storage::Option<Frame> {
            return Frame(Data::take(form), Data::take(fields));
          },

          [](Status status) -> Storage::Option<Frame> { return {}; });
}

auto Values::Frame::arguments(Capabilities::Callable callable)
    -> Storage::Option<Frame> {
  Global::Vector<Field> fields;
  Bool accepted = True;
  auto visit = [&](Abstract argument) {
    if (!accepted) {
      return;
    }

    if (fields.get_size() == 64) {
      accepted = False;
      return;
    }

    Field::compile(argument).visit(
        [&](Field& field) { fields.insert(Data::take(field)); },
        [&] { accepted = False; });
  };

  callable.visit_args(visit);
  if (!accepted) {
    return {};
  }

  return compile(Data::take(fields));
}

auto Values::Frame::result(Capabilities::Callable callable)
    -> Storage::Option<Frame> {
  const auto type = callable.get_result();
  Global::Vector<Field> fields;
  if (type.get_state() == Abstract::State::None) {
    return compile(Data::take(fields));
  }

  return Field::result(type).visit(
      [&](Field& field) -> Storage::Option<Frame> {
        fields.insert(Data::take(field));
        return compile(Data::take(fields));
      },

      []() -> Storage::Option<Frame> { return {}; });
}
