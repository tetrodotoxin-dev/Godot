# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_import.bzl", "cc_import")
load("@rules_cc//cc:cc_shared_library.bzl", "cc_shared_library")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

# The bridge projects negotiated interfaces into Godot classes and values.
# Providers use godot_ttx, which contains only the shared TTX contracts.
cc_library(
    name = "extension",
    srcs = glob([
        "classes/*.cpp",
        "modules/*.cpp",
        "values/*.cpp",
    ]),
    hdrs = glob([
        "classes/*.hpp",
        "modules/*.hpp",
        "values/*.hpp",
    ]),
    copts = select({"//:windows": ["/clang:-frtti"], "//conditions:default": ["-frtti", "-fvisibility=hidden"]}),
    deps = [
        ":godot_ttx",
    ] + select({
        "//:web": ["@godot_cpp_web//:godot_cpp"],
        "//conditions:default": ["@godot_cpp"],
    }),
)

cc_library(
    name = "registration",
    srcs = ["register.cpp"],
    copts = select({"//:windows": ["/clang:-frtti"], "//conditions:default": ["-frtti", "-fvisibility=hidden"]}),
    deps = [":extension"],
)

cc_shared_library(
    name = "runtime",
    shared_lib_name = select({"//:windows": "godot_ttx.dll", "//conditions:default": "libgodot_ttx.so"}),
    win_def_file = "//source/godot_ttx/build:godot_ttx.def",
    tags = ["__DONT_DEPEND_ON_DEF_PARSER__"],
    user_link_flags = select({
        "//:web": [],
        "//:windows": [],
        "//conditions:default": ["-Wl,-z,defs", "-Wl,-rpath,$ORIGIN", "-Wl,-soname,libgodot_ttx.so"],
    }),
    deps = ["//source/godot_ttx/contracts:implementation"],
)

# Godot loads this binary through its GDExtension entry. Provider libraries
# link godot_ttx and leave the engine binding implementation inside this host.
cc_shared_library(
    name = "bridge",
    shared_lib_name = select({
        "//:windows": "ttx_bridge.dll",
        "//conditions:default": "libttx_bridge.so",
    }),
    win_def_file = "//source/godot_ttx/build:ttx_bridge.def",
    tags = ["__DONT_DEPEND_ON_DEF_PARSER__"],
    user_link_flags = select({
        "//:web": [],
        "//:windows": [],
        "//conditions:default": ["-Wl,-z,defs", "-Wl,-rpath,$ORIGIN", "-Wl,-soname,libttx_bridge.so"],
    }),
    deps = [":registration"],
)

filegroup(
    name = "interface",
    srcs = [":runtime"],
    output_group = "interface_library",
)

cc_import(
    name = "godot_ttx",
    shared_library = ":runtime",
    interface_library = select({"//:windows": ":interface", "//conditions:default": None}),
    deps = ["//source/godot_ttx/contracts:headers", "@ttx"],
)

genrule(
    name = "addon",
    srcs = [
        "godot_ttx.gdextension",
        ":runtime",
        ":bridge",
        "@ttx//:build",
        "@perimortem//:build",
    ] + glob(["providers/*"]),
    outs = ["godot_ttx.tar"],
    cmd = "tar -chf $@ --transform='s|^source/godot_ttx/providers/|providers/|' " +
          "--transform='s|^.*/\\(lib[^/]*\\.so\\)|\\1|' " +
          "--transform='s|^source/godot_ttx/godot_ttx.gdextension|godot_ttx.gdextension|' $(SRCS)",
)

load("@rules_pkg//pkg:mappings.bzl", "pkg_files", "strip_prefix")
load("@rules_pkg//pkg:zip.bzl", "pkg_zip")
load("//source/godot_ttx/build:package.bzl", "sdk_headers", "sdk_libraries")

sdk_headers(
    name = "sdk_headers",
    contracts = "//source/godot_ttx/contracts:headers",
)

sdk_libraries(
    name = "sdk_libraries",
    library = ":godot_ttx",
)

pkg_files(
    name = "sdk_manifest",
    srcs = ["godot_ttx.gdextension", ":bridge"],
    prefix = "addons/godot_ttx",
)

pkg_files(
    name = "sdk_exporter",
    srcs = glob(["providers/*"]),
    strip_prefix = strip_prefix.from_pkg(),
    prefix = "addons/godot_ttx",
)

pkg_files(
    name = "sdk_notices",
    srcs = ["//:LICENSE", "@godot_cpp//:LICENSE.md", "licenses/TTX.txt", "licenses/Perimortem.txt"],
    renames = {"//:LICENSE": "Godot-TTX.txt", "@godot_cpp//:LICENSE.md": "godot-cpp.txt", "licenses/TTX.txt": "TTX.txt", "licenses/Perimortem.txt": "Perimortem.txt"},
    prefix = "licenses",
)

pkg_files(
    name = "sdk_build",
    srcs = ["//source/godot_ttx/build:provider.BUILD.bazel"],
    renames = {"//source/godot_ttx/build:provider.BUILD.bazel": "BUILD.bazel"},
)

pkg_zip(
    name = "sdk",
    srcs = [":sdk_headers", ":sdk_libraries", ":sdk_manifest", ":sdk_exporter", ":sdk_notices", ":sdk_build"],
    out = "godot-ttx-sdk.zip",
    package_file_name = select({"//:windows": "godot-ttx-sdk-windows-x86_64.zip", "//conditions:default": "godot-ttx-sdk-linux-x86_64.zip"}),
)
