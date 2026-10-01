# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "headers",
    hdrs = glob(["*.hpp", "*.h"]),
    includes = ["../.."],
    deps = ["@ttx"],
)

cc_library(
    name = "implementation",
    srcs = glob(["*.cpp"]),
    alwayslink = True,
    deps = [":headers"],
)
