// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "godot_ttx/modules/imports.hpp"

#include <godot_cpp/classes/project_settings.hpp>

using namespace Godot::Extension;
using namespace Perimortem;
using namespace Ttx::Semantic::Negotiation;

auto Modules::Imports::open(Core::View::Bytes name) const
    -> Utility::Result<Library, Ttx::Data::Status> {
  using Result = Utility::Result<Library, Ttx::Data::Status>;
  const auto key = godot::String::utf8(
      reinterpret_cast<const char*>(name.get_data()), name.get_size());
  const godot::Dictionary paths =
      godot::ProjectSettings::get_singleton()->get_setting_with_override(
          "ttx/imports");
  if (!paths.has(key)) {
    return Ttx::Data::Status::Unsupported;
  }
  const auto location =
      godot::ProjectSettings::get_singleton()->globalize_path(paths[key]);
  // Configuration selects the current artifact. Existing bindings keep their
  // earlier library, even when a project replaces or removes the alias.
  for (const auto& entry : loaded.get_view()) {
    if (entry.path == location) {
      return entry.library;
    }
  }
  const auto path = location.utf8();
  Memory::Allocator::Arena errors;
  return Library::open(
             {reinterpret_cast<const U8*>(path.get_data()),
              Count(path.length())},
             errors)
      .visit(
          [&](Library& library) -> Result {
            loaded.emplace(Loaded(location, library));
            return library;
          },
          [](Core::View::Bytes) -> Result {
            return Ttx::Data::Status::IoError;
          });
}

auto Modules::Imports::load(
    Core::View::Bytes name,
    void* receiver,
    void (*receive)(void*, ttx_abstract)) const -> Binding::Status {
  return open(name).visit(
      [&](Library& library) {
        auto observe = [&](Query query) {
          return query.bind<Ttx::Concept::Abstract>().visit(
              [&](Ttx::Concept::Abstract subject) {
                receive(receiver, subject.get_abi());
                return Binding::Status::Satisfied;
              },
              [](Binding::Failure failure) {
                return static_cast<Binding::Status>(failure);
              });
        };
        return library.visit(get_query(), Receiver(observe));
      },
      [](Ttx::Data::Status status) {
        return status == Ttx::Data::Status::Unsupported
                   ? Binding::Status::Unknown
                   : Binding::Status::Rejected;
      });
}

auto Modules::Imports::supports(System::Uuid id) const -> Binding::Status {
  return id == Ttx::Concept::Capabilities::Import::contract_id
             ? Binding::Status::Satisfied
             : Binding::Status::Unknown;
}

auto Modules::Imports::bind_interface(
    System::Uuid id,
    Ttx::Data::Form::Storage target) const -> Binding::Status {
  return id == Ttx::Concept::Capabilities::Import::contract_id
             ? Binding::provide<Ttx::Concept::Capabilities::Import>(
                   Ttx::Concept::Capabilities::Import::provide(*this).get_abi(),
                   target)
             : Binding::Status::Unknown;
}
