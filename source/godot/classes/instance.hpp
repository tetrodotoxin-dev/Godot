// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "ttx/graph/policies/retained.hpp"

#include "godot/classes/method.hpp"

namespace Godot::Extension::Classes {

class Class;

// Instance holds one acquisition for a Godot object. Method bindings are
// requested through its retained Abstract immediately before invocation, so
// a provider can substitute state or refuse a changed interface. Destruction
// consumes the acquisition while Imports still keeps its module loaded.
class Instance {
 public:
  Instance(Class& type, Ttx::Graph::Policies::Retained publication)
      : type(type), publication(publication) {}
  ~Instance();
  Instance(const Instance&) = delete;
  auto operator=(const Instance&) -> Instance& = delete;
  auto invoke(const Method& method, const void* input, void* output)
      -> Ttx::Data::Status;

 private:
  Class& type;
  Ttx::Graph::Policies::Retained publication;
};

}  // namespace Godot::Extension::Classes
