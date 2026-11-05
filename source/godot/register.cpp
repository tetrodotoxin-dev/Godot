// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "toolchain/memory/lifetime.hpp"

#include "perimortem/memory/allocator/global.hpp"

#include "godot/modules/registry.hpp"
#include "godot_cpp/classes/project_settings.hpp"
#include "godot_cpp/godot.hpp"

using namespace Godot::Extension;
using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Toolchain::Memory;

static Modules::Registry* classes = nullptr;

static auto initialize(godot::ModuleInitializationLevel level) -> void {
  if (level != godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
    return;
  }

  const auto allocation =
      Allocator::Global::allocate(sizeof(Modules::Registry));
  classes = &Lifetime::construct<Modules::Registry>(
      allocation.get_data(), [] { return Modules::Registry(); });

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

  classes->~Registry();
  Allocator::Global::release(Data::cast<U8>(classes));
  classes = nullptr;
}

C_LINKAGE GDE_EXPORT auto godot_ttx_init(
    GDExtensionInterfaceGetProcAddress address,
    GDExtensionClassLibraryPtr library,
    GDExtensionInitialization* initialization) -> GDExtensionBool {
  godot::GDExtensionBinding::InitObject init(address, library, initialization);
  init.register_initializer(initialize);
  init.register_terminator(terminate);
  init.set_minimum_library_initialization_level(
      godot::MODULE_INITIALIZATION_LEVEL_SCENE);
  return init.init();
}
