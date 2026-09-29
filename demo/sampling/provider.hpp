// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "demo/sampling/contracts/samples.hpp"
#include "ttx/concept/capabilities/borrow.hpp"
#include "ttx/concept/policies/borrowed.hpp"

namespace Godot::Demo::Sampling {

// Provider exposes Samples over a native computation. Borrow retains that
// computation through its owner's callbacks and returns an Abstract that keeps
// the same interfaces. Releasing the acquired answer returns that reference.
class Provider {
 public:
  Provider(
      const void* source,
      const sample_operations& operations,
      void (*retain)(const void*),
      void (*release)(const void*))
      : binding(source, &operations),
        retain_value(retain),
        return_value(release) {}
  Provider(const Provider&) = delete;
  auto operator=(const Provider&) -> Provider& = delete;

  // The Query observes this provider. A consumer retaining the computation
  // requests Borrow before keeping its Samples binding.
  auto get_query() const -> ttx_semantic_query;
  auto get_data() const -> Perimortem::Core::View::Bytes {
    return Perimortem::Core::View::Bytes();
  }
  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage target) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto borrow() const -> Perimortem::Utility::Result<
      Ttx::Concept::Policies::Borrowed,
      Ttx::Semantic::Negotiation::Binding::Failure>;
  auto release() const -> void { return_value(binding.source); }

 private:
  sample_api binding;
  void (*retain_value)(const void*);
  void (*return_value)(const void*);
};

}  // namespace Godot::Demo::Sampling
