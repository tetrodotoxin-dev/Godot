# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_shared_library.bzl", "cc_shared_library")

package(default_visibility = ["//visibility:public"])

config_setting(
    name = "cuda_enabled",
    define_values = {"godot_cuda": "1"},
)

cc_shared_library(
    name = "support",
    dynamic_deps = ["@tetrodotoxin//ttx:runtime"],
    shared_lib_name = "liblab_support.so",
    user_link_flags = [
        "-Wl,-z,defs",
        "-Wl,-rpath,$ORIGIN",
    ],
    deps = [
        "//gdextension/contracts",
        "//imaging/contracts",
        "//imaging/publication",
        "//plugins/render",
        "//sampling:publication",
        "//sampling/contracts",
    ],
)

cc_binary(
    name = "libgodot_ttx.so",
    srcs = ["addon/register.cpp"],
    copts = [
        "-fvisibility=hidden",
        "-frtti",
    ],
    dynamic_deps = [
        ":support",
        "@tetrodotoxin//ttx:runtime",
    ],
    linkopts = [
        "-Wl,-z,defs",
        "-Wl,-rpath,$ORIGIN",
    ],
    linkshared = True,
    linkstatic = True,
    deps = [
        "//adapters:cuda",
        "//adapters:imaging",
        "//gdextension",
        "@godot_cpp",
    ],
)

filegroup(
    name = "addon_payload",
    srcs = [
        "addon/editor.gd",
        "addon/godot_ttx.gdextension",
        "addon/plugin.cfg",
        ":libgodot_ttx.so",
        ":support",
        "//extensions/counter:counter_extension",
        "//extensions/sampling:sampler_extension",
        "//plugins/cpu:cpu_provider",
        "//tests:godot_check.gd",
        "@tetrodotoxin//ttx:runtime",
    ] + select({
        ":cuda_enabled": [
            "//plugins/cuda:cuda_provider",
            "@ttx_cuda//cuda:plugin",
        ],
        "//conditions:default": [],
    }),
)

genrule(
    name = "addon",
    srcs = [
        ":addon_payload",
        "//adapters:scripts",
        "//adapters:scene",
        "//adapters:resources",
    ],
    outs = ["godot_ttx.tar"],
    cmd = "tar -chf $@ --transform='s|.*/||' $(locations :addon_payload) && " +
          "tar -rhf $@ --transform='s|^adapters/imaging/scripts/|gdscript/|' $(locations //adapters:scripts) && " +
          "tar -rhf $@ --transform='s|^adapters/imaging/||' $(locations //adapters:scene) && " +
          "tar -rhf $@ --transform='s|^adapters/||' $(locations //adapters:resources)",
)
