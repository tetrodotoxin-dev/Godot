# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")

def _runtime_libraries(ctx):
    # The linker supplies the deployed names. Interface libraries such as the
    # host GPU driver stay on the host rather than entering the addon archive.
    libraries = [
        library.dynamic_library
        for dependency in ctx.attr.deps
        for linker in dependency[CcInfo].linking_context.linker_inputs.to_list()
        for library in linker.libraries
        if library.dynamic_library
    ]
    return [DefaultInfo(files = depset(libraries))]

runtime_libraries = rule(
    implementation = _runtime_libraries,
    attrs = {"deps": attr.label_list(providers = [CcInfo])},
)
