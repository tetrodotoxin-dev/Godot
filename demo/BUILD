# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_shared_library.bzl", "cc_shared_library")
load("//demo:build/runtime.bzl", "runtime_libraries")

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
        "//demo/imaging/contracts",
        "//demo/imaging/publication",
        "//demo/imaging/render",
        "//demo/sampling:provider",
        "//demo/sampling/contracts",
        "//extension/contracts",
    ],
)

cc_binary(
    name = "libgodot_demo.so",
    srcs = ["//demo/adapters:register.cpp"],
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
        "//demo/adapters:cuda",
        "//demo/adapters:imaging",
        "//extension",
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
        "//demo/adapters:editor.gd",
        "//demo/adapters:godot_demo.gdextension",
        "//demo/adapters:plugin.cfg",
        "//demo/counter:counter_extension",
        "//demo/imaging/cpu:cpu_provider",
        "//demo/sampling/class:sampler_extension",
    ] + select({
        "//:cuda_enabled": [
            ":cuda_notices",
            ":cuda_runtime",
            "//demo/imaging/cuda:cuda_provider",
            "@ttx_cuda//cuda:plugin",
        ],
        "//conditions:default": [],
    }),
)

genrule(
    name = "addon",
    srcs = [
        "//extension:addon",
        ":addon_payload",
        "//demo/adapters:scripts",
        "//demo/adapters:scene",
        "//demo/adapters:resources",
    ],
    outs = ["godot_ttx.tar"],
    cmd = "cp $(location //extension:addon) $@ && chmod u+w $@ && " +
          "tar -rhf $@ --transform='s|.*/||' $(locations :addon_payload) && " +
          "tar -rhf $@ --transform='s|^demo/adapters/imaging/scripts/|gdscript/|' $(locations //demo/adapters:scripts) && " +
          "tar -rhf $@ --transform='s|^demo/adapters/imaging/||' $(locations //demo/adapters:scene) && " +
          "tar -rhf $@ --transform='s|^demo/adapters/||' $(locations //demo/adapters:resources)",
)

# Platform selection changes the code generator, not the library graph.
# Export stages these modules under web/ beside the native addon.
filegroup(
    name = "web_payload",
    srcs = [
        ":libgodot_demo.so",
        ":support",
        "//demo/counter:counter_extension",
        "//demo/imaging/cpu:cpu_provider",
        "//demo/sampling/class:sampler_extension",
        "//extension:libgodot_ttx.so",
        "@perimortem//:build",
        "@ttx//:build",
    ],
)
