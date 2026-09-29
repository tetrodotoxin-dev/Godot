// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <godot_cpp/variant/array.hpp>

#include "perimortem/memory/dynamic/vector.hpp"

#include "extension/classes/class.hpp"
#include "extension/modules/imports.hpp"

namespace Godot::Extension::Modules {

// Classes exports each configured declaration and owns the resulting Godot
// registrations. It releases them before its import service closes the
// libraries used by their constructors and runtime instances.
class Classes {
 public:
  Classes(const Classes&) = delete;
  auto operator=(const Classes&) -> Classes& = delete;
  Classes() = default;
  ~Classes();
  auto load(const godot::Array& configuration) -> void;

 private:
  auto compile(const godot::Dictionary& configuration)
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes>;
  Imports imports;
  Perimortem::Memory::Dynamic::Vector<::Godot::Extension::Classes::Class*>
      terminals;
};

}  // namespace Godot::Extension::Modules
