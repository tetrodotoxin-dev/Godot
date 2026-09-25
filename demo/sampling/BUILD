# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "publication",
    srcs = ["publication.cpp"],
    hdrs = ["publication.hpp"],
    deps = ["//demo/sampling/contracts"],
)

cc_library(
    name = "function",
    srcs = ["function.cpp"],
    hdrs = ["function.hpp"],
    deps = [
        ":publication",
        "@tetrodotoxin//ttx:concept",
    ],
)
