// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "perimortem/system/process/library.hpp"

#include "ttx/graph/abstract.h"

#include "godot_cpp/core/memory.hpp"
#include "godot_cpp/variant/string.hpp"

namespace Godot::Demo::Adapters::Cuda {

// Program, Kernel and Buffer keep the loaded module through their final
// retained release. This host reference count shares executable availability.
// each resource separately consumes its own provider acquisition.
class Library {
 public:
  static auto open(const godot::String& path, godot::String& error) -> Library*;
  void retain();
  void release();
  auto get_abstract() const -> ttx_abstract { return abstract; }

 private:
  Library(Perimortem::System::Process::Library module, ttx_abstract abstract);
  Count references = 1;
  Perimortem::System::Process::Library module;
  ttx_abstract abstract;
};

}  // namespace Godot::Demo::Adapters::Cuda
