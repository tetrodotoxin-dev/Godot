# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

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
