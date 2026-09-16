# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "contracts",
    srcs = glob(["*.cpp"]),
    hdrs = glob([
        "*.hpp",
        "*.h",
    ]),
    deps = [
        "@tetrodotoxin//perimortem:memory",
        "@tetrodotoxin//ttx:concept",
    ],
)
