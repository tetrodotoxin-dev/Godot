# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

def _godot_cpp_impl(ctx):
    ctx.download_and_extract(
        url = "https://codeload.github.com/godotengine/godot-cpp/tar.gz/507ed9d840c01a3c5b2a39af8bb4000bfac30bf5",
        sha256 = "30da4ac295997061a6af81ae531510efd4e6a86e506eb4466edc0898ef3fe048",
        type = "tar.gz",
        strip_prefix = "godot-cpp-507ed9d840c01a3c5b2a39af8bb4000bfac30bf5",
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
        ctx.attr.bits,
    ])
    if generated.return_code:
        fail(generated.stdout + generated.stderr)
    ctx.symlink(ctx.attr.build_file, "BUILD")

godot_cpp_repository = repository_rule(
    implementation = _godot_cpp_impl,
    attrs = {
        "bits": attr.string(default = "64", values = ["32", "64"]),
        "generator": attr.label(mandatory = True, allow_single_file = True),
        "profile": attr.label(mandatory = True, allow_single_file = True),
        "build_file": attr.label(mandatory = True, allow_single_file = True),
    },
)
