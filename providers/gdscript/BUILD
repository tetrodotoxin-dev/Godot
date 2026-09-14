# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

# Only this provider knows the Godot script runtime. Its image contracts and
# publication support remain the same dependencies used by the native backends.
cc_library(
    name = "gdscript",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.hpp"]),
    deps = ["//contracts", "//providers:support", "@godot_cpp//:godot_cpp"],
    # Upstream Godot uses dynamic_cast when a Variant becomes Ref<RefCounted>.
    # This is the script runtime boundary, not TTX identity negotiation.
    copts = ["-fvisibility=hidden", "-frtti"],
    visibility = ["//visibility:public"],
)

filegroup(
    name = "scripts",
    srcs = ["provider.gd", "image.gd"],
    visibility = ["//visibility:public"],
)
