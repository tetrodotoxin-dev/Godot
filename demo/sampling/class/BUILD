# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_shared_library.bzl", "cc_shared_library")

cc_library(
    name = "implementation",
    srcs = glob(["*.cpp"]),
    hdrs = [
        "declaration.hpp",
        "exports.hpp",
        "runtime.hpp",
        "sampler.hpp",
    ],
    copts = ["-fvisibility=hidden"],
    deps = [
        ":contracts",
        ":declarations",
        "//demo/sampling:function",
        "//demo/sampling/contracts",
        "//extension/contracts",
        "@ttx",
    ],
)

cc_shared_library(
    name = "sampler_extension",
    dynamic_deps = [
        "//demo:support",
    ],
    shared_lib_name = "libsampler_extension.so",
    user_link_flags = select({
        "//:web": [],
        "//conditions:default": [
            "-Wl,-z,defs",
            "-Wl,-rpath,$ORIGIN",
        ],
    }),
    visibility = ["//visibility:public"],
    deps = [":implementation"],
)

cc_library(
    name = "declarations",
    hdrs = ["method.hpp"],
    visibility = ["//visibility:public"],
    deps = ["@ttx"],
)

cc_library(
    name = "contracts",
    hdrs = ["contracts.h"],
    visibility = ["//visibility:public"],
    deps = ["@ttx"],
)
