// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "extension/values/frame.hpp"

#include "perimortem/core/algorithm/sort.hpp"

using namespace Perimortem;
using namespace Godot::Extension;

auto Godot::Extension::Values::Frame::compile(
    Ttx::Concept::Declarations::Callable::Frame source)
    -> Utility::Result<Frame, Ttx::Data::Status> {
  using Ttx::Data::Status;
  using Ttx::Data::Form::Representation;
  const auto& form = source.get_representation();
  Memory::Dynamic::Vector<Field> fields;
  struct Observation {
    Representation::Position position;
    auto operator>(const Observation& other) const -> Bool {
      return position.offset > other.position.offset;
    }
  };
  Memory::Dynamic::Vector<Observation> expected;
  for (Count index = 0; index != source.get_size(); ++index) {
    const auto status =
        Field::compile(source.get_subject(index), source.get_offset(index))
            .visit(
                [&](Field& field) {
                  const auto& value = field.get_representation();
                  if (field.get_offset() > form.get_extent() ||
                      value.get_extent() >
                          form.get_extent() - field.get_offset()) {
                    return Status::Bounds;
                  }

                  value.visit([&](Representation::Position position) {
                    position.offset += field.get_offset();
                    expected.insert(Observation(position));
                    return Status::Success;
                  });
                  fields.emplace(Core::Data::take(field));
                  return Status::Success;
                },
                [](Status failure) { return failure; });
    if (status != Status::Success) {
      return status;
    }
  }

  // Every physical input must have exactly one prepared source. Compare the
  // ordered primitive coordinates once to reject overlap, missing fields or
  // a role whose promised carrier disagrees with the declared frame. Runtime
  // conversion then uses the retained offsets without a representation walk.
  Core::Algorithm::sort(expected.get_access());
  Count index = 0;
  auto status = form.visit([&](Representation::Position position) {
    if (index == expected.get_size()) {
      return Status::Incompatible;
    }

    const auto& wanted = expected[index++].position;
    return position.offset == wanted.offset && position.compatible(wanted)
               ? Status::Success
               : Status::Incompatible;
  });
  if (status != Status::Success || index != expected.get_size()) {
    return Status::Incompatible;
  }

  return Frame(form, Core::Data::take(fields));
}
