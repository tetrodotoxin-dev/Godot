// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/sampling/class/exports.hpp"

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "ttx/concept/policies/none.h"
#include "ttx/semantic/negotiation/library.h"

using namespace Godot::Demo;
using namespace Perimortem;

auto Sampling::Class::Exports::get_data() const -> Core::View::Bytes {
  return "Sampling module"_view;
}

auto Sampling::Class::Exports::resolve_concept(Core::View::Bytes name) const
    -> Ttx::Concept::Abstract {
  return name == declaration.get_data()
             ? Ttx::Concept::Abstract::provide(declaration)
             : Ttx::Concept::Abstract(ttx_none());
}

auto Sampling::Class::Exports::visit_concepts(
    Ttx::Concept::Abstract::Visitor visitor) const -> void {
  visitor(declaration.get_data(), Ttx::Concept::Abstract::provide(declaration));
}

PERIMORTEM_C __attribute__((visibility("default"))) ttx_binding_status
    ttx_query(ttx_semantic_query host, ttx_query_receiver receive) {
  const Sampling::Class::Exports plugin =
      Sampling::Class::Exports(Ttx::Semantic::Negotiation::Query(host));
  return receive.receive(
      receive.source, Ttx::Concept::Abstract::provide(plugin).get_query());
}
