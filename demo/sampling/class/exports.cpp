// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/sampling/class/exports.hpp"

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "ttx/concept/modules/module.h"

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

PERIMORTEM_C __attribute__((visibility("default"))) ttx_data_status
    ttx_module_open(ttx_semantic_query host, ttx_module_acquisition* output) {
  auto storage = Core::Bibliotheca::check_out(sizeof(Sampling::Class::Exports));
  auto* plugin = new (storage.ptr, Core::Placement::Construct)
      Sampling::Class::Exports(Ttx::Semantic::Negotiation::Query(host));
  *output = {
    Ttx::Concept::Abstract::provide(*plugin).get_abi(), plugin,
    [](const void* source) {
      auto* plugin = const_cast<Sampling::Class::Exports*>(
          static_cast<const Sampling::Class::Exports*>(source));
      plugin->~Exports();
      Core::Bibliotheca::remit(reinterpret_cast<U8*>(plugin));
    }};
  return TTX_DATA_SUCCESS;
}
