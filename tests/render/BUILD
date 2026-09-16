# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "cpu",
    srcs = ["main.cpp"],
    args = ["$(rootpath //plugins/cpu:cpu_provider)"],
    data = ["//plugins/cpu:cpu_provider"],
    dynamic_deps = [
        "//:support",
        "@tetrodotoxin//ttx:runtime",
    ],
    deps = [
        "//gdextension/contracts",
        "//imaging/contracts",
        "//sampling/contracts",
        "@tetrodotoxin//ttx:concept",
    ],
)

cc_test(
    name = "cuda",
    srcs = ["main.cpp"],
    args = ["$(rootpath //plugins/cuda:cuda_provider)"],
    data = ["//plugins/cuda:cuda_provider"],
    dynamic_deps = [
        "//:support",
        "@tetrodotoxin//ttx:runtime",
    ],
    tags = ["local"],
    deps = [
        "//gdextension/contracts",
        "//imaging/contracts",
        "//sampling/contracts",
        "@tetrodotoxin//ttx:concept",
    ],
)
