# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "cuda",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.hpp"]) + [":kernel_source"],
    copts = ["-fvisibility=hidden"],
    deps = [
        "//imaging/contracts",
        "//imaging/publication",
        "@ttx_cuda//build:fft",
        "@ttx_cuda//cuda/runtime",
    ],
)

genrule(
    name = "kernel_source",
    srcs = ["kernels.cu"],
    outs = ["kernels.inc"],
    cmd = "python3 $(location //tools:embed_cuda.py) $(location kernels.cu) $@",
    tools = ["//tools:embed_cuda.py"],
)
