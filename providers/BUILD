# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

# Optional C++ support for publishing native images. It contains no CPU/CUDA
# storage implementation and cannot depend on the host's dependency graph.
cc_library(
    name = "support",
    srcs = ["image.cpp"],
    hdrs = ["image.hpp"],
    deps = ["//contracts"],
    visibility = ["//visibility:public"],
)
