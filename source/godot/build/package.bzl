# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")
load("@rules_pkg//pkg:providers.bzl", "PackageFilesInfo")

def _sdk_libraries_impl(ctx):
    files = {}
    for linker in ctx.attr.library[CcInfo].linking_context.linker_inputs.to_list():
        for library in linker.libraries:
            artifact = library.resolved_symlink_dynamic_library or library.dynamic_library
            if artifact:
                files["addons/godot_ttx/" + artifact.basename] = artifact
    return [
        DefaultInfo(files = depset(files.values())),
        PackageFilesInfo(dest_src_map = files, attributes = {"mode": "0644"}),
    ]

sdk_libraries = rule(
    implementation = _sdk_libraries_impl,
    attrs = {"library": attr.label(mandatory = True, providers = [CcInfo])},
)
