# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_shared_library.bzl", "cc_shared_library")

package(default_visibility = ["//visibility:public"])

config_setting(name = "cuda_enabled", define_values = {"godot_cuda": "1"})

# A shared Perimortem runtime retains allocation ownership across native modules.
# The providers share protocol support, but neither links the host image graph,
# Godot bindings, or the other backend's implementation.
cc_shared_library(
    name = "image_runtime",
    deps = [
        "//contracts",
        "//providers:support",
        "//sampling:publication",
        "@tetrodotoxin//ttx:semantic",
        "@tetrodotoxin//ttx:data",
        "@tetrodotoxin//perimortem:abi",
        "@tetrodotoxin//perimortem:core",
        "@tetrodotoxin//perimortem:memory",
        "@tetrodotoxin//perimortem:system",
        "@tetrodotoxin//perimortem:serialization",
        "@tetrodotoxin//perimortem:compression",
        "@tetrodotoxin//perimortem:graphics",
    ],
    user_link_flags = ["-Wl,-z,defs"],
)

cc_binary(
    name = "libgodot_ttx.so",
    srcs = glob(["extension/**/*.cpp", "extension/**/*.hpp"]),
    copts = ["-fvisibility=hidden", "-frtti"],
    deps = ["//images", "//sampling:function", "//operations:standard", "//providers/gdscript", "@godot_cpp//:godot_cpp"],
    dynamic_deps = [":image_runtime"],
    linkopts = ["-Wl,-z,defs", "-Wl,-rpath,$ORIGIN"],
    linkshared = True,
    linkstatic = True,
)

filegroup(
    name = "addon_payload",
    srcs = [
        ":libgodot_ttx.so",
        ":image_runtime",
        "//providers/cpu:cpu_provider",
        "godot_ttx.gdextension",
        "//tests:godot_check.gd",
    ] + select({
        ":cuda_enabled": ["//providers/cuda:cuda_provider"],
        "//conditions:default": [],
    }),
)

genrule(
    name = "addon",
    srcs = [":addon_payload", "//providers/gdscript:scripts"],
    outs = ["godot_ttx.tar"],
    cmd = "tar -chf $@ --transform='s|.*/||' $(locations :addon_payload) && " +
          "tar -rhf $@ --transform='s|^providers/||' $(locations //providers/gdscript:scripts)",
)
