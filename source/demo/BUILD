# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_shared_library.bzl", "cc_shared_library")
load("//source/demo:build/runtime.bzl", "runtime_libraries")

package(default_visibility = ["//visibility:public"])

cc_shared_library(
    name = "support",
    shared_lib_name = "liblab_support.so",
    user_link_flags = select({
        "//:web": [],
        "//conditions:default": [
            "-Wl,-z,defs",
            "-Wl,-rpath,$ORIGIN",
        ],
    }),
    deps = [
        "//source/demo/imaging/contracts",
        "//source/demo/imaging/publication",
        "//source/demo/imaging/render",
        "//source/demo/sampling:provider",
        "//source/demo/sampling/contracts",
    ],
)

cc_binary(
    name = "libgodot_demo.so",
    srcs = ["//source/demo/adapters:register.cpp"],
    copts = [
        "-fvisibility=hidden",
        "-frtti",
    ],
    dynamic_deps = [
        ":support",
    ],
    linkopts = select({
        "//:web": [],
        "//conditions:default": [
            "-Wl,-z,defs",
            "-Wl,-rpath,$ORIGIN",
        ],
    }),
    linkshared = True,
    linkstatic = True,
    deps = [
        "//source/demo/adapters:cuda",
        "//source/demo/adapters:imaging",
        "//source/godot_ttx:extension",
    ] + select({
        "//:web": ["@godot_cpp_web//:godot_cpp"],
        "//conditions:default": ["@godot_cpp"],
    }),
)

runtime_libraries(
    name = "cuda_runtime",
    deps = [
        "@ttx_cuda//build:compute",
        "@ttx_cuda//build:fft",
    ],
)

genrule(
    name = "cuda_notices",
    srcs = [
        "@cuda_nvrtc_linux//:LICENSE",
        "@libcufft_linux//:LICENSE",
    ],
    outs = [
        "NVIDIA-NVRTC-LICENSE.txt",
        "NVIDIA-CUFFT-LICENSE.txt",
    ],
    cmd = "cp $(location @cuda_nvrtc_linux//:LICENSE) $(location NVIDIA-NVRTC-LICENSE.txt) && " +
          "cp $(location @libcufft_linux//:LICENSE) $(location NVIDIA-CUFFT-LICENSE.txt)",
)

filegroup(
    name = "addon_payload",
    srcs = [
        ":libgodot_demo.so",
        ":support",
        "//source/demo/adapters:editor.gd",
        "//source/demo/adapters:godot_demo.gdextension",
        "//source/demo/adapters:plugin.cfg",
        "//source/demo/counter:counter_extension",
        "//source/demo/imaging/cpu:cpu_provider",
        "//source/demo/sampling/class:sampler_extension",
    ] + select({
        "//:cuda_enabled": [
            ":cuda_notices",
            ":cuda_runtime",
            "//source/demo/imaging/cuda:cuda_provider",
            "@ttx_cuda//cuda:plugin",
        ],
        "//conditions:default": [],
    }),
)

genrule(
    name = "addon",
    srcs = [
        "//source/godot_ttx:addon",
        ":addon_payload",
        "//source/demo/adapters:scripts",
        "//source/demo/adapters:scene",
        "//source/demo/adapters:resources",
    ],
    outs = ["godot_ttx.tar"],
    cmd = "cp $(location //source/godot_ttx:addon) $@ && chmod u+w $@ && " +
          "tar -rhf $@ --transform='s|.*/||' $(locations :addon_payload) && " +
          "tar -rhf $@ --transform='s|^source/demo/adapters/imaging/scripts/|gdscript/|' $(locations //source/demo/adapters:scripts) && " +
          "tar -rhf $@ --transform='s|^source/demo/adapters/imaging/||' $(locations //source/demo/adapters:scene) && " +
          "tar -rhf $@ --transform='s|^source/demo/adapters/||' $(locations //source/demo/adapters:resources)",
)

# Platform selection changes the code generator, not the library graph.
# Export stages these modules under web/ beside the native addon.
filegroup(
    name = "web_payload",
    srcs = [
        ":libgodot_demo.so",
        ":support",
        "//source/demo/counter:counter_extension",
        "//source/demo/imaging/cpu:cpu_provider",
        "//source/demo/sampling/class:sampler_extension",
        "//source/godot_ttx:runtime",
        "//source/godot_ttx:bridge",
        "@perimortem//:build",
        "@ttx//:build",
    ],
)
