# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_shared_library.bzl", "cc_shared_library")

cc_library(
    name = "implementation",
    srcs = glob(["*.cpp", "*.c"]),
    hdrs = glob(["*.hpp", "*.h"]),
    copts = ["-fvisibility=hidden"],
    deps = ["//contracts", "//providers:support", "//sampling:publication", "@fftw_sdk//:fftw"],
)

cc_shared_library(
    name = "cpu_provider",
    deps = [":implementation"],
    dynamic_deps = ["//:image_runtime"],
    user_link_flags = ["-Wl,-z,defs", "-Wl,-rpath,$ORIGIN"],
    visibility = ["//visibility:public"],
)
