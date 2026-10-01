# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "imaging",
    includes = ["../.."],
    srcs = glob(["imaging/**/*.cpp"]),
    hdrs = glob(["imaging/**/*.hpp"]) + ["imports.hpp"],
    copts = [
        "-fvisibility=hidden",
        "-frtti",
    ],
    deps = [
        "//source/godot_ttx:extension",
        "//source/demo/imaging/contracts",
        "//source/demo/imaging/graph",
        "//source/demo/imaging/publication",
    ] + select({
        "//:web": ["@godot_cpp_web//:godot_cpp"],
        "//conditions:default": ["@godot_cpp"],
    }),
)

cc_library(
    name = "cuda",
    includes = ["../.."],
    srcs = glob(["cuda/*.cpp"]),
    hdrs = glob(["cuda/*.hpp"]) + ["imports.hpp"],
    copts = [
        "-fvisibility=hidden",
        "-frtti",
    ],
    deps = [
        "//source/godot_ttx:extension",
        "@ttx_cuda//cuda/contracts",
    ] + select({
        "//:web": ["@godot_cpp_web//:godot_cpp"],
        "//conditions:default": ["@godot_cpp"],
    }),
)

filegroup(
    name = "scripts",
    srcs = glob(["imaging/scripts/*.gd"]),
)

filegroup(
    name = "scene",
    srcs = glob(["imaging/scene/*.gd"]),
)

filegroup(
    name = "resources",
    srcs = glob([
        "imaging/resources/*.gd",
        "imaging/editor/*.gd",
        "cuda/resources/*.gd",
        "cuda/editor/*.gd",
    ]),
)

exports_files(["register.cpp", "editor.gd", "plugin.cfg", "godot_demo.gdextension"])
