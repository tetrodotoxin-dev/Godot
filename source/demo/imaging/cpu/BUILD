load("@rules_cc//cc:cc_library.bzl", "cc_library")

load("@tetro_toolchain//:defs.bzl", "library", "WEB", "WINDOWS")

library(
    name = "cpu_provider",
    module = "lab_cpu",
    linkage = "shared",
    srcs = ["provider.cpp", "module.c", "image.hpp"],
    includes = ["../../.."],
    copts = ["-fexceptions", "-fvisibility-inlines-hidden"] + select({WEB: ["-fwasm-exceptions"], "//conditions:default": []}),
    linkopts = select({WEB: ["-fwasm-exceptions", "-Wl,--export=__wasm_call_ctors"], "//conditions:default": []}),
    shared_lib_name = select({WINDOWS: "cpu_provider.dll", "//conditions:default": "libcpu_provider.so"}),
    implementation_deps = ["//source/demo/imaging/contracts", ":convolution"],
    visibility = ["//visibility:public"],
)

filegroup(name = "runtime", srcs = [":cpu_provider_shared"], visibility = ["//visibility:public"])
exports_files(["pocketfft.BUILD.bazel"])

# PocketFFT owns its standard-library dependency. Its one translation unit
# establishes that host environment before including the shared runtime SDK.
cc_library(
    name = "convolution",
    srcs = ["convolve.cpp"],
    hdrs = ["image.hpp"],
    includes = ["../../.."],
    copts = ["-fexceptions", "-fvisibility-inlines-hidden", "-include", "pocketfft_hdronly.h"] + select({WEB: ["-fwasm-exceptions"], "//conditions:default": []}),
    deps = ["//source/demo/imaging/contracts", "@pocketfft"],
)
