# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

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
