# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_shared_library.bzl", "cc_shared_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "cuda",
    includes = ["../../.."],
    srcs = glob(
        ["*.cpp"],
        exclude = ["module.cpp"],
    ),
    hdrs = glob(["*.hpp"]) + [":kernel_source"],
    copts = ["-fvisibility=hidden"],
    deps = [
        "//source/demo/imaging/contracts",
        "//source/demo/imaging/publication",
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
    includes = ["../../.."],
    srcs = ["module.cpp"],
    copts = ["-fvisibility=hidden"],
    deps = [
        "//source/demo/imaging/contracts",
        "//source/demo/imaging/cuda",
        "//source/demo/imaging/render",
        "//source/demo/sampling/cuda",
    ],
)

cc_shared_library(
    name = "cuda_provider",
    dynamic_deps = [
        "//source/demo:support",
    ] + ["@ttx_cuda//cuda:plugin"],
    user_link_flags = [
        "-Wl,-z,defs",
        "-Wl,-rpath,$ORIGIN",
    ],
    deps = [":module"],
)
