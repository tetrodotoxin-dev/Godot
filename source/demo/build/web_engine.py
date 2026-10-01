# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

# This template hosts the lab's 2D scene and its extension. Keep normal GUI
# controls because the lab uses OptionButton and rich layout containers.
# The module list is the engine capability budget, independent of which TTX
# providers a project imports. Additional engine features require a new build.
disable_3d = True
disable_physics_2d = True
disable_physics_3d = True
disable_xr = True

# GDScript providers and Resource publication use the standard scene system.
# FreeType and the fallback text server retain the lab's Latin text rendering.
# SVG remains available for project icons and other ordinary 2D resources.
modules_enabled_by_default = False
module_gdscript_enabled = True
module_freetype_enabled = True
module_text_server_fb_enabled = True
module_svg_enabled = True

# C++ providers may catch failures from their own numerical libraries. The
# runtime supplies Wasm exception tags and support routines for those modules.
# Engine translation units keep their existing compilation settings.
linkflags = "-fwasm-exceptions"
