# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_pkg//pkg:tar.bzl", "pkg_tar")
load("@rules_pkg//pkg:mappings.bzl", "pkg_files")
load("//source/godot/build:package.bzl", "sdk_libraries")

package(default_visibility = ["//visibility:public"])

sdk_libraries(name = "image_libraries", library = "//source/demo/adapters:demo")
sdk_libraries(name = "cpu_libraries", library = "//source/demo/imaging/cpu:cpu_provider")
sdk_libraries(name = "counter_libraries", library = "//source/demo/counter:counter_extension")
sdk_libraries(name = "cuda_libraries", library = "@compute_cuda//:cuda", tags = ["manual"])

pkg_files(
    name = "notices",
    srcs = ["licenses/Compute.txt", "//:LICENSE", "//source/godot:licenses/TTX.txt", "//source/godot:licenses/Perimortem.txt", "@godot_cpp//:LICENSE.md", "@pocketfft//:LICENSE.md"],
    prefix = "addons/godot_ttx/licenses",
    renames = {
        "licenses/Compute.txt": "Compute.txt",
        "//:LICENSE": "Lab-MIT.txt",
        "//source/godot:licenses/TTX.txt": "TTX.txt",
        "//source/godot:licenses/Perimortem.txt": "Perimortem.txt",
        "@godot_cpp//:LICENSE.md": "Godot-CPP-MIT.txt",
        "@pocketfft//:LICENSE.md": "PocketFFT-BSD.txt",
    },
)

# The lab package installs its providers beside the independent bridge SDK.
# The SDK itself contains no demo scene or provider-specific Godot classes.
pkg_tar(
    name = "addon",
    srcs = [
        ":image_libraries", ":cpu_libraries", ":counter_libraries", ":notices",
        "//source/godot:sdk_libraries", "//source/godot:sdk_manifest", "//source/godot:sdk_exporter",
        "//source/demo/adapters:resources", "//source/demo/adapters:scene", "//source/demo/adapters:scripts",
    ] + select({"//:cuda_enabled": [":cuda_libraries"], "//conditions:default": []}),
    extension = "tar",
)

filegroup(
    name = "web_payload",
    srcs = [":image_libraries", ":cpu_libraries", ":counter_libraries", "//source/godot:sdk_libraries"],
)
