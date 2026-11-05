# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_pkg//pkg:tar.bzl", "pkg_tar")
load("@tetro_toolchain//:defs.bzl", "format", "snapshot", "vscode")
load("//source/godot/build:package.bzl", "sdk_libraries")
load("//validation/compute:test.bzl", "godot_runner")

package(default_visibility = ["//visibility:public"])

config_setting(
    name = "cuda_enabled",
    define_values = {"godot_cuda": "1"},
)

config_setting(
    name = "web",
    constraint_values = ["@platforms//cpu:wasm32"],
)

exports_files(["LICENSE"])

alias(
    name = "sdk",
    actual = "//source/godot:sdk",
    tags = ["manual"],
)

sdk_libraries(
    name = "demo_cpu_libraries",
    library = "@compute//:cpu",
    manifest = True,
)

sdk_libraries(
    name = "demo_cuda_libraries",
    library = "@compute//:cuda",
    manifest = True,
    tags = ["manual"],
)

pkg_tar(
    name = "demo_addon",
    srcs = [
        ":demo_cpu_libraries",
        "//source/godot:sdk_exporter",
        "//source/godot:sdk_libraries",
        "//source/godot:sdk_manifest",
    ] + select({":cuda_enabled": [":demo_cuda_libraries"], "//conditions:default": []}),
    extension = "tar",
)

filegroup(
    name = "demo_web_payload",
    srcs = [
        ":demo_cpu_libraries",
        "//source/godot:sdk_libraries",
    ],
)

config_setting(
    name = "windows",
    constraint_values = ["@platforms//os:windows"],
)

godot_runner(
    name = "tests",
    script = "//validation/compute:test.sh",
    data = [
        "//validation/compute:addon",
        "//validation/compute:project",
    ],
)

format()
snapshot()
vscode()
