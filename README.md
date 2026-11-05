# TTX for Godot

TTX for Godot loads independently built modules and exposes their graph
contracts to GDScript. Providers use the TTX SDK and keep their implementation
separate from Godot. The extension discovers declarations it can represent and
leaves unsupported declarations available through their original graphs.

The SDK contains the GDExtension and its runtime dependencies. It ships no C++
headers. Configure `ttx/modules` with the provider libraries a project uses;
feature overrides select the corresponding libraries for an export.

## Image lab

The [example project](example/project.godot) is GDScript. It loads the Compute
CPU module and uses the generated Engine, Buffer and Commands classes alongside
a script provider. Each provider controls its storage and execution. The scene
compares their results and demonstrates selective updates.

[Try the browser demo](https://tetrodotoxin.dev/lab/).

[![Image lab](example/preview.gif)](https://tetrodotoxin.dev/lab/)

## Build

Install Bazel, Python 3 and Godot 4.7 or later. Bazel acquires the pinned SDKs
and compiler dependencies.

```sh
bazel build :sdk --config=release
bazel run :tests
```

The SDK archive is `.bin/bin/source/godot/godot-ttx-sdk.zip`. Extract it into a
Godot project.

Build the native addon, install it into the example and open the project in the
Godot editor:

```sh
bazel build :demo_addon --config=release --config=cuda
tools/install.sh example
godot --editor --path example
```

Omit `--config=cuda` to package only the CPU provider. To build, install and run
the native example directly:

```sh
tools/run.sh
```

`tools/install.sh` accepts any existing Godot project directory. The project
must contain `project.godot`.

`source/godot/` owns the extension. `example/` contains the ordinary Godot
project and its GDScript composition. It has no native build target and is not
part of the SDK. `tools/export.py` builds a Web export using a matching
extension-enabled Godot template.

TTX supplies graph discovery and contract negotiation. Coherence supplies the
Type, Field and Callable model that the bridge compiles into Godot classes and
methods. A class is published only when its description, forms and provider
access establish a complete usable binding.
