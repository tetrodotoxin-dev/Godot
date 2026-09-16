# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "check",
    srcs = ["main.cpp"],
    args = ["$(rootpath //extensions/counter:counter_extension)"],
    data = ["//extensions/counter:counter_extension"],
    dynamic_deps = [
        "//:support",
        "@tetrodotoxin//ttx:runtime",
    ],
    deps = [
        "//extensions/counter:inspection",
        "//extensions/sampling:contracts",
        "@tetrodotoxin//ttx:concept",
        "@tetrodotoxin//ttx:semantic",
    ],
)

cc_test(
    name = "policy_check",
    srcs = ["policy.cpp"],
    args = ["$(rootpath //extensions/counter:counter_extension)"],
    copts = ["-frtti"],
    data = ["//extensions/counter:counter_extension"],
    dynamic_deps = [
        "//:support",
        "@tetrodotoxin//ttx:runtime",
    ],
    deps = [
        "//extensions/counter:inspection",
        "//extensions/sampling:contracts",
        "//gdextension",
    ],
)

cc_binary(
    name = "headless",
    srcs = ["main.cpp"],
    dynamic_deps = [
        "//:support",
        "@tetrodotoxin//ttx:runtime",
    ],
    deps = [
        "//extensions/counter:inspection",
        "//extensions/sampling:contracts",
        "@tetrodotoxin//ttx:concept",
        "@tetrodotoxin//ttx:semantic",
    ],
)
