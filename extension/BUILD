# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")

package(default_visibility = ["//visibility:public"])

# This terminal depends on Godot, TTX and its value/lifecycle contracts.
# Image and CUDA adapters are application consumers of the bridge.
cc_library(
    name = "extension",
    srcs = glob([
        "classes/*.cpp",
        "modules/*.cpp",
        "values/*.cpp",
    ]),
    hdrs = glob([
        "classes/*.hpp",
        "modules/*.hpp",
        "values/*.hpp",
    ]),
    copts = ["-frtti", "-fvisibility=hidden"],
    deps = [
        "//extension/contracts",
        "@tetrodotoxin//ttx:concept",
    ] + select({
        "//:web": ["@godot_cpp_web//:godot_cpp"],
        "//conditions:default": ["@godot_cpp"],
    }),
)

cc_binary(
    name = "libgodot_ttx.so",
    srcs = ["register.cpp"],
    copts = ["-fvisibility=hidden", "-frtti"],
    dynamic_deps = ["@tetrodotoxin//ttx:runtime"],
    linkopts = select({
        "//:web": ["--oformat=wasm", "-sSIDE_MODULE=1"],
        "//conditions:default": ["-Wl,-z,defs", "-Wl,-rpath,$ORIGIN"],
    }),
    linkshared = True,
    linkstatic = True,
    deps = [":extension"],
)

genrule(
    name = "addon",
    srcs = ["godot_ttx.gdextension", ":libgodot_ttx.so", "@tetrodotoxin//ttx:runtime"],
    outs = ["godot_ttx.tar"],
    cmd = "tar -chf $@ --transform='s|.*/||' $(SRCS)",
)

wasm_cc_binary(
    name = "web_runtime",
    cc_target = "@tetrodotoxin//ttx:runtime",
    outputs = ["web/libttx_runtime.wasm"],
    simd = "simd128",
    threads = "off",
)

wasm_cc_binary(
    name = "web",
    cc_target = ":libgodot_ttx.so",
    outputs = ["web/libgodot_ttx.wasm"],
    simd = "simd128",
    threads = "off",
)
