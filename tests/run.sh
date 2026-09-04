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

strtok_output="$(./build/strtok_probe)"
if [ "$strtok_output" != "strtok-ok" ]; then
    echo "unexpected strtok probe output: $strtok_output" >&2
    exit 1
fi

strerror_output="$(./build/strerror_probe)"
if [ "$strerror_output" != "strerror-ok" ]; then
    echo "unexpected strerror probe output: $strerror_output" >&2
    exit 1
fi

ctype_output="$(./build/ctype_probe)"
if [ "$ctype_output" != "ctype-ok" ]; then
    echo "unexpected ctype probe output: $ctype_output" >&2
    exit 1
fi

bsearch_output="$(./build/bsearch_probe)"
if [ "$bsearch_output" != "bsearch-ok" ]; then
    echo "unexpected bsearch probe output: $bsearch_output" >&2
    exit 1
fi

atoi_output="$(./build/atoi_probe)"
if [ "$atoi_output" != "atoi-ok" ]; then
    echo "unexpected atoi probe output: $atoi_output" >&2
    exit 1
fi

errno_output="$(./build/errno_probe)"
if [ "$errno_output" != "errno-ok" ]; then
    echo "unexpected errno probe output: $errno_output" >&2
    exit 1
fi

strtol_output="$(./build/strtol_probe)"
if [ "$strtol_output" != "strtol-ok" ]; then
    echo "unexpected strtol probe output: $strtol_output" >&2
    exit 1
fi

strtoul_output="$(./build/strtoul_probe)"
if [ "$strtoul_output" != "strtoul-ok" ]; then
    echo "unexpected strtoul probe output: $strtoul_output" >&2
    exit 1
fi

allocator_output="$(./build/allocator_probe)"
if [ "$allocator_output" != "allocator-ok" ]; then
    echo "unexpected allocator probe output: $allocator_output" >&2
    exit 1
fi

calloc_output="$(./build/calloc_probe)"
if [ "$calloc_output" != "calloc-ok" ]; then
    echo "unexpected calloc probe output: $calloc_output" >&2
    exit 1
fi

realloc_output="$(./build/realloc_probe)"
if [ "$realloc_output" != "realloc-ok" ]; then
    echo "unexpected realloc probe output: $realloc_output" >&2
    exit 1
fi

getenv_output="$(env -i MINI_GETENV_ALPHA=value MINI_GETENV_EMPTY= \
    MINI_GETENV_ALPHA_SUFFIX=suffix ./build/getenv_probe)"
if [ "$getenv_output" != "getenv-ok" ]; then
    echo "unexpected getenv probe output: $getenv_output" >&2
    exit 1
fi

stdio_output="$(./build/stdio_probe)"
expected_stdio_output='AABC

stdio-ok'
if [ "$stdio_output" != "$expected_stdio_output" ]; then
    echo "unexpected stdio probe output:" >&2
    printf '%s\n' "$stdio_output" >&2
    exit 1
fi

./build/memory_differential
./build/string_differential
./build/strtok_differential
./build/bsearch_differential
./build/atoi_differential
./build/strtol_differential
./build/strtoul_differential
./build/allocator_failure_test
./build/stdio_write_test

echo "runtime, syscall, memory, string, strtok, strerror, ctype, bsearch, atoi, errno, strtol, strtoul, allocator, calloc, realloc, getenv, stdio, and differential probes passed"
