# What the image timings measure

The lab has two useful performance questions. A user creating an image operation
pays for its result and graph ownership. A consumer holding an agreed callable
should pay only for dispatch and the implementation it invokes. Measuring the
first does not establish the cost of the second.

The original 1-pixel GDScript comparison exposed about 1.6 µs of extra time through
TTX. That was a **fresh image-operation round trip**, not a retained thunk call.
It excluded pixel readback. Backend warmup removed first-use preparation, but each
iteration still constructed a new Call, fulfilled its binding, created a provider
image and returned a new Godot Resource.

## Prepared form reuse

The follow-up now retains the compiled pixel bytes when deriving CPU, CUDA and
script images. Each result still owns its own payload and image lifetime. The
optional provider helper uses the existing `Core::Object<U8>` owner, and its
Representation carries the encoded size. There is no global cache, extra form
wrapper or retained source image. Initial publications compile independently.

The Godot return callback also constructs its object Variant directly in the
engine's return slot. This avoids a temporary Variant copy and destruction while
the local Ref keeps the Resource alive until the return slot owns it.

These are matched release measurements, using three runs per stage. The old
installed modules were preserved and loaded with the same native benchmark driver
used for the revised providers. The TTX baseline and public image ABI are unchanged.

| Native 1-pixel path | Before | Shared forms |
| --- | ---: | ---: |
| CPU retained inversion, including result and release | 183 ns | 53 ns |
| CPU host invocation with retained binding | 215 ns | 80 ns |
| CPU fresh graph Call, evaluation and release | 296 ns | 150 ns |
| CUDA retained inversion, including device work | 7.47 µs | 7.15 µs |
| CUDA fresh graph Call, evaluation and release | 7.92 µs | 7.30 µs |

Both native providers fulfill a callable in about 16 ns, or 20 ns through the
host wrapper. Direct dimension dispatch is about 1–2 ns. CPU results now request
two Perimortem allocations rather than three, and CUDA results one rather than
two. Adding the graph Call adds one request in either case. GPU allocation is a
separate driver operation and is not included in those allocator counts.

| Fresh Godot 1-pixel operation | Before | Shared forms | Direct return |
| --- | ---: | ---: | ---: |
| CPU | 1.389 µs | 1.144 µs | 1.095 µs |
| CUDA | 8.816 µs | 8.690 µs | 8.596 µs |
| GDScript through TTX | 2.811 µs | 2.654 µs | 2.576 µs |
| Direct GDScript control | 1.267 µs | 1.267 µs | 1.261 µs |

Native host overhead is below a microsecond in these measurements. The complete
Godot Resource-producing path remains above that target, even for a 1-pixel CPU
image. CUDA's synchronous device work must also remain visible in its totals.
Independent timing rows are not an exact subtraction ledger, particularly for
GPU work with variable driver scheduling and clocks.

The form test rejects the old provider, verifies sharing through two generations,
releases each originating image before observing its derivative, checks a new
shape cannot change an old form, and checks active descriptor storage returns to
its starting level. CUDA additionally proves the source allocation ends while the
derivative survives. The complete native, script, refusal, reentrancy and rendered
three-provider tests pass after both changes.

Larger operations remain dominated by their algorithms. The final 2048×1024
63×63 convolution measured 205.1 ms on CPU and 5.77 ms on CUDA. The small constant
savings do not materially change that comparison. A further reduction in the
Godot path needs a measured decision about repeated Resource construction or its
call boundary, rather than caching a result and labeling it another execution.

## A scalar comparison without Resource construction

The [sampling example](sampling.md) now provides the corresponding retained
compute path. Setup owns one function binding; each invocation returns an integer.
A one-sample CPU call measured about **2.3 ns natively and 28.7 ns through Godot**.
It creates neither an image nor a graph result. CPU/CUDA partitions return exactly
the same combined count as one complete interval.

That result isolates a different ownership choice rather than making the original
image operation disappear. Large sampling work also exercises real GPU computation:
16.8 million samples took about 3.13 ms on CPU and 34 µs on CUDA, including scalar
readback. See the example for the full contract, tests and measurement limits.

## Follow the actual operation

1. [TtxImage::apply](../extension/ttx_image.cpp) converts the arguments, constructs
   a native Call and evaluates it before exposing a Resource.
2. [Call::evaluate_value](../images/call.cpp) fulfills a binding on its first
   evaluation. A new Call therefore negotiates even when another Call previously
   used the same operation on the same source.
3. The retained typed binding invokes the provider directly. For the script
   provider, [Image::select](../providers/gdscript/image.cpp) caches the source's
   published Godot Callable. Later TTX fulfillment does not reenter script
   `fulfill(UUID)` for that populated slot.
4. The provider constructs an immutable result. The shared optional
   [native Image publisher](../providers/image.cpp) retains its source form for
   derived results. A new source publication compiles its own RGBA8 form.
5. The extension returns a new `TtxImage` Resource. Replacing the previous script
   result also releases its graph and provider image.

An existing Call behaves differently. An unchanged dependency graph returns its
cached answer. Changing an argument with a stable receiver reuses its binding
while executing the operation again. Replacing the receiver requires fulfillment
against that new subject. Cached observation must not be presented as repeated
filter execution.

## Baseline tiny-image mechanics

Measured on 2026-09-13 with a Ryzen 9 9950X3D, release `-O3` builds and TTX
`e6f236e8b4092c1b32cb4c44ed265bfe9dca7da1`. Values are medians of three batch
means. No CPU affinity or clock policy was changed. Short timings vary with core
placement and frequency, so these are observations rather than budgets.

| Native path | ns per iteration | Perimortem allocation requests |
| --- | ---: | ---: |
| Direct image dimension table call across the loaded module | 1.6 | 0 |
| Host dimension observation, including its observation scope | 4.0 | 0 |
| Core callable fulfillment | 16 | 0 |
| Host callable fulfillment | 20 | 0 |
| Retained inversion, including a new 1-pixel result and release | 181 | 3 |
| Host inversion with a retained binding | 227 | 3 |
| Host inversion fulfilling again | 232 | 3 |
| Fresh native graph Call, evaluation and release | 276 | 4 |
| Observe an unchanged retained Call | 9.9 | 0 |
| Compile/write the same 1-pixel RGBA8 form | 133 | 1 |

The [mechanics executable](../tests/performance/main.cpp) uses the production
loaded CPU provider. The direct dimension call is a simple stored answer through
its bootstrap table. It is not an inversion or a memory copy. Fulfillment rows
measure setup independently of operation invocation. Inversion rows include the
provider's result construction and reclamation.

Allocation requests count Bibliotheca traffic, not every `malloc`, Godot
allocation or device allocation. No process-wide allocation hook is installed.
Each native action releases its result inside the measured iteration, while
script assignment replaces the preceding result after creating the next one.
Consequently, subtracting these independently timed rows is not exact accounting.

The matching script-facing measurements were:

| 1-pixel inversion | µs per fresh operation |
| --- | ---: |
| Direct GDScript | 1.262 |
| GDScript through TTX | 2.806 |
| Native CPU through TtxImage | 1.377 |
| CUDA through TtxImage | 9.001 |

The script detour adds about 1.54 µs here. Native fulfillment is measured in tens
of nanoseconds, so eliminating that handshake alone cannot remove the detour.
CUDA also allocates its result, launches a kernel and synchronizes completion.
A 1-pixel image cannot amortize those costs.

## Baseline profile

Separate one-million-operation runs were sampled with `perf`, using the actual
installed extension. The addon, runtime and CPU provider matched the binaries
used by the native benchmark. The percentages below are **inclusive sampled
cycles** across each process, including startup and teardown. Nested rows overlap
and must not be summed. Godot's installed executable is stripped, so attribution
inside its engine remains less precise than attribution to our named functions.

| Sampled path | CPU backend | Script backend |
| --- | ---: | ---: |
| Create the returned TtxImage Resource | 20.4% | 9.5% |
| Provider Image constructor, including its pixel form | 12.2% | 5.3% |
| Host callable fulfillment | 2.1% | 1.2% |
| Admit the returned script object | — | 18.5% |
| Invoke the script Callable, including its implementation | — | 31.5% |

The sampled stacks also contain Godot reference operations and libc mutex and
read/write lock routines. These percentages establish where work lands, not that
all locking is redundant. The source stability and module lifetime contracts
remain necessary.

This profile motivated the form sharing and direct return changes above. The
133 ns form benchmark isolates repeated preparation, while the matched before/after
runs measure the actual effect of removing it. Repeated execution without creating
a Godot Resource still requires choosing an explicit reusable operation owner.
The existing cached graph already avoids work when its answer has not changed.

## Scale the useful work

The [script benchmark](../tests/gdscript/benchmark.gd) now reaches 2048×1024 pixels
and includes 63×63 convolution at both large sizes. These are the same three-run
medians, on an RTX 5070 with driver 610.57.04 and Godot 4.7.2. Native inversion uses
at least 50 samples and convolution 10, after three warmups. Final readback is
outside the operation timer.

| Operation | Dimensions | CPU | CUDA | CPU / CUDA |
| --- | --- | ---: | ---: | ---: |
| Invert | 256×128 | 16.24 µs | 8.16 µs | 2.0× |
| Invert | 1024×512 | 262.52 µs | 57.20 µs | 4.6× |
| Invert | 2048×1024 | 1.064 ms | 0.089 ms | 12.0× |
| Convolve, 7×7 | 64×32 | 134.7 µs | 22.7 µs | 5.9× |
| Convolve, 63×63 | 1024×512 | 48.83 ms | 0.99 ms | 49.3× |
| Convolve, 63×63 | 2048×1024 | 205.79 ms | 5.79 ms | 35.5× |

Larger inversion is about 25 ms in GDScript at 1024×512 and 101 ms at 2048×1024.
Direct GDScript and the TTX script path fall within measurement variation there.
The small script convolution takes about 11.5 ms. Large spatial convolution is
explicitly skipped above two million source/kernel sample positions so this
synchronous diagnostic remains practical.

These are complete provider operations, not isolated kernels. Source creation and
upload are outside the timer. CPU convolution creates and destroys FFTW plans on
each invocation, although FFTW wisdom is warm. CUDA retains cuFFT plans and scratch
by dimensions. That resource-lifetime difference belongs in any interpretation
of the comparison and is a concrete CPU-provider optimization opportunity.

Readback can dominate a small amount of GPU work. The single final 2048×1024
inversion readback had a three-run median of 1.317 ms for CUDA and 0.664 ms for CPU.
The corresponding convolution readbacks were 1.336 ms and 1.026 ms. These are
individual observations, not throughput estimates or an exact full-pipeline sum.
The retained CUDA intermediates matter precisely because every operation need
not download them.

Each batch checks its final output against the other implementations. Inversion
requires identical bytes, convolution permits one byte of rounding difference.
For the large convolution cases, CPU and CUDA agreement is the available check.
The existing independent numerical tests remain unchanged.

## Reproduce

After building and installing the release addon using the development guide:

```sh
bazel run //tests/performance:mechanics --config=release --config=cuda
godot --headless --path demo --script "$PWD/tests/gdscript/benchmark.gd" -- --cuda
```

Omit `--config=cuda` from the native command and `--cuda` from the script command
to run without the GPU provider. The native benchmark is isolated
from the addon and acceptance test executables. Neither command treats a timing
threshold as a correctness requirement.

For an actual Godot-path profile, provide a local output location to perf:

```sh
perf record -e cycles:u -F 997 --call-graph dwarf -o /tmp/ttx-image.perf -- \
  godot --headless --path demo --script "$PWD/tests/gdscript/benchmark.gd" -- --profile --cpu
DEBUGINFOD_URLS='' perf report -i /tmp/ttx-image.perf --stdio --children --call-graph none
```

Omit `--cpu` to sample the script provider through TTX. Add `--direct` to sample
the direct script implementation. Profiled timings are kept separate from the
unprofiled comparison above.
