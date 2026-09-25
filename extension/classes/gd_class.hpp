// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "extension/contracts/gd_class.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/modules/module.hpp"
#include "ttx/semantic/ownership/publication.hpp"

namespace Godot::Extension::Classes {

// GDClass interprets a class declaration through Godot's exposure policy. Its
// own binding adds emission while other questions still reach the encountered
// Abstract, preserving any restrictions that object already carries. Godot
// names and native bases come from host configuration rather than the provider.
//
// This object is temporary compiler state. Emission copies registration facts
// and retains an independent runtime factory and module. A resulting Class
// never calls back into this policy or its source graph.
class GDClass {
 public:
  GDClass(
      Ttx::Concept::Abstract subject,
      const Ttx::Concept::Modules::Module& module,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Bytes base)
      : subject(subject), module(module), name(name), base(base) {}
  auto get_interface() const -> Ttx::Concept::Abstract;
  auto get_error() const -> Perimortem::Core::View::Bytes { return error; }

 private:
  // Emission uses this private policy's configuration and writes its
  // diagnostic. Keeping it here avoids publishing mutable compiler state as
  // another API.
  auto emit() const -> Perimortem::Utility::
      Result<Ttx::Semantic::Ownership::Publication, Ttx::Data::Status>;
  Ttx::Concept::Abstract subject;
  const Ttx::Concept::Modules::Module& module;
  Perimortem::Core::View::Bytes name;
  Perimortem::Core::View::Bytes base;
  mutable Perimortem::Core::View::Bytes error;
};

}  // namespace Godot::Extension::Classes
