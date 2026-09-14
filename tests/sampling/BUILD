# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "check",
    srcs = ["main.cpp"],
    deps = ["//sampling:function", "//contracts"],
    dynamic_deps = ["//:image_runtime"],
    data = ["//providers/cpu:cpu_provider"] + select({
        "//:cuda_enabled": ["//providers/cuda:cuda_provider"],
        "//conditions:default": [],
    }),
    args = ["$(rootpath //providers/cpu:cpu_provider)"] + select({
        "//:cuda_enabled": ["$(rootpath //providers/cuda:cuda_provider)"],
        "//conditions:default": [],
    }),
)
