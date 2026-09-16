# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_shared_library.bzl", "cc_shared_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "implementation",
    srcs = ["provider.cpp"],
    hdrs = ["provider.hpp"],
    copts = ["-fvisibility=hidden"],
    deps = [
        "//imaging/contracts",
        "//imaging/cuda",
        "//plugins/render",
        "//sampling/cuda",
    ],
)

cc_shared_library(
    name = "cuda_provider",
    dynamic_deps = [
        "//:support",
        "@tetrodotoxin//ttx:runtime",
    ] + ["@ttx_cuda//cuda:plugin"],
    user_link_flags = [
        "-Wl,-z,defs",
        "-Wl,-rpath,$ORIGIN",
    ],
    deps = [":implementation"],
)
