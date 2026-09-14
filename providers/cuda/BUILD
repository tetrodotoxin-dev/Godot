# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_shared_library.bzl", "cc_shared_library")

cc_library(
    name = "implementation",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.hpp"]) + [":kernel_source"],
    copts = ["-fvisibility=hidden"],
    deps = ["//contracts", "//providers:support", "//sampling:publication", "@cuda_sdk//:cuda"],
)

cc_shared_library(
    name = "cuda_provider",
    deps = [":implementation"],
    dynamic_deps = ["//:image_runtime"],
    user_link_flags = ["-Wl,-z,defs", "-Wl,-rpath,$ORIGIN"],
    visibility = ["//visibility:public"],
)

# Runtime compilation remains local to the CUDA provider. Embed its real CUDA
# source so the installed library needs no source-tree lookup.
genrule(
    name = "kernel_source",
    srcs = ["kernels.cu"],
    outs = ["kernels.inc"],
    tools = ["//tools:embed_cuda.py"],
    cmd = "python3 $(location //tools:embed_cuda.py) $(location kernels.cu) $@",
)
