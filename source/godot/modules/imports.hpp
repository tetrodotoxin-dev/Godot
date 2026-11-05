// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "perimortem/memory/global/vector.hpp"
#include "perimortem/system/process/library.hpp"

#include "godot_cpp/variant/array.hpp"
#include "godot_cpp/variant/string.hpp"
#include "ttx/graph/abstract.hpp"

namespace Godot::Extension::Modules {

// Imports keeps each loaded module available through the last use of its
// graph. Entry records pass Data transfer and Semantic negotiation before any
// Graph operation is called. The host root exposes accepted modules by their
// configured path. A failed module leaves the other roots usable.
class Imports {
 public:
  auto load(const godot::Array& paths) -> void;
  auto open(const godot::String& path)
      -> Perimortem::Core::Storage::Option<Ttx::Graph::Abstract>;
  auto get_abstract() -> Ttx::Graph::Abstract;

 private:
  struct Loaded {
    godot::String path;
    Perimortem::System::Process::Library library;
    Ttx::Graph::Abstract abstract;
  };

  static auto supports(void* context, perimortem_uuid id)
      -> ttx_negotiation_status;
  static auto bind(void* context, perimortem_uuid id, void* output)
      -> ttx_negotiation_status;
  static auto route(void* context, perimortem_view_bytes path) -> ttx_abstract;
  static auto visit(void* context, ttx_graph_visitor callback) -> void;
  static auto state(void* context) -> ttx_abstract_state;
  Perimortem::Memory::Global::Vector<Loaded> loaded;
};

}  // namespace Godot::Extension::Modules
