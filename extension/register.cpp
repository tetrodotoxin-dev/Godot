// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>

#include "extension/ttx_image.hpp"
#include "extension/ttx_sampler.hpp"

static void initialize(godot::ModuleInitializationLevel level) {
  if (level == godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
    godot::ClassDB::register_class<Godot::Extension::TtxImage>();
    godot::ClassDB::register_class<Godot::Extension::TtxSampler>();
  }
}

static void terminate(godot::ModuleInitializationLevel) {}

// GDExtension supplies the engine boundary regardless of how either binary was
// built. Registration exposes the host object without changing the TTX ABI.
extern "C" GDExtensionBool GDE_EXPORT godot_ttx_init(
    GDExtensionInterfaceGetProcAddress address,
    GDExtensionClassLibraryPtr library,
    GDExtensionInitialization* initialization) {
  godot::GDExtensionBinding::InitObject init(address, library, initialization);
  init.register_initializer(initialize);
  init.register_terminator(terminate);
  init.set_minimum_library_initialization_level(
      godot::MODULE_INITIALIZATION_LEVEL_SCENE);
  return init.init();
}
