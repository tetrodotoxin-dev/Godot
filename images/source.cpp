// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "images/source.hpp"

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/object.hpp"

#include "images/observation.hpp"

using namespace Godot;
using namespace Perimortem;

Images::Source::Source(U8* allocation, Image image)
    : Expression(allocation), image(Core::Data::take(image)) {
  advance();
}

auto Images::Source::create(Image image) -> Source& {
  static const Core::Object<>::Descriptor descriptor(
      sizeof(Source), alignof(Source),
      [](U8* bytes) { reinterpret_cast<Source*>(bytes)->~Source(); });
  auto storage = Core::Object<>::create(descriptor).get_payload();
  return *new (storage, Core::Placement::Construct)
      Source(storage, Core::Data::take(image));
}

auto Images::Source::publish(Image next) -> Bool {
  if (Observation::active()) {
    return False;
  }

  // Releasing the prior image may run script destruction callbacks. Install
  // the complete value and its revision first, then release that prior owner
  // while nested publication is still excluded. Nested reads see the new value.
  Observation observation;
  Image previous(Core::Data::take(image));
  image = Core::Data::take(next);
  advance();
  return True;
}

auto Images::Source::evaluate_value(Memory::Allocator::Arena&, U64)
    -> Utility::Result<const Image&, Core::View::Bytes> {
  return image;
}

auto Images::Source::resolve_concept(Core::View::Bytes name) const
    -> const Abstract& {
  if (name == "value"_view) {
    return image;
  }

  return Abstract::resolve_concept(name);
}

void Images::Source::visit_concepts(Visitor visitor) const {
  visitor("value"_view, image);
}
