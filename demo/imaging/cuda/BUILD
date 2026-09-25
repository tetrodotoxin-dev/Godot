# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_shared_library.bzl", "cc_shared_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "cuda",
    srcs = glob(["*.cpp"], exclude = ["module.cpp"]),
    hdrs = glob(["*.hpp"]) + [":kernel_source"],
    copts = ["-fvisibility=hidden"],
    deps = [
        "//demo/imaging/contracts",
        "//demo/imaging/publication",
        "@ttx_cuda//build:fft",
        "@ttx_cuda//cuda/runtime",
    ],
)

genrule(
    name = "kernel_source",
    srcs = ["kernels.cu"],
    outs = ["kernels.inc"],
    cmd = "python3 $(location :embed.py) $(location kernels.cu) $@",
    tools = [":embed.py"],
)

exports_files(["embed.py"])

cc_library(
    name = "module",
    srcs = ["module.cpp"],
    copts = ["-fvisibility=hidden"],
    deps = [
        "//demo/imaging/contracts",
        "//demo/imaging/cuda",
        "//demo/imaging/render",
        "//demo/sampling/cuda",
    ],
)

cc_shared_library(
    name = "cuda_provider",
    dynamic_deps = [
        "//demo:support",
        "@tetrodotoxin//ttx:runtime",
    ] + ["@ttx_cuda//cuda:plugin"],
    user_link_flags = [
        "-Wl,-z,defs",
        "-Wl,-rpath,$ORIGIN",
    ],
    deps = [":module"],
)
