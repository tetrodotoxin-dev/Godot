load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "contracts",
    hdrs = ["image.h", "provider.h", "offers.h"],
    includes = ["../../.."],
    deps = ["@ttx//:graph", "@compute//:compute"],
    visibility = ["//visibility:public"],
)
