# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_shared_library.bzl", "cc_shared_library")

cc_library(
    name = "rejected",
    testonly = True,
    srcs = ["rejected.c"],
    copts = ["-fvisibility=hidden"],
    deps = ["@tetrodotoxin//ttx:concept"],
)

cc_shared_library(
    name = "rejected_extension",
    testonly = True,
    dynamic_deps = [
        "//:support",
        "@tetrodotoxin//ttx:runtime",
    ],
    user_link_flags = [
        "-Wl,-z,defs",
        "-Wl,-rpath,$ORIGIN",
    ],
    deps = [":rejected"],
)

cc_library(
    name = "values",
    testonly = True,
    srcs = ["values.cpp"],
    copts = ["-fvisibility=hidden"],
    deps = [
        "//extensions/sampling:declarations",
        "//gdextension/contracts",
        "//sampling/contracts",
        "@tetrodotoxin//ttx:concept",
    ],
)

cc_shared_library(
    name = "values_extension",
    testonly = True,
    dynamic_deps = [
        "//:support",
        "@tetrodotoxin//ttx:runtime",
    ],
    user_link_flags = [
        "-Wl,-z,defs",
        "-Wl,-rpath,$ORIGIN",
    ],
    deps = [":values"],
)
