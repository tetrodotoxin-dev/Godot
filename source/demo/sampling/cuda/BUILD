# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "cuda",
    includes = ["../../.."],
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.hpp"]),
    copts = ["-fvisibility=hidden"],
    deps = [
        "//source/demo/imaging/cuda",
        "//source/demo/sampling:provider",
    ],
)
