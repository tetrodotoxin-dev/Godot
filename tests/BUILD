load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_shared_library.bzl", "cc_shared_library")

# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "probe",
    testonly = True,
    srcs = ["probe.cpp"],
    hdrs = ["echo.hpp"],
    copts = ["-fvisibility=hidden"],
    deps = [
        "//gdextension/contracts",
        "//imaging/contracts",
        "//imaging/publication",
        "//plugins/render",
        "//sampling/contracts",
    ],
)

cc_shared_library(
    name = "probe_provider",
    testonly = True,
    dynamic_deps = [
        "//:support",
        "@tetrodotoxin//ttx:runtime",
    ],
    user_link_flags = [
        "-Wl,-z,defs",
        "-Wl,-rpath,$ORIGIN",
    ],
    deps = [":probe"],
)

cc_test(
    name = "image_check",
    srcs = [
        "echo.hpp",
        "forms.cpp",
        "forms.hpp",
        "fulfillment.cpp",
        "fulfillment.hpp",
        "image_check.cpp",
        "library.cpp",
        "library.hpp",
    ],
    args = [
        "$(rootpath //plugins/cpu:cpu_provider)",
        "$(rootpath :probe_provider)",
    ] + select({
        "//:cuda_enabled": ["$(rootpath //plugins/cuda:cuda_provider)"],
        "//conditions:default": [],
    }),
    data = [
        ":probe_provider",
        "//plugins/cpu:cpu_provider",
    ] + select({
        "//:cuda_enabled": ["//plugins/cuda:cuda_provider"],
        "//conditions:default": [],
    }),
    dynamic_deps = [
        "//:support",
        "@tetrodotoxin//ttx:runtime",
    ],
    deps = [
        "//imaging/graph",
        "//imaging/operations",
        "@tetrodotoxin//perimortem:system",
    ],
)

exports_files(["godot_check.gd"])
