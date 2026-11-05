# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")
load("@rules_pkg//pkg:providers.bzl", "PackageFilesInfo")

def _sdk_libraries_impl(ctx):
    files = {}
    primary = None
    for linker in ctx.attr.library[CcInfo].linking_context.linker_inputs.to_list():
        for library in linker.libraries:
            artifact = library.resolved_symlink_dynamic_library or library.dynamic_library
            if artifact:
                files["addons/godot_ttx/" + artifact.basename] = artifact
                if linker.owner == ctx.attr.library.label:
                    primary = artifact
    if ctx.attr.manifest:
        if not primary:
            fail("Module packaging requires the SDK's declared primary library")
        manifest = ctx.actions.declare_file(ctx.label.name + ".json")
        ctx.actions.write(manifest, json.encode({
            "library": primary.basename,
            "runtime": [{"library": file.basename} for file in files.values() if file != primary],
        }) + "\n")
        files["addons/godot_ttx/" + primary.basename + ".json"] = manifest
    return [
        DefaultInfo(files = depset(files.values())),
        PackageFilesInfo(dest_src_map = files, attributes = {"mode": "0644"}),
    ]

sdk_libraries = rule(
    implementation = _sdk_libraries_impl,
    attrs = {
        "library": attr.label(mandatory = True, providers = [CcInfo]),
        "manifest": attr.bool(default = False),
    },
)
