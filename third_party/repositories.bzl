# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Pinned Godot bindings and the explicit local CUDA SDK boundary."""

def _godot_cpp_impl(ctx):
    ctx.download_and_extract(
        url = "https://codeload.github.com/godotengine/godot-cpp/tar.gz/e83fd0904c13356ed1d4c3d09f8bb9132bdc6b77",
        sha256 = "d4c03daf46c8bef4544614182b0762c3e32789151c7f8f44e8eb2053d5cddeba",
        type = "tar.gz",
        strip_prefix = "godot-cpp-e83fd0904c13356ed1d4c3d09f8bb9132bdc6b77",
    )
    python = ctx.which("python3")
    if not python:
        fail("godot-cpp binding generation requires python3")
    # Reading these inputs makes their contents part of repository invalidation.
    # Passing only their paths to Python leaves a changed class profile invisible
    # to Bazel and can preserve an obsolete generated API.
    ctx.file("generate_godot.py", ctx.read(ctx.attr.generator))
    ctx.file("build_profile.json", ctx.read(ctx.attr.profile))
    generated = ctx.execute([
        python,
        ctx.path("generate_godot.py"),
        ctx.path("."),
        ctx.path("build_profile.json"),
    ])
    if generated.return_code:
        fail(generated.stdout + generated.stderr)
    ctx.symlink(ctx.attr.build_file, "BUILD")

godot_cpp_repository = repository_rule(
    implementation = _godot_cpp_impl,
    attrs = {
        "generator": attr.label(mandatory = True, allow_single_file = True),
        "profile": attr.label(mandatory = True, allow_single_file = True),
        "build_file": attr.label(mandatory = True, allow_single_file = True),
    },
)

def _cuda_impl(ctx):
    root = ctx.os.environ.get("CUDA_ROOT", "/opt/cuda")
    if not ctx.path(root + "/include/cuda.h").exists:
        fail("CUDA headers missing under " + root + "; set CUDA_ROOT or omit --config=cuda")
    ctx.symlink(root + "/include", "include")
    ctx.symlink(root + "/lib64", "lib64")
    ctx.file("BUILD", """
load("@rules_cc//cc:cc_import.bzl", "cc_import")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
package(default_visibility = ["//visibility:public"])
cc_import(name = "nvrtc", shared_library = "lib64/libnvrtc.so")
cc_import(name = "cufft", shared_library = "lib64/libcufft.so")
cc_import(name = "driver", interface_library = "lib64/stubs/libcuda.so", system_provided = True)
cc_library(
    name = "cuda",
    hdrs = glob(["include/**/*.h"]),
    includes = ["include"],
    deps = [":driver", ":nvrtc", ":cufft"],
    linkopts = ["-Wl,-rpath,%s/lib64"],
)
""" % root)

cuda_repository = repository_rule(
    implementation = _cuda_impl,
    environ = ["CUDA_ROOT"],
    local = True,
)

def _fftw_impl(ctx):
    root = ctx.os.environ.get("FFTW_ROOT", "/usr")
    ctx.symlink(root + "/include/fftw3.h", "include/fftw3.h")
    ctx.symlink(root + "/lib/libfftw3f.so", "lib/libfftw3f.so")
    ctx.file("BUILD", """
load("@rules_cc//cc:cc_import.bzl", "cc_import")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
package(default_visibility = ["//visibility:public"])
cc_import(name = "library", shared_library = "lib/libfftw3f.so")
cc_library(name = "fftw", hdrs = ["include/fftw3.h"], includes = ["include"], deps = [":library"])
""")

fftw_repository = repository_rule(implementation = _fftw_impl, environ = ["FFTW_ROOT"], local = True)
