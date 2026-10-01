// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>

#include "demo/adapters/cuda/ttx_cuda_program.hpp"
#include "demo/adapters/imaging/ttx_image.hpp"
#include "demo/adapters/imports.hpp"
#include "godot_ttx/modules/imports.hpp"

static Godot::Extension::Modules::Imports* import_service = nullptr;

auto Godot::Demo::Adapters::imports() -> Ttx::Concept::Capabilities::Import {
  return Ttx::Concept::Capabilities::Import::provide(*import_service);
}

// The image graph and CUDA Resources belong to this demonstration. Keeping
// their registration here lets the TTX extension load other modules without
// linking the lab's image pipeline or choosing its compute providers.
static void initialize(godot::ModuleInitializationLevel level) {
  if (level == godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
    import_service = new Godot::Extension::Modules::Imports();
    godot::ClassDB::register_class<Godot::Demo::Adapters::Imaging::TtxImage>();
    godot::ClassDB::register_class<
        Godot::Demo::Adapters::Cuda::TtxCudaBuffer>();
    godot::ClassDB::register_class<
        Godot::Demo::Adapters::Cuda::TtxCudaKernel>();
    godot::ClassDB::register_class<
        Godot::Demo::Adapters::Cuda::TtxCudaProgram>();
  }
}

static void terminate(godot::ModuleInitializationLevel level) {
  if (level == godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
    delete import_service;
    import_service = nullptr;
  }
}

PERIMORTEM_C GDExtensionBool GDE_EXPORT godot_demo_init(
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
