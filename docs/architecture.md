# Architecture and observable contracts

The lab separates three jobs that were intertwined in the earlier prototype:
selecting a callable implementation, retaining its resources, and deciding
whether its answer is still current. That separation lets an image keep its
pixels on a GPU while participating in an ordinary Godot dependency graph.

## From publication to invocation

A loaded provider exports [godot_image_provider_open_v2](../contracts/provider.h).
Opening it supplies an owned factory. The same factory record can also be admitted
from an existing host object, as the GDScript provider demonstrates. Creating an image returns one owned image
reference and its bootstrap [control table](../contracts/image.h). The host can
retain or release that reference, inspect dimensions and obtain an already usable
Semantic Query. This initial C ABI agreement makes negotiation possible without
first negotiating how to call `bind`.

`Simulacra::fulfill` then establishes an optional operation:

1. Query binds the Thunk protocol UUID.
2. Thunk fulfills the requested operation UUID, calling convention and prepared
   Data Representation of the operation table.
3. The consumer retains the returned table and opaque receiver with the image
   whose publication supplied them.

The current realization is System V AMD64. The operation UUID fixes its actual
signature, including `self`, argument meaning, failure behavior and result
ownership. Data validates the table's physical representation. These are separate
obligations: Invert and Convolve each contain one function pointer, but matching
those pointer slots would not make their signatures interchangeable.

[Operation](../images/operation.hpp) captures the exact typed invocation when the
host publishes a method. Its argument family tells the Godot boundary how to
convert inputs; it does not choose which callable contract to invoke. The
[test Echo contract](../tests/echo.hpp) deliberately has the same host argument
family as Invert and a different native signature. It exercises this distinction
through the production consumer and a separately loaded provider.

A retained [Call](../images/call.cpp) invokes its typed thunk directly until the
receiver publication changes. It does not rediscover callable metadata or fit
arguments on every operation. Core also permits a known native C++ owner to
supply the same Handle directly; Unsupported falls back to negotiation, while
Pending and Rejected stop that attempt. Loaded Godot providers use the Core route,
so the product does not depend on that local optimization.

The receiver belongs to the provider. A thunk may recover its own native state;
the consumer cannot cast it to CPU Image or CUDA Image. A provider may supply a
projection whose state has an entirely different layout. Calls here complete
synchronously. A network or asynchronous implementation would need its own
execution policy while satisfying the advertised callable's contract.

## Dependency boundaries in the source and build

| Owner | What it knows |
| --- | --- |
| `contracts` | Image carriers, UUIDs, kernel observations and pixel transport. |
| `providers:support` | Optional C++ publication of control/Thunk/Block tables. |
| `providers/cpu` | Perimortem host buffers, CPU kernels and FFTW plans. |
| `providers/cuda` | CUDA context, device allocations, NVRTC and cuFFT. |
| `providers/gdscript` | Godot object retention, Variant conversion and actual script image operations. |
| `modules` | Optional native library loading, symbols and executable lifetime. |
| `images` | Operation publications, module owners and a Concept dependency graph. |
| `extension` | Godot Resources, Variants, method registration and textures' input bytes. |

The two backend libraries depend on public contracts and the optional
[Providers::Image](../providers/image.hpp) helper. They have no dependency on one
another or on `images`, Godot, or TTX Concept. Their only shared implementation
policy is the explicitly chosen publication helper and supporting runtime. A C
backend can provide the public tables directly without inheriting that class.

One shared `image_runtime` library owns the canonical Perimortem/Data/Semantic
runtime used across these modules. This preserves allocator ownership. It does
not contain the host graph. The extension and native test link the host owners;
provider targets have only their own implementation and contract dependencies.
[check-boundaries.sh](../tools/check-boundaries.sh) verifies those closures.

The [standard vocabulary](../operations/standard.cpp) lends a static array of
Operation publications. It allocates no global dynamic catalogue that would
need release after a thread-local allocator has shut down. A dynamic vocabulary
can still borrow an array from an explicit owner whose lifetime is known.

## Pixel observation is a separate contract

All three providers currently publish pixels through Data Block. Semantic Flow agrees
on the pixel Representation, then Copy supplies Storage for a synchronous commit.
CPU copies its retained bytes. CUDA downloads directly into the supplied surface.
GDScript returns a PackedByteArray observation, which its bridge copies into that
surface after checking the promised extent.
The destination belongs to that operation; it is not saved on a shared writer
where another reader could replace it.

Offering Block does not imply support for Direct, Shared or Fragment. Likewise,
fulfilling convolution transfers a function table and receiver information, not
image pixels. Device storage can remain resident through inversion, convolution
and subsequent composition. A foreign CPU or script overlay is observed through its public
pixel interface and uploaded for the CUDA composite.

[Contracts::Pixels](../contracts/pixels.cpp) consumes only this public surface.
Both the host's final readback and a provider's foreign-image observation use it,
so CUDA can consume a CPU or script overlay without linking its implementation or the
host dependency graph.

## Ownership and invalidation

The native retention chain keeps executable code alive until its final use:

- Host Image owns a native image reference and retains its loaded Provider.
- Image releases the native value before releasing Provider.
- Provider releases its factory before its optional Library owner unloads the module.
- A script factory retains its Godot object; each script image retains its own
  publication and the selected Callables. Godot supplies their executable lifetime.
- CUDA allocations retain Runtime, which owns the context, Program and FFT plans.
- Runtime destroys those child resources before releasing its context.

Pixel descriptors and payload allocations have related but distinct lifetimes.
A table's address or a copied receiver pointer alone does not retain either the
image or its module. Native reload is disabled while such publications can exist.

[Source](../images/source.hpp) owns a current immutable image. Publishing new
pixels replaces that value and advances its revision. A [Call](../images/call.hpp)
retains its receiver and arguments, optional kernel storage, binding, cached
output and dependency revisions. A pull visits shared ancestors once, then only
recomputes calls whose inputs changed.

Moving the overlay changes a composite argument, so the composite reuses its
background binding and blur. Changing the background replaces the receiver
publication and requires another binding. Failures are cached until inputs
change, allowing corrected sources to recover. The graph still visits reachable
dependencies during a pull; it does not promise constant-time graph validation.

`snapshot()` retains the current immutable answer independently. Ordinary derived
results follow changes to their dependencies. Historical retention and current
validity therefore remain separate promises.

The graph is also navigable through Abstract: operation, receiver, numbered
arguments and current value expose the actual owners. A value's Query can fulfill
an operation without selecting a native C++ image type. Those views borrow the
current publication; retaining Image preserves the historical value itself.

Publication, evaluation and final release currently occur on one worker.
An Observation scope rejects source replacement and provider reconfiguration from
provider callbacks while the current answer is borrowed. Source replacement installs
its new value/revision before releasing old state, so destruction callbacks see a
complete publication. Provider
diagnostic bytes are borrowed until the next call, so owners copy errors they
retain. Unsupported, Pending and Rejected remain distinct through the native
helper and host diagnostics; refusal is not an instruction to bypass a policy.
Cross-worker transfer, durable subject identity and scheduling remain later work.

## Computation and numerical agreement

CPU convolution uses FFTW; CUDA uses cuFFT. Both pad the source and centered kernel,
transform channels, multiply spectra and crop back to the requested image. The
CPU [convolution stages](../providers/cpu/convolve.cpp) and CUDA
[kernels](../providers/cuda/kernels.cu) keep those steps visible. Alpha is preserved.

GDScript implements the same image contract with a spatial convolution loop in
[image.gd](../providers/gdscript/image.gd). Its inversion and composition also run
in the script; their native bridge performs only contract, ownership and argument
conversion. A Callable is selected on first successful fulfillment and retained
with its publication. The script's own delegate objects remain its responsibility.

The visual comparison reports the largest observed channel difference. Native
acceptance uses independent reference calculations: inversion and source/alpha
preservation are exact, while convolution and composition allow a one-byte RGB
difference. Equal outputs in a demo frame do not prove equality for every accepted
coefficient vector. Tightening the numerical promise needs explicit input-domain,
rounding, cancellation, padding and clamping review, preserving those oracles.

The displayed compute time excludes first preparation and explicit readback. It
still includes the actual operation path, rather than measuring only FFT kernels.
Overlay refresh timing includes invalidated composition and readback. Neither
number measures TTX dispatch in isolation or establishes an across-machine speed
ratio. The native tests separately count transfers, FFT plan builds and live image
allocations so a visually correct result cannot hide unnecessary device work.

## What this example establishes

The lab demonstrates negotiated callable substitution, a navigable persistent
image graph, mixed-provider composition and selective invalidation. It remains
an external consumer with its own Godot/FFTW/CUDA deployment requirements.

It does not yet demonstrate independent language frontends, a Build dialect,
LSP source invalidation, arbitrary cross-process transport or TTX self-hosting.
The C fixtures in the sibling TTX tests exercise the lower foreign fulfillment
boundary; this repository includes two native image providers and one running in GDScript.
Those distinct evidence boundaries are useful when the next examples begin
putting pressure on the same infrastructure.

The script provider adds a managed runtime to this ownership model without
requiring Data or Semantic changes. [Provider patterns](provider-patterns.md)
records the resulting lifetime and conversion boundaries, including the parts
that remain candidates for a reusable extension.
