# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_shared_library.bzl", "cc_shared_library")

cc_library(
    name = "probe",
    testonly = True,
    srcs = ["probe.cpp"],
    hdrs = ["echo.hpp"],
    deps = ["//contracts", "//providers:support"],
    copts = ["-fvisibility=hidden"],
)

cc_shared_library(
    name = "probe_provider",
    testonly = True,
    deps = [":probe"],
    dynamic_deps = ["//:image_runtime"],
    user_link_flags = ["-Wl,-z,defs", "-Wl,-rpath,$ORIGIN"],
)

cc_test(
    name = "image_check",
    srcs = ["image_check.cpp", "forms.hpp", "forms.cpp", "fulfillment.hpp", "fulfillment.cpp", "echo.hpp", "library.hpp", "library.cpp"],
    deps = ["//modules", "//images", "//operations:standard"],
    dynamic_deps = ["//:image_runtime"],
    data = ["//providers/cpu:cpu_provider", ":probe_provider"] + select({"//:cuda_enabled": ["//providers/cuda:cuda_provider"], "//conditions:default": []}),
    args = ["$(rootpath //providers/cpu:cpu_provider)", "$(rootpath :probe_provider)"] + select({"//:cuda_enabled": ["$(rootpath //providers/cuda:cuda_provider)"], "//conditions:default": []}),
)

exports_files(["godot_check.gd"])
