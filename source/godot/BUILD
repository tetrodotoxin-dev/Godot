# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_pkg//pkg:mappings.bzl", "pkg_files", "strip_prefix")
load("@rules_pkg//pkg:tar.bzl", "pkg_tar")
load("@rules_pkg//pkg:zip.bzl", "pkg_zip")
load("@tetro_toolchain//:defs.bzl", "WINDOWS", "WEB", "library")
load("//source/godot/build:package.bzl", "sdk_libraries")

package(default_visibility = ["//visibility:public"])

library(
    name = "bridge",
    module = "godot",
    linkage = "shared",
    srcs = ["register.cpp"] + glob(["classes/*.cpp", "classes/*.hpp", "modules/*.cpp", "modules/*.hpp", "values/*.cpp", "values/*.hpp"]),
    includes = [".."],
    # The host SDK supplies placement operations before dependency headers.
    # Toolchain detects those declarations without changing its standalone API.
    copts = select({WINDOWS: ["/clang:-frtti", "/clang:-fvisibility-inlines-hidden", "/FIgodot_cpp/core/memory.hpp"], "//conditions:default": ["-frtti", "-fvisibility-inlines-hidden", "-include", "godot_cpp/core/memory.hpp"]}),
    shared_lib_name = select({WINDOWS: "ttx_bridge.dll", "//conditions:default": "libttx_bridge.so"}),
    # Godot's Web loader invokes this linker-generated function to run C++
    # static initialization before calling the extension entry point.
    linkopts = select({WEB: ["-Wl,--export=__wasm_call_ctors"], "//conditions:default": []}),
    win_def_file = "//source/godot/build:ttx_bridge.def",
    implementation_deps = ["@ttx//:graph", "@perimortem//:system"] + select({WEB: ["@godot_cpp_web//:godot_cpp"], "//conditions:default": ["@godot_cpp//:godot_cpp"]}),
)

sdk_libraries(name = "sdk_libraries", library = ":bridge")

pkg_files(
    name = "sdk_manifest",
    srcs = ["godot_ttx.gdextension"],
    prefix = "addons/godot_ttx",
)

pkg_files(
    name = "sdk_exporter",
    srcs = glob(["providers/*"]),
    prefix = "addons/godot_ttx",
    strip_prefix = strip_prefix.from_pkg(),
)

pkg_files(
    name = "sdk_notices",
    srcs = ["licenses/Perimortem.txt", "licenses/TTX.txt", "//:LICENSE", "@godot_cpp//:LICENSE.md"],
    prefix = "licenses",
    renames = {"//:LICENSE": "Godot-TTX.txt", "@godot_cpp//:LICENSE.md": "godot-cpp.txt", "licenses/TTX.txt": "TTX.txt", "licenses/Perimortem.txt": "Perimortem.txt"},
)

pkg_zip(name = "sdk", srcs = [":sdk_exporter", ":sdk_libraries", ":sdk_manifest", ":sdk_notices"], out = "godot-ttx-sdk.zip")
pkg_tar(name = "addon", srcs = [":sdk_exporter", ":sdk_libraries", ":sdk_manifest"], strip_prefix = "addons/godot_ttx", extension = "tar")
