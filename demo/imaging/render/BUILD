# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "render",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.hpp"]),
    deps = [
        "//extension/contracts",
        "//demo/imaging/contracts",
        "//demo/sampling/contracts",
        "@tetrodotoxin//ttx:concept",
    ],
)
