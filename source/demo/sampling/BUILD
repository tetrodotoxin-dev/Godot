# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "provider",
    includes = ["../.."],
    srcs = ["provider.cpp"],
    hdrs = ["provider.hpp"],
    deps = ["//source/demo/sampling/contracts"],
)

cc_library(
    name = "function",
    includes = ["../.."],
    srcs = ["function.cpp"],
    hdrs = ["function.hpp"],
    deps = [
        ":provider",
        "@ttx",
    ],
)
