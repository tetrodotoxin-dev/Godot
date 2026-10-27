// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "godot/modules/classes.hpp"

#include "toolchain/memory/lifetime.hpp"

#include "perimortem/memory/allocator/global.hpp"

#include "ttx/graph/capabilities/describe.hpp"
#include "ttx/graph/capabilities/retain.hpp"

#include "godot_cpp/core/class_db.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Graph;
using namespace Ttx::Semantic;
using namespace Godot::Extension;

auto Modules::Classes::consider(Abstract abstract) -> bool {
  if (!abstract.bind<Capabilities::Type>() ||
      !abstract.bind<Capabilities::Create>()) {
    return false;
  }

  bool accepted = false;
  abstract.bind<Capabilities::Retain>().visit(
      [&](Capabilities::Retain request) {
        request.retain().visit(
            [&](Policies::Retained acquired) {
              const auto factory = acquired.get_abstract();
              if (!factory.bind<Capabilities::Create>() ||
                  !factory.bind<Capabilities::Type>()) {
                acquired.release();
                return;
              }

              factory.bind<Capabilities::Describe>().visit(
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

                          Global::Vector<Godot::Extension::Classes::Method>
                              methods;
                          bool duplicate = false;
                          auto method = [&](View::Bytes route,
                                            Abstract declaration) {
                            Godot::Extension::Classes::Method::compile(
                                route, declaration)
                                .visit(
                                    [&](Godot::Extension::Classes::Method&
                                            value) {
                                      for (Count i = 0; i < methods.get_size();
                                           ++i) {
                                        const auto& existing =
                                            methods.get_data()[i];
                                        if (existing.get_name() ==
                                            value.get_name()) {
                                          duplicate = true;
                                          return;
                                        }
                                      }

                                      methods.insert(Data::take(value));
                                    },

                                    [] {});
                          };

                          factory.visit_routes(Abstract::Visitor(method));
                          if (duplicate) {
                            return;
                          }

                          const auto allocation = Allocator::Global::allocate(
                              sizeof(Godot::Extension::Classes::Class));
                          auto* type = &Toolchain::Memory::Lifetime::construct<
                              Godot::Extension::Classes::Class>(
                              allocation.get_data(), [&] {
                                return Godot::Extension::Classes::Class(
                                    acquired, name, Data::take(methods));
                              });

                          registrations.insert(type);
                          accepted = true;
                        },

                        [] {});
                  },

                  [] {});
              if (!accepted) {
                acquired.release();
              }
            },

            [&](Negotiation::Status status) {});
      },

      [] {});
  return accepted;
}

auto Modules::Classes::visit(Abstract abstract, Count depth, Count& budget)
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

auto Modules::Classes::load(const godot::Array& paths) -> void {
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

    unique.insert(matches == 1);
  }

  Global::Vector<Godot::Extension::Classes::Class*> accepted;
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

Modules::Classes::~Classes() {
  for (Count i = registrations.get_size(); i > 0; --i) {
    auto* type = registrations.get_data()[i - 1];
    type->~Class();
    Allocator::Global::release(Data::cast<U8>(type));
  }
}
