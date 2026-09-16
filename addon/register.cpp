// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>

#include "perimortem/core/bibliotheca.hpp"

#include "adapters/cuda/ttx_cuda_program.hpp"
#include "adapters/imaging/ttx_image.hpp"
#include "gdextension/modules/classes.hpp"

static Gdextension::Modules::Classes* classes = nullptr;

static void initialize(godot::ModuleInitializationLevel level) {
  if (level == godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
    godot::ClassDB::register_class<Godot::Adapters::Imaging::TtxImage>();
    godot::ClassDB::register_class<Godot::Adapters::Cuda::TtxCudaBuffer>();
    godot::ClassDB::register_class<Godot::Adapters::Cuda::TtxCudaKernel>();
    godot::ClassDB::register_class<Godot::Adapters::Cuda::TtxCudaProgram>();
    auto storage = Perimortem::Core::Bibliotheca::check_out(
        sizeof(Gdextension::Modules::Classes));
    classes = new (storage.ptr, Perimortem::Core::Placement::Construct)
        Gdextension::Modules::Classes(
            godot::ProjectSettings::get_singleton()->get_setting(
                "ttx/imports", godot::Dictionary()));
    const auto setting = godot::ProjectSettings::get_singleton()->get_setting(
        "ttx/classes", godot::Array());
    classes->load(setting);
  }
}

static void terminate(godot::ModuleInitializationLevel level) {
  if (level == godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
    classes->~Classes();
    Perimortem::Core::Bibliotheca::remit(reinterpret_cast<U8*>(classes));
    classes = nullptr;
  }
}

// GDExtension supplies the engine boundary regardless of how either binary was
// built. Registration exposes the host object without changing the TTX ABI.
PERIMORTEM_C GDExtensionBool GDE_EXPORT godot_ttx_init(
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
