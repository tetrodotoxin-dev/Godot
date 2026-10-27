// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "toolchain/toolchain.h"

#include "godot_cpp/classes/project_settings.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/godot.hpp"

#ifdef GODOT_CUDA_ADAPTER
#include "demo/adapters/cuda/ttx_cuda_program.hpp"
#endif
#include "demo/adapters/imaging/ttx_image.hpp"

// The image graph and CUDA Resources belong to this demonstration. Keeping
// their registration here lets the TTX extension load other modules without
// linking the lab's image pipeline or choosing its compute providers.
static void initialize(godot::ModuleInitializationLevel level) {
  if (level == godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
    godot::ClassDB::register_class<Godot::Demo::Adapters::Imaging::TtxImage>();

#ifdef GODOT_CUDA_ADAPTER

    godot::ClassDB::register_class<
        Godot::Demo::Adapters::Cuda::TtxCudaBuffer>();
    godot::ClassDB::register_class<
        Godot::Demo::Adapters::Cuda::TtxCudaKernel>();
    godot::ClassDB::register_class<
        Godot::Demo::Adapters::Cuda::TtxCudaProgram>();

#endif
  }
}

static void terminate(godot::ModuleInitializationLevel level) {
  if (level == godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
  }
}

C_LINKAGE GDExtensionBool GDE_EXPORT godot_demo_init(
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
