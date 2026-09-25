# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_shared_library.bzl", "cc_shared_library")

cc_library(
    name = "implementation",
    srcs = [
        "counter.c",
        "exports.c",
        "runtime.c",
    ],
    hdrs = [
        "counter.h",
        "runtime.h",
    ],
    copts = ["-fvisibility=hidden"],
    deps = [
        "//extension/contracts",
        "//demo/sampling/contracts",
        "@tetrodotoxin//ttx:concept",
    ],
)

cc_shared_library(
    name = "counter_extension",
    dynamic_deps = [
        "//demo:support",
        "@tetrodotoxin//ttx:runtime",
    ],
    shared_lib_name = select({
        "//:web": "libcounter_extension.wasm",
        "//conditions:default": "libcounter_extension.so",
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
