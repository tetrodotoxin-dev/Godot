// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/modules/import.hpp"
#include "ttx/semantic/ownership/factory.hpp"
#include "ttx/semantic/ownership/publication.hpp"

namespace Godot::Demo::Sampling::Class {

// Runtime retains the injected import capability independently of its source
// graph. Its factory creates plain Samplers and publishes their typed thunks.
// The host and executable module outlive all resulting factories and instances.
class Runtime {
 public:
  static auto emit(Ttx::Semantic::Negotiation::Query host) -> Perimortem::
      Utility::Result<Ttx::Semantic::Ownership::Publication, Ttx::Data::Status>;
  Runtime(
      Ttx::Concept::Modules::Import imports,
      Ttx::Semantic::Negotiation::Query host)
      : imports(imports), host(host) {}
  auto get_query() const -> ttx_semantic_query;

 private:
  Ttx::Concept::Modules::Import imports;
  Ttx::Semantic::Negotiation::Query host;
};

}  // namespace Godot::Demo::Sampling::Class
