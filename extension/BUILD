# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

# This terminal depends on Godot, TTX and its value/lifecycle contracts.
# Image and CUDA adapters are application consumers of the bridge.
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
    copts = [
        "-frtti",
        "-fvisibility=hidden",
    ],
    deps = [
        "//extension/contracts",
        "@ttx",
    ] + select({
        "//:web": ["@godot_cpp_web//:godot_cpp"],
        "//conditions:default": ["@godot_cpp"],
    }),
)

cc_binary(
    name = "libgodot_ttx.so",
    srcs = ["register.cpp"],
    copts = [
        "-fvisibility=hidden",
        "-frtti",
    ],
    linkopts = select({
        "//:web": [],
        "//conditions:default": [
            "-Wl,-z,defs",
            "-Wl,-rpath,$ORIGIN",
        ],
    }),
    linkshared = True,
    linkstatic = True,
    deps = [":extension"],
)

genrule(
    name = "addon",
    srcs = [
        "godot_ttx.gdextension",
        ":libgodot_ttx.so",
        "@ttx//:build",
        "@perimortem//:build",
    ],
    outs = ["godot_ttx.tar"],
    cmd = "tar -chf $@ --transform='s|.*/||' $(SRCS)",
)
