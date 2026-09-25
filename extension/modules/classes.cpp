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
    const Ttx::Concept::Modules::Module& module,
    const godot::Dictionary& config) -> Utility::
    Result<Ttx::Semantic::Ownership::Publication, Core::View::Bytes> {
  using Result =
      Utility::Result<Ttx::Semantic::Ownership::Publication, Core::View::Bytes>;
  // Configuration selects a sequence of exposed names. Walking each layer's
  // visitor retains namespace and policy interception rather than resolving
  // through a wrapper to a more permissive referent. The terminal needs no
  // second export registry or provider specific discovery format.
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
      subject, module,
      Core::View::Bytes(
          reinterpret_cast<const U8*>(name.get_data()), name.length()),
      Core::View::Bytes(
          reinterpret_cast<const U8*>(base.get_data()), base.length()));
  return policy.get_interface()
      .bind<::Godot::Extension::Contracts::GDClass>()
      .visit(
          [&](::Godot::Extension::Contracts::GDClass gdclass) -> Result {
            return gdclass.emit().visit(
                [](Ttx::Semantic::Ownership::Publication& terminal) -> Result {
                  return Core::Data::take(terminal);
                },
                [&](Ttx::Data::Status) -> Result {
                  return policy.get_error();
                });
          },
          [](Ttx::Semantic::Negotiation::Binding::Failure) -> Result {
            return "GDClass policy did not bind."_view;
          });
}

auto Godot::Extension::Modules::Classes::compile(
    const godot::Dictionary& config) -> Utility::
    Result<Ttx::Semantic::Ownership::Publication, Core::View::Bytes> {
  using Result =
      Utility::Result<Ttx::Semantic::Ownership::Publication, Core::View::Bytes>;
  const auto alias = godot::String(config.get("module", "")).utf8();
  return imports
      .open(
          Core::View::Bytes(
              reinterpret_cast<const U8*>(alias.get_data()), alias.length()))
      .visit(
          [&](Ttx::Concept::Modules::Module& module) -> Result {
            return module.open(imports.get_query())
                .visit(
                    [&](Ttx::Concept::Modules::Module::Acquisition& discovery)
                        -> Result { return expose(discovery, module, config); },
                    [](Ttx::Data::Status) -> Result {
                      return "Module acquisition could not initialize."_view;
                    });
          },
          [](Ttx::Data::Status) -> Result {
            return "The configured module could not be imported."_view;
          });
}

auto Godot::Extension::Modules::Classes::load(const godot::Array& configuration)
    -> void {
  for (int64_t index = 0; index < configuration.size(); ++index) {
    const godot::Dictionary entry = configuration[index];
    compile(entry).visit(
        [&](Ttx::Semantic::Ownership::Publication& terminal) {
          // compile has returned: every source graph and GDClass policy is
          // already destroyed. Registration consumes only the prepared output.
          auto status =
              terminal.get_query()
                  .bind<::Godot::Extension::Contracts::Class>()
                  .visit(
                      [](::Godot::Extension::Contracts::Class type) {
                        return type.publish();
                      },
                      [](Ttx::Semantic::Negotiation::Binding::Failure) {
                        return Ttx::Data::Status::Invalid;
                      });
          if (status == Ttx::Data::Status::Success) {
            terminals.emplace(Core::Data::take(terminal));
          } else {
            godot::UtilityFunctions::push_error(
                "TTX emitted class could not register: ",
                entry.get("name", ""));
          }
        },
        [&](Core::View::Bytes error) {
          godot::UtilityFunctions::push_error(
              "TTX class ", entry.get("name", ""), ": ",
              godot::String::utf8(
                  reinterpret_cast<const char*>(error.get_data()),
                  error.get_size()));
        });
  }
}

Godot::Extension::Modules::Classes::~Classes() {
  for (Count index = terminals.get_size(); index > 0; --index) {
    terminals.get_data()[index - 1].close();
  }
}
