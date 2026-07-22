#!/usr/bin/env bash
#
# GPU parity gate: encodes a deterministic image with the CPU encoder
# and with EVERY OpenCL GPU device found on this machine, at a set of
# root counts covering all kernel path geometries, and fails when any
# output differs from the CPU reference.
#
# Run from the regtest directory: ./gpu-parity.bash

cd "$(dirname "$0")"

BIN=../dvdisaster
test -x $BIN || BIN=../dvdisaster.exe
if ! test -x $BIN; then
    echo "gpu-parity: build dvdisaster first"
    exit 1
fi

if command -v sha256sum >/dev/null; then
    HASH="sha256sum"
else
    HASH="shasum -a 256"
fi

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

# Enumerate the GPU devices

devices=$($BIN --encoding-device list 2>&1 | grep -Eo '^  gpu:[0-9]+' | grep -Eo '[0-9]+')
if test -z "$devices"; then
    echo "gpu-parity: no GPU devices found; nothing to compare (PASS by absence)"
    exit 0
fi

echo "gpu-parity: testing GPU device(s):" $devices

$BIN --regtest --debug -i$TMP/image.iso --random-image 21000 >/dev/null 2>&1

fail=0
for roots in 8r 32r 33r 100r 170r; do
    $BIN --regtest --debug --set-version 0.80 -i$TMP/image.iso -e$TMP/cpu.ecc \
	 -mRS03 -o file -n $roots -c --encoding-device cpu >/dev/null 2>&1
    ref=$($HASH $TMP/cpu.ecc | cut -d' ' -f1)

    for d in $devices; do
	rm -f $TMP/gpu.ecc
	$BIN --regtest --debug --set-version 0.80 -i$TMP/image.iso -e$TMP/gpu.ecc \
	     -mRS03 -o file -n $roots -c --encoding-device gpu:$d >/dev/null 2>&1
	got=$($HASH $TMP/gpu.ecc 2>/dev/null | cut -d' ' -f1)
	if test "$ref" == "$got"; then
	    echo "gpu-parity: $roots gpu:$d OK"
	else
	    echo "gpu-parity: $roots gpu:$d MISMATCH (cpu $ref, gpu $got)"
	    fail=1
	fi
    done
done

if test $fail -eq 0; then
    echo "gpu-parity: PASS (all devices bit-identical to the CPU encoder)"
else
    echo "gpu-parity: FAIL"
fi
exit $fail
