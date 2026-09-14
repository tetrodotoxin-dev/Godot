# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "images",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.hpp"]),
    deps = ["//contracts", "//modules", "@tetrodotoxin//ttx:concept"],
    visibility = ["//visibility:public"],
)
