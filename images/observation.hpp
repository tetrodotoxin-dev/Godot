// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Godot::Images {

// A provider may call back into the host while answering a question. Keeping
// one observation open prevents those callbacks from replacing publications
// that the current call is still borrowing. Reads can nest, and a shared pull
// number lets the graph visit common ancestors once within that observation.
//
// This is the image host's synchronous publication policy. Providers need no
// knowledge of the graph, and TTX bindings acquire no implicit locks or owners.
// The admitting worker ends the scope before accepting another publication.
class Observation {
 public:
  Observation();
  Observation(const Observation&) = delete;
  auto operator=(const Observation&) -> Observation& = delete;
  ~Observation();

  static auto active() -> Bool;
  auto get_pull() const -> U64 { return pull; }

 private:
  U64 pull;
};

}  // namespace Godot::Images
