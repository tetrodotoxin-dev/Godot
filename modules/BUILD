# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "modules",
    srcs = ["library.cpp"],
    hdrs = ["library.hpp"],
    deps = ["@tetrodotoxin//perimortem:memory"],
    linkopts = ["-ldl"],
    visibility = ["//visibility:public"],
)
