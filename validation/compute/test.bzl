# Copyright (c) 2023-present Matt Kaes and contributors

def _godot_test(ctx):
    executable = ctx.actions.declare_file(ctx.label.name)
    ctx.actions.symlink(
        output = executable,
        target_file = ctx.file.script,
        is_executable = True,
    )
    runfiles = ctx.runfiles(files = ctx.files.data)
    runfiles = runfiles.merge(ctx.attr.script[DefaultInfo].default_runfiles)
    return [DefaultInfo(
        executable = executable,
        runfiles = runfiles,
    )]

godot_runner = rule(
    implementation = _godot_test,
    executable = True,
    attrs = {
        "data": attr.label_list(allow_files = True),
        "script": attr.label(
            allow_single_file = True,
            mandatory = True,
        ),
    },
)
