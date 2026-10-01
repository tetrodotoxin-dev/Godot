# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")
load("@rules_pkg//pkg:providers.bzl", "PackageFilesInfo")

def _sdk_headers_impl(ctx):
    files = {}
    for header in ctx.attr.contracts[CcInfo].compilation_context.headers.to_list():
        if not header.is_source:
            continue
        path = header.short_path
        if "/headers/include/" in path:
            path = path.split("/headers/include/", 1)[1]
        elif path.startswith("source/godot_ttx/contracts/"):
            path = path.removeprefix("source/")
        else:
            fail("Unexpected public SDK header: " + path)
        files["include/" + path] = header
    return [
        DefaultInfo(files = depset(files.values())),
        PackageFilesInfo(dest_src_map = files, attributes = {"mode": "0644"}),
    ]

sdk_headers = rule(
    implementation = _sdk_headers_impl,
    attrs = {"contracts": attr.label(mandatory = True, providers = [CcInfo])},
)

def _sdk_libraries_impl(ctx):
    files = {}
    for linker in ctx.attr.library[CcInfo].linking_context.linker_inputs.to_list():
        for library in linker.libraries:
            for artifact in [library.dynamic_library, library.interface_library]:
                if artifact:
                    name = "godot_ttx.lib" if artifact.basename == "runtime.if.lib" else artifact.basename
                    files["addons/godot_ttx/" + name] = artifact
    return [
        DefaultInfo(files = depset(files.values())),
        PackageFilesInfo(dest_src_map = files, attributes = {"mode": "0644"}),
    ]

sdk_libraries = rule(
    implementation = _sdk_libraries_impl,
    attrs = {"library": attr.label(mandatory = True, providers = [CcInfo])},
)
