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
        ":inspection",
        "//gdextension/contracts",
        "//sampling/contracts",
        "@tetrodotoxin//ttx:concept",
    ],
)

cc_shared_library(
    name = "counter_extension",
    dynamic_deps = [
        "//:support",
        "@tetrodotoxin//ttx:runtime",
    ],
    user_link_flags = [
        "-Wl,-z,defs",
        "-Wl,-rpath,$ORIGIN",
    ],
    visibility = ["//visibility:public"],
    deps = [":implementation"],
)

cc_library(
    name = "inspection",
    srcs = ["inspection.c"],
    hdrs = [
        "inspection.h",
        "inspection.hpp",
    ],
    visibility = ["//visibility:public"],
    deps = ["@tetrodotoxin//ttx:semantic"],
)
