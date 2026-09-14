# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

# Timing stays in a separate executable. The production providers and host
# graph are linked unchanged, with no allocation hooks in the addon.
cc_binary(
    name = "mechanics",
    testonly = True,
    srcs = ["main.cpp"],
    deps = ["//images", "//operations:standard"],
    dynamic_deps = ["//:image_runtime"],
    data = ["//providers/cpu:cpu_provider"] + select({
        "//:cuda_enabled": ["//providers/cuda:cuda_provider"],
        "//conditions:default": [],
    }),
    # The repository uses run_in_cwd and publishes outputs under .bin.
    args = [".bin/bin/$(rootpath //providers/cpu:cpu_provider)"] + select({
        "//:cuda_enabled": [".bin/bin/$(rootpath //providers/cuda:cuda_provider)"],
        "//conditions:default": [],
    }),
)
