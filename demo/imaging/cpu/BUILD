# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_shared_library.bzl", "cc_shared_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "cpu",
    srcs = glob(
        ["*.cpp"],
        exclude = ["module.cpp"],
    ),
    hdrs = glob(["*.hpp"]),
    copts = [
        "-fvisibility=hidden",
        "-fexceptions",
    ] + select({
        "//:web": ["-fwasm-exceptions"],
        "//conditions:default": [],
    }),
    deps = [
        "//demo/imaging/contracts",
        "//demo/imaging/publication",
        "@pocketfft",
    ],
)

exports_files(["pocketfft.BUILD.bazel"])

cc_library(
    name = "module",
    srcs = ["module.cpp"],
    copts = ["-fvisibility=hidden"],
    deps = [
        "//demo/imaging/contracts",
        "//demo/imaging/cpu",
        "//demo/imaging/render",
        "//demo/sampling:provider",
        "//demo/sampling/cpu",
    ],
)

cc_shared_library(
    name = "cpu_provider",
    dynamic_deps = [
        "//demo:support",
    ],
    shared_lib_name = "libcpu_provider.so",
    user_link_flags = select({
        "//:web": [
            "-fwasm-exceptions",
        ],
        "//conditions:default": [
            "-Wl,-z,defs",
            "-Wl,-rpath,$ORIGIN",
        ],
    }),
    deps = [":module"],
)
