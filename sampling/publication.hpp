// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "contracts/samples.hpp"
#include "sampling/provider.h"

namespace Godot::Sampling {

// Native providers can publish a Samples table without building an Abstract
// graph. This optional helper supplies the existing Thunk negotiation and
// closes the provider's own state when its publication ends. Another language
// can implement the C bootstrap directly without inheriting this class.
class Publication {
 public:
  Publication(
      const void* source,
      const sample_operations& operations,
      void (*release)(const void*))
      : binding(
            Ttx::Semantic::Binding::provide<Contracts::Samples>(
                source,
                operations)),
        release(release) {}
  Publication(const Publication&) = delete;
  auto operator=(const Publication&) -> Publication& = delete;

  // The entry transfers its one owned publication through this record. It
  // does not acquire another reference, and the query borrows this address.
  auto get_provider() const -> sample_provider;

 private:
  Ttx::Semantic::Binding binding;
  void (*release)(const void*);
};

}  // namespace Godot::Sampling
