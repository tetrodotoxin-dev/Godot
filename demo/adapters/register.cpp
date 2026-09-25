// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>

#include "demo/adapters/cuda/ttx_cuda_program.hpp"
#include "demo/adapters/imaging/ttx_image.hpp"

// The image graph and CUDA Resources belong to this demonstration. Keeping
// their registration here lets the TTX extension load other modules without
// linking the lab's image pipeline or choosing its compute providers.
static void initialize(godot::ModuleInitializationLevel level) {
  if (level == godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
    godot::ClassDB::register_class<Godot::Demo::Adapters::Imaging::TtxImage>();
    godot::ClassDB::register_class<
        Godot::Demo::Adapters::Cuda::TtxCudaBuffer>();
    godot::ClassDB::register_class<
        Godot::Demo::Adapters::Cuda::TtxCudaKernel>();
    godot::ClassDB::register_class<
        Godot::Demo::Adapters::Cuda::TtxCudaProgram>();
  }
}

PERIMORTEM_C GDExtensionBool GDE_EXPORT godot_demo_init(
    GDExtensionInterfaceGetProcAddress address,
    GDExtensionClassLibraryPtr library,
    GDExtensionInitialization* initialization) {
  godot::GDExtensionBinding::InitObject init(address, library, initialization);
  init.register_initializer(initialize);
  init.set_minimum_library_initialization_level(
      godot::MODULE_INITIALIZATION_LEVEL_SCENE);
  return init.init();
}
