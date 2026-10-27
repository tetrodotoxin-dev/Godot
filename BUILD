# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@tetro_toolchain//:defs.bzl", "format", "snapshot", "vscode")

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

config_setting(
    name = "windows",
    constraint_values = ["@platforms//os:windows"],
)

cc_binary(
    name = "tests",
    testonly = True,
    srcs = glob(["validation/tests/*.cpp"]),
    data = ["//source/demo/counter:runtime"],
    deps = ["@ttx//:graph", "@perimortem//:system", "@tetro_toolchain//source/toolchain/validation:test"],
)

format()
snapshot()
vscode()
