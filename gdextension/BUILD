# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

# This terminal depends on Godot, TTX and its value/lifecycle contracts.
# Image and CUDA adapters are application consumers of the bridge.
cc_library(
    name = "gdextension",
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
    copts = ["-frtti"],
    deps = [
        "//gdextension/contracts",
        "@godot_cpp",
        "@tetrodotoxin//ttx:concept",
    ],
)
