// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "gdextension/modules/imports.hpp"

#include <godot_cpp/classes/project_settings.hpp>

using namespace Gdextension;
using namespace Perimortem;

auto Gdextension::Modules::Imports::open(Core::View::Bytes name) const
    -> Utility::Result<Ttx::Concept::Modules::Module, Ttx::Data::Status> {
  using Result =
      Utility::Result<Ttx::Concept::Modules::Module, Ttx::Data::Status>;
  const auto key = godot::String::utf8(
      reinterpret_cast<const char*>(name.get_data()), name.get_size());
  if (!paths.has(key)) {
    return Ttx::Data::Status::Unsupported;
  }

  auto path = godot::ProjectSettings::get_singleton()
                  ->globalize_path(paths[key])
                  .utf8();
  Memory::Allocator::Arena errors;
  return Ttx::Concept::Modules::Module::load(
             Core::View::Bytes(
                 reinterpret_cast<const U8*>(path.get_data()), path.length()),
             errors)
      .visit(
          [](Ttx::Concept::Modules::Module& module) -> Result {
            return Core::Data::take(module);
          },
          [](Core::View::Bytes) -> Result {
            return Ttx::Data::Status::IoError;
          });
}

auto Gdextension::Modules::Imports::get_query() const
    -> Ttx::Semantic::Negotiation::Query {
  return Ttx::Semantic::Negotiation::Query(
      {this,
       [](const void* source, perimortem_uuid id,
          ttx_storage requested) -> ttx_binding_status {
         if (System::Uuid(id) != Ttx::Concept::Modules::Import::contract_id) {
           return TTX_BINDING_UNSUPPORTED;
         }

         static const ttx_import_operations operations = {
           [](const void* source, perimortem_view_bytes name,
              ttx_module* output) -> ttx_data_status {
             return static_cast<const Imports*>(source)
                 ->open(Core::View::Bytes(name.data, name.size))
                 .visit(
                     [&](Ttx::Concept::Modules::Module& module)
                         -> ttx_data_status {
                       *output = module.take();
                       return TTX_DATA_SUCCESS;
                     },
                     [](Ttx::Data::Status status) {
                       return static_cast<ttx_data_status>(status);
                     });
           },
         };
         const ttx_import api = {source, &operations};
         return ttx_binding_provide(
             ttx_import_representation(), &api, requested);
       },
       [](const void*, perimortem_uuid id) -> ttx_binding_status {
         return System::Uuid(id) == Ttx::Concept::Modules::Import::contract_id
                    ? TTX_BINDING_SATISFIED
                    : TTX_BINDING_UNSUPPORTED;
       }});
}
