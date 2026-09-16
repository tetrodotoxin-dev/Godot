# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "check",
    srcs = ["main.cpp"],
    args = ["$(rootpath //plugins/cpu:cpu_provider)"] + select({
        "//:cuda_enabled": ["$(rootpath //plugins/cuda:cuda_provider)"],
        "//conditions:default": [],
    }),
    data = ["//plugins/cpu:cpu_provider"] + select({
        "//:cuda_enabled": ["//plugins/cuda:cuda_provider"],
        "//conditions:default": [],
    }),
    dynamic_deps = [
        "//:support",
        "@tetrodotoxin//ttx:runtime",
    ],
    deps = [
        "//gdextension/contracts",
        "//imaging/contracts",
        "//sampling:function",
        "//sampling/contracts",
    ],
)
