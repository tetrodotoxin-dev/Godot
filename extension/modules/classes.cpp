// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "extension/modules/classes.hpp"

#include <godot_cpp/variant/utility_functions.hpp>

#include "perimortem/core/null_terminated.hpp"

#include "extension/classes/class.hpp"
#include "extension/classes/gd_class.hpp"
#include "ttx/concept/abstract.hpp"

using namespace Godot::Extension;
using namespace Perimortem;

static auto expose(
    Ttx::Concept::Abstract root,
    const godot::Dictionary& config,
    Memory::Dynamic::Vector<::Godot::Extension::Classes::Class*>& terminals)
    -> Core::Option<Core::View::Bytes> {
  // Configuration selects a sequence of exposed names. Each step visits the
  // encountered subject's advertised routes, so the next selection remains
  // governed by the policy that supplied it.
  godot::Array route;
  const godot::Variant configured = config.get("export", "");
  if (configured.get_type() == godot::Variant::ARRAY) {
    route = configured;
  } else {
    route.append(configured);
  }

  auto subject = root;
  for (int64_t index = 0; index != route.size(); ++index) {
    const auto expected = godot::String(route[index]).utf8();
    const Core::View::Bytes selected(
        reinterpret_cast<const U8*>(expected.get_data()), expected.length());
    Core::Option<Ttx::Concept::Abstract> found;
    bool duplicate = false;
    auto visitor = [&](Core::View::Bytes name, Ttx::Concept::Abstract value) {
      if (name == selected) {
        duplicate = bool(found);
        found = value;
      }
    };
    subject.visit_concepts(Ttx::Concept::Abstract::Visitor(visitor));
    if (!found || duplicate) {
      return "The configured export path is missing or ambiguous."_view;
    }
    subject = *found;
  }

  const auto name = godot::String(config.get("name", "")).utf8();
  const auto base = godot::String(config.get("base", "Node")).utf8();
  Godot::Extension::Classes::GDClass policy(
      terminals,
      Core::View::Bytes(
          reinterpret_cast<const U8*>(name.get_data()), name.length()),
      Core::View::Bytes(
          reinterpret_cast<const U8*>(base.get_data()), base.length()));
  const auto status =
      Ttx::Concept::Abstract::provide(policy)
          .bind<Ttx::Concept::Capabilities::Export>()
          .visit(
              [&](Ttx::Concept::Capabilities::Export exporter) {
                return exporter.expose(subject);
              },
              [](Ttx::Semantic::Negotiation::Binding::Failure failure) {
                return static_cast<Ttx::Semantic::Negotiation::Binding::Status>(
                    failure);
              });
  if (status != Ttx::Semantic::Negotiation::Binding::Status::Satisfied) {
    return policy.get_error();
  }
  return Core::Option<Core::View::Bytes>();
}

auto Godot::Extension::Modules::Classes::compile(
    const godot::Dictionary& config) -> Core::Option<Core::View::Bytes> {
  using namespace Ttx::Semantic::Negotiation;
  const auto alias = godot::String(config.get("module", "")).utf8();
  const perimortem_view_bytes input = perimortem_view_bytes(
      reinterpret_cast<const U8*>(alias.get_data()), Count(alias.length()));
  Core::Option<Core::View::Bytes> error;
  auto receive = [&](Ttx::Concept::Abstract root) {
    error = expose(root, config, terminals);
  };
  const auto status =
      Ttx::Concept::Capabilities::Import::provide(imports).visit(
          &input,
          Ttx::Data::Form::Compiled<Ttx::Data::Form::Native<
              perimortem_view_bytes>::reference>::get_representation(),
          receive);
  if (status != Binding::Status::Satisfied) {
    return "The configured module could not be imported."_view;
  }
  return error;
}

auto Godot::Extension::Modules::Classes::load(const godot::Array& configuration)
    -> void {
  for (int64_t index = 0; index < configuration.size(); ++index) {
    const godot::Dictionary entry = configuration[index];
    if (const auto error = compile(entry)) {
      godot::UtilityFunctions::push_error(
          "TTX class ", entry.get("name", ""), ": ",
          godot::String::utf8(
              reinterpret_cast<const char*>(error->get_data()),
              error->get_size()));
    }
  }
}

Godot::Extension::Modules::Classes::~Classes() {
  for (Count index = terminals.get_size(); index > 0; --index) {
    delete terminals[index - 1];
  }
}
