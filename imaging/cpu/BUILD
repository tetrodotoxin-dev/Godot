# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "cpu",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.hpp"]),
    copts = ["-fvisibility=hidden"],
    deps = [
        "//imaging/contracts",
        "//imaging/publication",
        "@fftw_sdk//:fftw",
    ],
)

exports_files(["sdk.bzl"])
