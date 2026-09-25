// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <godot_cpp/variant/dictionary.hpp>

#include "ttx/concept/modules/import.hpp"

namespace Godot::Extension::Modules {

// Imports gives the application control over module names. Resource paths are
// interpreted here because their meaning belongs to this Godot project, not
// to a counter or compute provider. The service outlives every emitted class
// and runtime instance that borrowed it during module initialization.
class Imports {
 public:
  explicit Imports(godot::Dictionary paths) : paths(paths) {}
  auto get_query() const -> Ttx::Semantic::Negotiation::Query;
  auto open(Perimortem::Core::View::Bytes name) const -> Perimortem::Utility::
      Result<Ttx::Concept::Modules::Module, Ttx::Data::Status>;

 private:
  godot::Dictionary paths;
};

}  // namespace Godot::Extension::Modules
