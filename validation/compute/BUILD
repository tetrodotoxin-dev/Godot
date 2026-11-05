# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_pkg//pkg:tar.bzl", "pkg_tar")
load("//source/godot/build:package.bzl", "sdk_libraries")

exports_files(["test.sh"], visibility = ["//:__pkg__"])

sdk_libraries(name = "cpu", library = "@compute//:cpu", manifest = True)
sdk_libraries(name = "cuda", library = "@compute//:cuda", manifest = True)

filegroup(
    name = "project",
    srcs = [
        "project.godot",
        "smoke.gd",
        "smoke.tscn",
    ],
    visibility = ["//:__pkg__"],
)

pkg_tar(
    name = "addon",
    srcs = [
        ":cpu",
        ":cuda",
        "//source/godot:sdk_libraries",
        "//source/godot:sdk_manifest",
        "//source/godot:sdk_exporter",
    ],
    extension = "tar",
    visibility = ["//:__pkg__"],
)
