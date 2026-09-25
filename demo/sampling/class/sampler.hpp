// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

#include "demo/sampling/function.hpp"
#include "ttx/concept/modules/import.hpp"

namespace Godot::Demo::Sampling::Class {

// Sampler owns a selected compute capability. Import identifiers belong to
// its host's policy, so this implementation needs no filenames, resource paths,
// Godot values or class declarations. Replacing a function only commits after
// the replacement has fulfilled the same Samples contract.
class Sampler {
 public:
  Sampler(
      Ttx::Concept::Modules::Import imports,
      Ttx::Semantic::Negotiation::Query host)
      : imports(imports), host(host) {}
  auto configure(Perimortem::Core::View::Bytes provider) -> bool;
  auto count(S64 seed, S64 first, S64 size) -> S64;
  auto get_error() const -> Perimortem::Core::View::Bytes {
    return error.get_view();
  }

 private:
  Ttx::Concept::Modules::Import imports;
  Ttx::Semantic::Negotiation::Query host;
  Perimortem::Core::Option<Godot::Demo::Sampling::Function> function;
  Perimortem::Memory::Dynamic::Bytes error;
};

}  // namespace Godot::Demo::Sampling::Class
