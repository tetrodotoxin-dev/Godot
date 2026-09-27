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
        "//demo/sampling/contracts",
        "//extension/contracts",
        "@ttx",
    ],
)

cc_shared_library(
    name = "counter_extension",
    dynamic_deps = [
        "//demo:support",
    ],
    shared_lib_name = "libcounter_extension.so",
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
