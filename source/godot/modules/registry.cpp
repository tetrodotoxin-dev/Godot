// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "godot/modules/registry.hpp"

#include "toolchain/memory/lifetime.hpp"

#include "perimortem/core/view/null_terminated.hpp"
#include "perimortem/memory/allocator/global.hpp"

#include "coherence/compilation/compiler.hpp"
#include "coherence/type_system/sections.hpp"
#include "coherence/type_system/selections/intersection.hpp"
#include "coherence/type_system/type.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "ttx/graph/capabilities/describe.hpp"
#include "ttx/graph/capabilities/selection.hpp"
#include "ttx/graph/policies/constant.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Graph;
using namespace Ttx::Data;
using namespace Ttx::Semantic;
using namespace Godot::Extension;
using namespace Toolchain::Memory;

// TODO: Coherence canonicalization must identify equivalent semantic Type
// locations across modules and simulacra. Until that contract exists, this
// bridge accepts only Constant Type locations whose Type and Constant reentry
// operations reproduce one context address. This is narrower than Constant's
// general semantic location guarantee.
static auto constant_type(Abstract candidate) -> Storage::Option<Abstract> {
  Storage::Option<Abstract> result;

  candidate.bind<Coherence::TypeSystem::Type::Contract>().visit(
      [&](Coherence::TypeSystem::Type::Contract type) {
        const Abstract location = type.get_abstract();
        location.bind<Policies::Constant>().visit(
            [&](Policies::Constant constant) {
              const Abstract selected = constant.get_abstract();
              if (selected.get_abi().context != location.get_abi().context) {
                return;
              }

              selected.bind<Coherence::TypeSystem::Type::Contract>().visit(
                  [&](Coherence::TypeSystem::Type::Contract rebound) {
                    if (rebound.get_abstract().get_abi().context ==
                        selected.get_abi().context) {
                      result = selected;
                    }
                  },

                  [] {});
            },

            [] {});
      },

      [] {});

  return result;
}

template <typename Function>
static auto visit_callables(Abstract type, Abstract scope, Function& callback)
    -> bool {
  const Storage::Vector<Abstract, 2> sections = {{
    Coherence::TypeSystem::Sections::callables().get_abstract(),
    scope,
  }};

  auto request = Coherence::TypeSystem::Selections::Intersection::create(
      sections.get_view());
  Negotiation::Status status = Negotiation::Status::Unknown;
  type.bind<Capabilities::Selection>().visit(
      [&](Capabilities::Selection selection) {
        auto visit = [&](View::Bytes, Abstract callable) {
          callback(callable);
        };

        status =
            selection.select(request.get_abstract(), Abstract::Visitor(visit));
      },

      [] {});
  return status == Negotiation::Status::Satisfied;
}

auto Modules::Registry::resolver() -> Values::Field::Resolver {
  return {
    this,
    [](void* context, Abstract abstract,
       const Form::Representation& form) -> Values::Field::Materializer {
      auto* type =
          Data::cast<Modules::Registry>(context)->materialized_type(abstract);
      if (!type || !type->get_representation().compatible(form)) {
        return {};
      }

      return type->get_materializer();
    }};
}

auto Modules::Registry::materialized_type(Abstract abstract)
    -> Classes::Class* {
  return constant_type(abstract).visit(
      [&](Abstract type) -> Classes::Class* {
        using Class = Classes::Class;
        const auto api = type.get_abi();
        for (Count i = 0; i < registrations.get_size(); ++i) {
          auto* registered = registrations.get_data()[i];
          const auto existing = registered->get_abstract().get_abi();
          if (existing.context == api.context) {
            return registered->is_represented() && registered->is_complete()
                       ? registered
                       : nullptr;
          }
        }

        if (!type_budget) {
          return nullptr;
        }

        --type_budget;

        Class* compiled = nullptr;
        Coherence::Compilation::Compiler::compile(type).visit(
            [&](Form::Representation& form) {
              if (!form.get_extent() || form.get_extent() > 65536 ||
                  form.get_alignment() > 64) {
                return;
              }

              type.bind<Ttx::Graph::Capabilities::Describe>().visit(
                  [&](Ttx::Graph::Capabilities::Describe describe) {
                    describe.get_name().visit(
                        [&](View::Bytes label) {
                          const auto name = godot::String::utf8(
                              Data::cast<const char>(label.get_data()),
                              label.get_size());
                          if (!name.is_valid_identifier() ||
                              godot::ClassDB::class_exists(name)) {
                            return;
                          }

                          for (Count i = 0; i < registrations.get_size(); ++i) {
                            auto* existing = registrations.get_data()[i];
                            if (existing->get_name() == name) {
                              return;
                            }
                          }

                          auto allocation =
                              Allocator::Global::allocate(sizeof(Class));
                          compiled = &Lifetime::construct<Class>(
                              allocation.get_data(),
                              [&] { return Class(type, name); });
                          compiled->set_value_type(Data::take(form));
                          registrations.insert(compiled);

                          Global::Vector<Classes::Method> methods;
                          bool duplicate = false;
                          auto visit = [&](Abstract declaration) {
                            Classes::Method::compile(
                                declaration, resolver(), &type)
                                .visit(
                                    [&](Classes::Method& method) {
                                      for (Count i = 0; i < methods.get_size();
                                           ++i) {
                                        if (methods.get_data()[i].get_name() ==
                                            method.get_name()) {
                                          duplicate = true;
                                          return;
                                        }
                                      }

                                      methods.insert(Data::take(method));
                                    },

                                    [] {});
                          };

                          const bool selected = visit_callables(
                              type,
                              Coherence::TypeSystem::Sections::members()
                                  .get_abstract(),
                              visit);
                          if (!selected || duplicate) {
                            compiled->refuse();
                            return;
                          }

                          compiled->set_methods(Data::take(methods));
                        },

                        [] {});
                  },

                  [] {});
            },

            [](Status) {});
        // Recursive method compilation can already hold this placeholder as a
        // materializer. Keep its address available so dependency closure can
        // refuse those callers together when scoped discovery marked the Type
        // incomplete.
        return compiled;
      },

      []() -> Classes::Class* { return nullptr; });
}

auto Modules::Registry::consider(Abstract abstract) -> bool {
  return constant_type(abstract).visit(
      [&](Abstract type) {
        bool accepted = false;
        type.bind<Capabilities::Describe>().visit(
            [&](Capabilities::Describe describe) {
              describe.get_name().visit(
                  [&](View::Bytes label) {
                    const auto name = godot::String::utf8(
                        Data::cast<const char>(label.get_data()),
                        label.get_size());
                    if (!name.is_valid_identifier() ||
                        godot::ClassDB::class_exists(name)) {
                      return;
                    }

                    Global::Vector<Classes::Method> methods;
                    bool duplicate = false;
                    auto method = [&](Abstract declaration) {
                      Classes::Method::compile(declaration, resolver())
                          .visit(
                              [&](Classes::Method& value) {
                                for (Count i = 0; i < methods.get_size(); ++i) {
                                  const auto& existing = methods.get_data()[i];
                                  if (existing.get_name() == value.get_name()) {
                                    duplicate = true;
                                    return;
                                  }
                                }

                                methods.insert(Data::take(value));
                              },

                              [] {});
                    };

                    const bool selected = visit_callables(
                        type,
                        Coherence::TypeSystem::Sections::statics()
                            .get_abstract(),
                        method);
                    if (!selected || duplicate || methods.is_empty()) {
                      return;
                    }

                    const auto allocation =
                        Allocator::Global::allocate(sizeof(Classes::Class));
                    auto* registration = &Lifetime::construct<Classes::Class>(
                        allocation.get_data(), [&] {
                          return Classes::Class(
                              type, name, Data::take(methods));
                        });

                    registrations.insert(registration);
                    accepted = true;
                  },

                  [] {});
            },

            [] {});
        return accepted;
      },

      [] { return false; });
}

auto Modules::Registry::visit(Abstract abstract, Count depth, Count& budget)
    -> void {
  if (!budget || depth == 16 ||
      abstract.get_state() != Abstract::State::Present) {
    return;
  }

  --budget;
  if (consider(abstract)) {
    return;
  }

  auto callback = [&](View::Bytes route, Abstract child) {
    visit(child, depth + 1, budget);
  };

  abstract.visit_routes(Abstract::Visitor(callback));
}

auto Modules::Registry::load(const godot::Array& paths) -> void {
  imports.load(paths);

  Count budget = 4096;
  visit(imports.get_abstract(), 0, budget);

  Global::Vector<Bool> unique;
  for (Count i = 0; i < registrations.get_size(); ++i) {
    const auto* type = registrations.get_data()[i];
    Count matches = 0;
    for (Count j = 0; j < registrations.get_size(); ++j) {
      const auto* other = registrations.get_data()[j];
      if (type->get_name() == other->get_name()) {
        ++matches;
      }
    }

    unique.insert(type->is_complete() && matches == 1);
  }

  Global::Vector<Classes::Class*> accepted;
  // A refused registration also removes callers whose record conversions
  // depend on it. This closes the dependency set before any Class is deleted,
  // so accepted methods retain every materializer they can call.
  bool changed;
  do {
    changed = false;
    for (Count i = 0; i < registrations.get_size(); ++i) {
      if (!unique.get_data()[i]) {
        continue;
      }

      for (Count j = 0; j < registrations.get_size(); ++j) {
        if (!unique.get_data()[j] && registrations.get_data()[i]->depends_on(
                                         *registrations.get_data()[j])) {
          unique.get_data()[i] = False;
          changed = true;
          break;
        }
      }
    }
  } while (changed);

  for (Count i = 0; i < registrations.get_size(); ++i) {
    auto* type = registrations.get_data()[i];
    if (unique.get_data()[i] && type->publish()) {
      accepted.insert(type);
    } else {
      type->~Class();
      Allocator::Global::release(Data::cast<U8>(type));
    }
  }

  registrations = Data::take(accepted);
}

Modules::Registry::~Registry() {
  for (Count i = registrations.get_size(); i > 0; --i) {
    auto* type = registrations.get_data()[i - 1];
    type->~Class();
    Allocator::Global::release(Data::cast<U8>(type));
  }
}
