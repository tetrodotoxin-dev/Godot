# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "imaging",
    srcs = glob(["imaging/**/*.cpp"]),
    hdrs = glob(["imaging/**/*.hpp"]),
    copts = [
        "-fvisibility=hidden",
        "-frtti",
    ],
    deps = [
        "//imaging/contracts",
        "//imaging/graph",
        "//imaging/operations",
        "//imaging/publication",
        "@godot_cpp",
    ],
)

cc_library(
    name = "cuda",
    srcs = glob(["cuda/*.cpp"]),
    hdrs = glob(["cuda/*.hpp"]),
    copts = [
        "-fvisibility=hidden",
        "-frtti",
    ],
    deps = [
        "//gdextension",
        "@godot_cpp",
        "@ttx_cuda//cuda/contracts",
    ],
)

filegroup(
    name = "scripts",
    srcs = glob(["imaging/scripts/*.gd"]),
)

filegroup(
    name = "scene",
    srcs = glob(["imaging/scene/*.gd"]),
)
