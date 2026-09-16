#!/usr/bin/env bash
# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$root"
bazel build //:addon //tests/classes:rejected_extension //tests/classes:values_extension --config="${CONFIGURATION:-release}" "$@"
project=$(mktemp -d)
trap 'rm -rf -- "$project"' EXIT
mkdir -p "$project/addons/godot_ttx" "$project/.godot"
tar -xf .bin/bin/godot_ttx.tar -C "$project/addons/godot_ttx"
cp .bin/bin/tests/classes/libvalues_extension.so .bin/bin/tests/classes/librejected_extension.so "$project/addons/godot_ttx/"
cp tests/classes/counter.tscn "$project/"
printf '%s\n' res://addons/godot_ttx/godot_ttx.gdextension > "$project/.godot/extension_list.cfg"
bridge_before=$(sha256sum "$project/addons/godot_ttx/libgodot_ttx.so")

python3 tests/classes/configure.py "$project" valid
godot --headless --path "$project" --script "$root/tests/classes/check.gd"
godot --headless --path "$project" --script "$root/tests/classes/timing.gd"
python3 tests/classes/configure.py "$project" bad_base
godot --headless --path "$project" --script "$root/tests/classes/declined.gd" -- BadChild > "$project/base.log" 2>&1
cat "$project/base.log"
rg -q 'requires an instantiable native Godot base' "$project/base.log"
python3 tests/classes/configure.py "$project" none
godot --headless --path "$project" --script "$root/tests/classes/absent.gd"

python3 tests/classes/configure.py "$project" rejected
godot --headless --path "$project" --script "$root/tests/classes/check.gd" > "$project/rejections.log" 2>&1
cat "$project/rejections.log"
python3 - "$project/rejections.log" <<'PY'
from pathlib import Path
import sys
log = Path(sys.argv[1]).read_text()
assert 'name is occupied' in log
assert 'could not be imported' in log
assert 'Module acquisition could not initialize' in log
assert log.index('REJECTED publication released') < log.index('REJECTED module unloaded')
assert 'PASS published classes' in log
PY

python3 tests/classes/configure.py "$project" counter
for policy in deny pending unsupported; do
    TTX_COUNTER_POLICY="$policy" godot --headless --path "$project" --script "$root/tests/classes/declined.gd" -- TtxCounter > "$project/policy.log" 2>&1
    cat "$project/policy.log"
    rg -q 'PASS class compilation declined' "$project/policy.log"
done

python3 tests/classes/configure.py "$project" values
godot --headless --path "$project" --script "$root/tests/classes/values.gd"
TTX_VALUES_BAD_SIGNATURE=1 godot --headless --path "$project" --script "$root/tests/classes/declined.gd" -- TtxValues > "$project/signature.log" 2>&1
cat "$project/signature.log"
rg -q 'no supported synchronous callable realization' "$project/signature.log"
for failure in TTX_VALUES_REFUSE_CREATE TTX_VALUES_REFUSE_INSTANCE; do
    env "$failure=1" godot --headless --path "$project" --script "$root/tests/classes/construction.gd" > "$project/construction.log" 2>&1
    cat "$project/construction.log"
    rg -q 'PASS runtime construction failure' "$project/construction.log"
    python3 - "$project/construction.log" "$failure" <<'PYTEST'
from pathlib import Path
import sys
log = Path(sys.argv[1]).read_text()
assert log.index('VALUES discovery released') < log.index('PASS runtime construction failure')
assert log.index('VALUES factory released') < log.index('VALUES module unloaded')
if sys.argv[2] == 'TTX_VALUES_REFUSE_INSTANCE':
    assert log.index('VALUES instance released') < log.index('VALUES factory released')
else:
    assert 'VALUES instance released' not in log
PYTEST
done

for case in type few many; do
    status=0
    godot --headless --path "$project" --quit-after 2 --script "$root/tests/classes/invalid.gd" -- "$case" > "$project/$case.log" 2>&1 || status=$?
    [[ $status == 0 || $status == 1 ]]
    cat "$project/$case.log"
    rg -q 'SCRIPT ERROR: Invalid (call|type)' "$project/$case.log"
done
[[ "$bridge_before" == "$(sha256sum "$project/addons/godot_ttx/libgodot_ttx.so")" ]]
echo 'PASS TTX graphs, emitted classes, policies and failures with unchanged bridge binary'
