#!/bin/sh
set -eu

hello_output="$(./build/hello)"
if [ "$hello_output" != "hello from mini-libc" ]; then
    echo "unexpected hello output: $hello_output" >&2
    exit 1
fi

set +e
runtime_output="$(MINI_LIBC_SENTINEL=present ./build/runtime_probe alpha beta)"
runtime_status=$?
set -e
if [ "$runtime_status" -ne 37 ]; then
    echo "runtime probe returned $runtime_status, expected 37" >&2
    exit 1
fi
if [ "$runtime_output" != "runtime-ok" ]; then
    echo "unexpected runtime probe output: $runtime_output" >&2
    exit 1
fi

syscall_output="$(./build/syscall_probe)"
if [ "$syscall_output" != "syscall-ok" ]; then
    echo "unexpected syscall probe output: $syscall_output" >&2
    exit 1
fi

memory_output="$(./build/memory_probe)"
if [ "$memory_output" != "memory-ok" ]; then
    echo "unexpected memory probe output: $memory_output" >&2
    exit 1
fi

string_output="$(./build/string_probe)"
if [ "$string_output" != "string-ok" ]; then
    echo "unexpected string probe output: $string_output" >&2
    exit 1
fi

atoi_output="$(./build/atoi_probe)"
if [ "$atoi_output" != "atoi-ok" ]; then
    echo "unexpected atoi probe output: $atoi_output" >&2
    exit 1
fi

./build/memory_differential
./build/string_differential
./build/atoi_differential

echo "runtime, syscall, memory, string, atoi, and differential probes passed"
