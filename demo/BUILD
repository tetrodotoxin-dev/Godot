# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_shared_library.bzl", "cc_shared_library")

package(default_visibility = ["//visibility:public"])

cc_shared_library(
    name = "support",
    dynamic_deps = ["@tetrodotoxin//ttx:runtime"],
    shared_lib_name = select({
        "//:web": "liblab_support.wasm",
        "//conditions:default": "liblab_support.so",
    }),
    user_link_flags = select({
        "//:web": [
            "--oformat=wasm",
            "-sSIDE_MODULE=1",
        ],
        "//conditions:default": [
            "-Wl,-z,defs",
            "-Wl,-rpath,$ORIGIN",
        ],
    }),
    deps = [
        "//extension/contracts",
        "//demo/imaging/contracts",
        "//demo/imaging/publication",
        "//demo/imaging/render",
        "//demo/sampling:publication",
        "//demo/sampling/contracts",
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
        "@tetrodotoxin//ttx:runtime",
    ],
    linkopts = select({
        "//:web": [
            "--oformat=wasm",
            "-sSIDE_MODULE=1",
        ],
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

filegroup(
    name = "addon_payload",
    srcs = [
        "//demo/adapters:editor.gd",
        "//demo/adapters:godot_demo.gdextension",
        "//demo/adapters:plugin.cfg",
        ":libgodot_demo.so",
        ":support",
        "//demo/counter:counter_extension",
        "//demo/sampling/class:sampler_extension",
        "//demo/imaging/cpu:cpu_provider",
    ] + select({
        "//:cuda_enabled": [
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

# Web uses the same module boundaries as the native addon. Keeping the shared
# runtime separate lets provider allocations and retained objects cross those
# boundaries without privately linking another Perimortem allocator.
wasm_cc_binary(
    name = "web_support",
    cc_target = ":support",
    outputs = ["web/liblab_support.wasm"],
    simd = "simd128",
    threads = "off",
)

wasm_cc_binary(
    name = "web_extension",
    cc_target = ":libgodot_demo.so",
    outputs = ["web/libgodot_demo.wasm"],
    simd = "simd128",
    threads = "off",
)

wasm_cc_binary(
    name = "web_counter",
    cc_target = "//demo/counter:counter_extension",
    outputs = ["web/libcounter_extension.wasm"],
    simd = "simd128",
    threads = "off",
)

wasm_cc_binary(
    name = "web_sampler",
    cc_target = "//demo/sampling/class:sampler_extension",
    outputs = ["web/libsampler_extension.wasm"],
    simd = "simd128",
    threads = "off",
)

wasm_cc_binary(
    name = "web_cpu",
    cc_target = "//demo/imaging/cpu:cpu_provider",
    outputs = ["web/libcpu_provider.wasm"],
    simd = "simd128",
    threads = "off",
)

filegroup(
    name = "web_payload",
    srcs = [
        "//extension:web_runtime",
        "//extension:web",
        ":web_support",
        ":web_extension",
        ":web_counter",
        ":web_sampler",
        ":web_cpu",
    ],
)
