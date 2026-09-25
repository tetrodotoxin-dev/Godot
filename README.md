# TTX for Godot

This lab brings native C++, CUDA and GDScript implementations into the same
Godot scene through TTX contracts. Each provider owns its implementation and
storage. Godot discovers the operations it can use, connects them into an image
pipeline, and displays the result.

[Try it in your browser](https://tetrodotoxin.dev/lab/) or read
[how the pieces fit together](https://tetrodotoxin.dev/lab/#following-an-overlay-update).
The browser runs the CPU and GDScript providers. CUDA requests report that the
required provider is unavailable.

[![CPU, CUDA and GDScript rendering the same scene](demo/preview.gif)](https://tetrodotoxin.dev/lab/)

Changing the overlay updates the downstream composition while retaining the
filtered image. Changing the source invalidates the connected pipeline. The
scene compares the results so differences between providers remain visible.
Independent C and C++ modules also publish classes through the same bridge,
including a counter Node and a scalar sampling interface.

## Run locally

The current native build targets Linux. It needs Bazel, Clang and Godot 4.7 or
later. The Bazel module expects sibling checkouts of
[Tetrodotoxin](https://github.com/tetrodotoxin-dev/Tetrodotoxin) and
[CUDA](https://github.com/tetrodotoxin-dev/CUDA), named `tetrodotoxin` and `cuda`.
It fetches the pinned Godot C++ bindings and PocketFFT dependency.

```sh
demo/run.sh --cpu
```

For CUDA, install the NVIDIA driver and CUDA toolkit, then run
`demo/run.sh`. `CUDA_ROOT` selects the toolkit location and defaults to
`/opt/cuda`. The CPU path works without a CUDA installation.

Use `CONFIGURATION=debug` for a debug build. The bridge targets Godot 4.7's
API and is checked against the installed Godot 4.7.2 engine.

## Two parts

`extension/` is the reusable TTX class loader. It negotiates exported class
and callable declarations, converts Godot values, and retains the resulting
runtime factories and bindings. It has no image, sampling or CUDA dependency.
Build `//extension:addon` to obtain that bridge on its own.

`demo/` contains the scene and all application behavior: image operations,
CPU/CUDA providers, GDScript adapters, the counter and sampling examples, and
build/run/export scripts. Its addon bundles the generic bridge with a separate
library registering the demo's image and CUDA Resources.

## Extend the lab

The [demo scene](demo/lab.tscn) composes renderer cards, source Resources and
policies. [Project configuration](demo/project.godot) maps import names to
modules and chooses which exported declarations become Godot classes.

Native image implementations and their contracts live in `demo/imaging/`.
The provider entry points sit beside their CPU or CUDA implementations.
`demo/adapters/` supplies the Godot Resources, editor integration and GDScript
provider adapters used by this scene. `demo/counter/` and `demo/sampling/`
show independent C and C++ class exports through the generic extension.

Build `//demo:addon` and use `demo/install.sh /path/to/project` to install the
complete demonstration in another project. The generic archive is
`.bin/bin/extension/godot_ttx.tar`; the demonstration archive is
`.bin/bin/demo/godot_ttx.tar`.

The [website](https://tetrodotoxin.dev/) carries the wider design and ongoing
research. [Web build instructions](https://tetrodotoxin.dev/lab/source/)
describe the browser engine configuration; this checkout's export entry is
`demo/export.py`.
