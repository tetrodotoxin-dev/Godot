// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <godot_cpp/variant/array.hpp>

#include "perimortem/memory/dynamic/vector.hpp"

#include "extension/modules/imports.hpp"
#include "ttx/semantic/ownership/publication.hpp"

namespace Godot::Extension::Modules {

// Classes drives startup compilation and then owns only emitted terminals.
// Each discovery publication ends before its class is registered. Imports
// remains available to the independent runtime factories until all classes
// and instances have been released during scene teardown.
class Classes {
 public:
  explicit Classes(godot::Dictionary paths) : imports(paths) {}
  ~Classes();
  auto load(const godot::Array& configuration) -> void;

 private:
  // Compiling one configured declaration transfers a prepared terminal into
  // this lifetime owner. Discovery locals must die before publish is invoked.
  auto compile(const godot::Dictionary& configuration)
      -> Perimortem::Utility::Result<
          Ttx::Semantic::Ownership::Publication,
          Perimortem::Core::View::Bytes>;
  Imports imports;
  Perimortem::Memory::Dynamic::Vector<Ttx::Semantic::Ownership::Publication>
      terminals;
};

}  // namespace Godot::Extension::Modules
