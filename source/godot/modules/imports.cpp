// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "godot/modules/imports.hpp"

#include "perimortem/core/view/null_terminated.hpp"

#include "godot_cpp/classes/project_settings.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "ttx/data/transfer.h"
#include "ttx/graph/policies/none.h"
#include "ttx/semantic/negotiation/library.h"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Ttx::Graph;
using namespace Ttx::Semantic;
using namespace Godot::Extension;

auto Modules::Imports::open(const godot::String& path)
    -> Storage::Option<Abstract> {
  for (Count i = 0; i < loaded.get_size(); ++i) {
    const auto& entry = loaded.get_data()[i];
    if (entry.path == path) {
      return entry.abstract;
    }
  }

#ifdef TOOLCHAIN_WASM

  // Web export preloads provider modules beside the engine under their
  // filenames. The resource path remains in project settings so the exporter
  // can discover the file, while the loader selects that preloaded filename.
  const auto location = path.get_file();

#else

  const auto location =
      path.begins_with("res://")
          ? godot::ProjectSettings::get_singleton()->globalize_path(path)
          : path;

#endif

  const auto encoded = location.utf8();
  const View::Bytes bytes(
      perimortem_view_bytes{
        Data::cast<const U8>(encoded.get_data()), Count(encoded.length())});

  Allocator::Arena errors;
  Storage::Option<Abstract> result;
  Process::Library::open(bytes, errors)
      .visit(
          [&](Process::Library& library) {
            library.symbol("ttx_entry"_view, errors)
                .visit(
                    [&](void* symbol) {
                      const auto& published =
                          *Data::cast<const ttx_storage>(symbol);
                      ttx_semantic_query entry{};
                      const ttx_storage destination{
                        ttx_semantic_query_representation(),
                        Data::cast<U8>(&entry), sizeof(entry)};

                      if (ttx_transfer(&published, &destination) !=
                              TTX_DATA_SUCCESS ||
                          !entry.bind || !entry.supports) {
                        godot::UtilityFunctions::push_warning(
                            "TTX entry contract refused: ", location);
                        return;
                      }

                      Negotiation::Query(entry).bind<Abstract>().visit(
                          [&](Abstract abstract) {
                            loaded.insert(
                                Loaded{path, Data::take(library), abstract});
                            result = abstract;
                          },

                          [&](Negotiation::Status status) {
                            godot::UtilityFunctions::push_warning(
                                "TTX Abstract binding refused: ", location,
                                " status ", U8(status));
                          });
                    },

                    [&](View::Bytes error) {
                      godot::UtilityFunctions::push_warning(
                          "TTX entry: ", location, ": ",
                          godot::String::utf8(
                              Data::cast<const char>(error.get_data()),
                              error.get_size()));
                    });
          },

          [&](View::Bytes error) {
            godot::UtilityFunctions::push_warning(
                "TTX module: ", location, ": ",
                godot::String::utf8(
                    Data::cast<const char>(error.get_data()),
                    error.get_size()));
          });

  return result;
}

auto Modules::Imports::load(const godot::Array& paths) -> void {
  for (S64 i = 0; i < paths.size(); ++i) {
    if (paths[i].get_type() == godot::Variant::STRING) {
      open(paths[i]);
    }
  }
}

auto Modules::Imports::get_abstract() -> Abstract {
  static const ttx_abstract_ops operations = {
    supports, bind, route, visit, state};

  return Abstract(this, operations);
}

auto Modules::Imports::supports(void* context, perimortem_uuid id)
    -> ttx_negotiation_status {
  if (!id.high && id.low <= 1) {
    return TTX_NEGOTIATION_REJECTED;
  }

  return Identity::Uuid(id) == Abstract::contract_id ? TTX_NEGOTIATION_SATISFIED
                                                     : TTX_NEGOTIATION_UNKNOWN;
}

auto Modules::Imports::bind(void* context, perimortem_uuid id, void* output)
    -> ttx_negotiation_status {
  const auto status = supports(context, id);
  if (status == TTX_NEGOTIATION_SATISFIED) {
    *Data::cast<ttx_abstract>(output) =
        Data::cast<Imports>(context)->get_abstract().get_abi();
  }

  return status;
}

auto Modules::Imports::route(void* context, perimortem_view_bytes path)
    -> ttx_abstract {
  auto& imports = *Data::cast<Imports>(context);
  const auto name =
      godot::String::utf8(Data::cast<const char>(path.data), path.size);
  for (Count i = 0; i < imports.loaded.get_size(); ++i) {
    const auto& entry = imports.loaded.get_data()[i];
    if (entry.path == name) {
      return entry.abstract.get_abi();
    }
  }

  return ttx_none();
}

auto Modules::Imports::visit(void* context, ttx_graph_visitor callback)
    -> void {
  auto& imports = *Data::cast<Imports>(context);
  for (Count i = 0; i < imports.loaded.get_size(); ++i) {
    const auto& entry = imports.loaded.get_data()[i];
    const auto path = entry.path.utf8();
    callback.callback(
        callback.callback_context,
        perimortem_view_bytes{
          Data::cast<const U8>(path.get_data()), Count(path.length())},

        entry.abstract.get_abi());
  }
}

auto Modules::Imports::state(void* context) -> ttx_abstract_state {
  return TTX_ABSTRACT_PRESENT;
}
