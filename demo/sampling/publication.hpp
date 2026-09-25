// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "demo/sampling/contracts/samples.hpp"
#include "ttx/semantic/ownership/publication.h"

namespace Godot::Demo::Sampling {

// Native providers can publish a Samples table without building an Abstract
// graph. This optional helper supplies checked API binding and
// closes the provider's own state when its publication ends. Another language
// can implement the C bootstrap directly without inheriting this class.
class Publication {
 public:
  Publication(
      const void* source,
      const sample_operations& operations,
      void (*release)(const void*))
      : binding{source, &operations}, release(release) {}
  Publication(const Publication&) = delete;
  auto operator=(const Publication&) -> Publication& = delete;

  // The entry transfers its one owned publication through this record. It
  // does not acquire another reference, and the query borrows this address.
  auto get_publication() const -> ttx_publication;

 private:
  sample_api binding;
  void (*release)(const void*);
};

}  // namespace Godot::Demo::Sampling
