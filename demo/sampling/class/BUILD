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
        "//extension/contracts",
        "//demo/sampling:function",
        "//demo/sampling/contracts",
        "@tetrodotoxin//ttx:concept",
    ],
)

cc_shared_library(
    name = "sampler_extension",
    dynamic_deps = [
        "//demo:support",
        "@tetrodotoxin//ttx:runtime",
    ],
    shared_lib_name = select({
        "//:web": "libsampler_extension.wasm",
        "//conditions:default": "libsampler_extension.so",
    }),
    user_link_flags = select({
        "//:web": [
            "--oformat=wasm",
            "-sSIDE_MODULE=1",
        ],
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
    deps = ["@tetrodotoxin//ttx:concept"],
)

cc_library(
    name = "contracts",
    hdrs = ["contracts.h"],
    visibility = ["//visibility:public"],
    deps = ["@tetrodotoxin//ttx:data"],
)
