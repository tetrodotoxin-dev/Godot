# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

# Public carriers and observation helpers have no dependency on the host graph
# or either native backend. An independently loaded provider can use this surface.
cc_library(
    name = "contracts",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.h", "*.hpp"]),
    deps = [
        "@tetrodotoxin//ttx:semantic",
        "@tetrodotoxin//perimortem:memory",
    ],
)
