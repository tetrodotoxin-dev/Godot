// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "demo/imaging/contracts/provider.h"
#include "ttx/concept/capabilities/create.hpp"
#include "ttx/concept/policies/borrowed.hpp"
#include "ttx/semantic/negotiation/library.h"

namespace Godot::Demo::Imaging::Render {

// Runtime holds the image constructor and optional sampling state shared by
// retained Render declarations. The Compiler query is exposed as a separate
// child so its own Abstract continues to govern compiler operations.
class Runtime {
 public:
  Runtime(const Runtime&) = delete;
  auto operator=(const Runtime&) -> Runtime& = delete;
  using OpenImages = image_error (*)(image_provider*);
  using OpenSamples = ttx_binding_status (*)(ttx_query_receiver);
  static auto visit(
      OpenImages images,
      OpenSamples samples,
      ttx_query_receiver receive,
      Ttx::Semantic::Negotiation::Query capabilities =
          Ttx::Semantic::Negotiation::Query()) -> ttx_binding_status;
  static auto create(
      OpenImages images,
      OpenSamples samples,
      Ttx::Semantic::Negotiation::Query capabilities) -> Runtime&;
  ~Runtime();
  auto get_query() const -> Ttx::Semantic::Negotiation::Query;
  auto retain() const -> void;
  auto release() const -> void;
  auto get_compiler() const -> Ttx::Semantic::Negotiation::Query {
    return capabilities;
  }
  auto create_instance(void* receiver, void (*receive)(void*, ttx_abstract))
      const -> Ttx::Semantic::Negotiation::Binding::Status;
  static auto input(U32 method) -> const Ttx::Data::Form::Representation&;
  static auto output(U32 method) -> const Ttx::Data::Form::Representation&;

 private:
  Runtime(
      OpenImages images,
      OpenSamples samples,
      Ttx::Semantic::Negotiation::Query capabilities)
      : images(images), samples(samples), capabilities(capabilities) {}
  OpenImages images;
  OpenSamples samples;
  Ttx::Semantic::Negotiation::Query capabilities;
  Perimortem::Core::Option<Ttx::Concept::Policies::Borrowed> compute;
};

}  // namespace Godot::Demo::Imaging::Render
