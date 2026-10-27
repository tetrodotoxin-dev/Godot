// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "toolchain/memory/lifetime.hpp"

#include "perimortem/memory/allocator/global.hpp"

#include "godot/modules/classes.hpp"

#include "godot_cpp/classes/project_settings.hpp"
#include "godot_cpp/godot.hpp"

static Godot::Extension::Modules::Classes* classes = nullptr;

static auto initialize(godot::ModuleInitializationLevel level) -> void {
  if (level != godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
    return;
  }

  const auto allocation = Perimortem::Memory::Allocator::Global::allocate(
      sizeof(Godot::Extension::Modules::Classes));
  classes = &Toolchain::Memory::Lifetime::construct<
      Godot::Extension::Modules::Classes>(allocation.get_data(), [] {
    return Godot::Extension::Modules::Classes();
  });

  auto* settings = godot::ProjectSettings::get_singleton();
  if (!settings->has_setting("ttx/modules")) {
    settings->set_setting("ttx/modules", godot::Array());
  }

  classes->load(settings->get_setting_with_override("ttx/modules"));
}

static auto terminate(godot::ModuleInitializationLevel level) -> void {
  if (level != godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
    return;
  }

  classes->~Classes();
  Perimortem::Memory::Allocator::Global::release(
      Perimortem::Core::Data::cast<U8>(classes));
  classes = nullptr;
}

C_LINKAGE GDExtensionBool GDE_EXPORT godot_ttx_init(
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
