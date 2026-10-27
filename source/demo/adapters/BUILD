load("@tetro_toolchain//:defs.bzl", "library", "WEB", "WINDOWS")

library(
    name = "demo",
    module = "godot_demo",
    linkage = "shared",
    srcs = ["register.cpp", "imaging/ttx_image.cpp", "imaging/ttx_image.hpp", "imaging/scripts/bridge.cpp", "imaging/scripts/bridge.hpp"] + glob(["cuda/*.cpp", "cuda/*.hpp"]),
    includes = ["../.."],
    local_defines = ["GODOT_CUDA_ADAPTER"],
    copts = ["-frtti", "-fvisibility-inlines-hidden", "-include", "godot_cpp/core/memory.hpp"],
    linkopts = select({WEB: ["-Wl,--export=__wasm_call_ctors"], "//conditions:default": []}),
    shared_lib_name = select({WINDOWS: "godot_demo.dll", "//conditions:default": "libgodot_demo.so"}),
    implementation_deps = ["//source/demo/imaging/contracts", "@perimortem//:system", "@perimortem//:serialization"] + select({WEB: [], "//conditions:default": ["@compute_cuda//:headers"]}) + select({WEB: ["@godot_cpp_web//:godot_cpp"], "//conditions:default": ["@godot_cpp//:godot_cpp"]}),
    visibility = ["//visibility:public"],
)

filegroup(name = "runtime", srcs = [":demo_shared"], visibility = ["//visibility:public"])

load("@rules_pkg//pkg:mappings.bzl", "pkg_files", "strip_prefix")

pkg_files(
    name = "resources",
    srcs = glob(["imaging/resources/*.gd", "imaging/editor/*.gd", "cuda/resources/*.gd", "cuda/editor/*.gd"]) + ["editor.gd", "plugin.cfg", "godot_demo.gdextension"],
    strip_prefix = strip_prefix.from_pkg(),
    prefix = "addons/godot_ttx",
    visibility = ["//visibility:public"],
)

pkg_files(
    name = "scene",
    srcs = glob(["imaging/scene/*.gd"]),
    strip_prefix = strip_prefix.from_pkg("imaging/scene"),
    prefix = "addons/godot_ttx/scene",
    visibility = ["//visibility:public"],
)

pkg_files(
    name = "scripts",
    srcs = glob(["imaging/scripts/*.gd"]),
    strip_prefix = strip_prefix.from_pkg("imaging/scripts"),
    prefix = "addons/godot_ttx/gdscript",
    visibility = ["//visibility:public"],
)
