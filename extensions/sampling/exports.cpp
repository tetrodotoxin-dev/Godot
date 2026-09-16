// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "extensions/sampling/exports.hpp"

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "ttx/concept/modules/module.h"

using namespace Godot;
using namespace Perimortem;

auto Extensions::Sampling::Exports::get_data() const -> Core::View::Bytes {
  return "Sampling module"_view;
}

auto Extensions::Sampling::Exports::resolve_concept(
    Core::View::Bytes name) const -> Ttx::Concept::Abstract {
  return name == declaration.get_data()
             ? Ttx::Concept::Abstract::provide(declaration)
             : Ttx::Concept::Abstract(ttx_none());
}

auto Extensions::Sampling::Exports::visit_concepts(
    Ttx::Concept::Abstract::Visitor visitor) const -> void {
  visitor(declaration.get_data(), Ttx::Concept::Abstract::provide(declaration));
}

PERIMORTEM_C __attribute__((visibility("default"))) ttx_data_status
    ttx_module_open(ttx_semantic_query host, ttx_module_acquisition* output) {
  auto storage =
      Core::Bibliotheca::check_out(sizeof(Extensions::Sampling::Exports));
  auto* plugin = new (storage.ptr, Core::Placement::Construct)
      Extensions::Sampling::Exports(Ttx::Semantic::Negotiation::Query(host));
  *output = {
    Ttx::Concept::Abstract::provide(*plugin).get_abi(), plugin,
    [](const void* source) {
      auto* plugin = const_cast<Extensions::Sampling::Exports*>(
          static_cast<const Extensions::Sampling::Exports*>(source));
      plugin->~Exports();
      Core::Bibliotheca::remit(reinterpret_cast<U8*>(plugin));
    }};
  return TTX_DATA_SUCCESS;
}
