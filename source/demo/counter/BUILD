# Copyright (c) 2023-present Matt Kaes and contributors

load("@tetro_toolchain//:defs.bzl", "library", "WINDOWS", "WEB")

library(
    name = "counter_extension",
    module = "counter",
    linkage = "shared",
    srcs = ["counter.cpp", "module.c"],
    linkopts = select({WEB: ["-Wl,--export=__wasm_call_ctors"], "//conditions:default": []}),
    shared_lib_name = select({WINDOWS: "counter_extension.dll", "//conditions:default": "libcounter_extension.so"}),
    implementation_deps = ["@ttx//:graph"],
    visibility = ["//visibility:public"],
)

filegroup(name = "runtime", srcs = [":counter_extension_shared"], visibility = ["//visibility:public"])
