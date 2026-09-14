# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "publication",
    srcs = ["publication.cpp"],
    hdrs = ["provider.h", "publication.hpp"],
    deps = ["//contracts"],
)

cc_library(
    name = "function",
    srcs = ["function.cpp"],
    hdrs = ["function.hpp"],
    deps = [":publication", "//modules"],
)
