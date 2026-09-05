#!/bin/sh
set -eu

./tests/verify-compiler-neutral-c.sh

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

check_termination_case() {
    mode="$1"
    expected_status="$2"
    expected_output="$3"

    set +e
    actual_output="$(./build/runtime_probe "$mode")"
    actual_status=$?
    set -e

    if [ "$actual_status" -ne "$expected_status" ]; then
        echo "termination mode $mode returned $actual_status, expected $expected_status" >&2
        exit 1
    fi
    if [ "$actual_output" != "$expected_output" ]; then
        echo "termination mode $mode output '$actual_output', expected '$expected_output'" >&2
        exit 1
    fi
}

check_termination_case return-exit 23 CBA
check_termination_case call-exit 24 CBA
check_termination_case quick-exit 25 ''
check_termination_case capacity 26 XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
check_termination_case buffered-return 27 AB
check_termination_case buffered-call 28 AB
check_termination_case buffered-quick 29 ''
check_termination_case buffered-flush-quick 30 A

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

stdio_stderr_file=build/stdio_probe.stderr
set +e
stdio_output="$(printf 'xy' | ./build/stdio_probe 2>"$stdio_stderr_file")"
stdio_status=$?
set -e
stdio_stderr_output="$(cat "$stdio_stderr_file")"
rm -f "$stdio_stderr_file"
if [ "$stdio_status" -ne 0 ]; then
    echo "stdio probe returned $stdio_status" >&2
    exit 1
fi
expected_stdio_output='ABCDEFG
fmt:-42:17:4000000000:11:2a:2A:Z:ok:%
pad:[-00042][xy   ][0x2a][011][abc][    0023][  0007]
len:-5:250:-30000:60000:-1234567890:4000000000:-5000000000:9000000000
star:[12   ][wide][    002a]
sign:[+7][ 7][00000042][0X2A]
edge:[][0][     ]
stdio-ok'
if [ "$stdio_output" != "$expected_stdio_output" ]; then
    echo "unexpected stdio stdout:" >&2
    printf '%s\n' "$stdio_output" >&2
    exit 1
fi
expected_stdio_stderr='stderr-ok
format-err:0x00002a:Q   '
if [ "$stdio_stderr_output" != "$expected_stdio_stderr" ]; then
    echo "unexpected stdio stderr:" >&2
    printf '%s\n' "$stdio_stderr_output" >&2
    exit 1
fi

file_stream_path=build/file-stream-probe.tmp
rm -f "$file_stream_path"
set +e
file_stream_output="$(./build/file_stream_probe "$file_stream_path")"
file_stream_status=$?
set -e
if [ "$file_stream_status" -ne 0 ]; then
    echo "file stream probe returned $file_stream_status" >&2
    rm -f "$file_stream_path"
    exit 1
fi
if [ "$file_stream_output" != "file-stream-ok" ]; then
    echo "unexpected file stream output: $file_stream_output" >&2
    rm -f "$file_stream_path"
    exit 1
fi
if [ "$(cat "$file_stream_path")" != "ABC" ]; then
    echo "unexpected owned file contents" >&2
    rm -f "$file_stream_path"
    exit 1
fi
rm -f "$file_stream_path"

block_io_path=build/block_io_probe.tmp
rm -f "$block_io_path"
set +e
block_io_output="$(./build/block_io_probe)"
block_io_status=$?
set -e
if [ "$block_io_status" -ne 0 ]; then
    echo "block I/O probe returned $block_io_status" >&2
    rm -f "$block_io_path"
    exit 1
fi
if [ "$block_io_output" != "block-io-ok" ]; then
    echo "unexpected block I/O output: $block_io_output" >&2
    rm -f "$block_io_path"
    exit 1
fi
if [ "$(cat "$block_io_path")" != "ABCDE" ]; then
    echo "unexpected block I/O file contents" >&2
    rm -f "$block_io_path"
    exit 1
fi
rm -f "$block_io_path"

./build/memory_differential
./build/string_differential
./build/strtok_differential
./build/bsearch_differential
./build/atoi_differential
./build/strtol_differential
./build/strtoul_differential
./build/allocator_failure_test
./build/stdio_write_test
./build/stdio_block_test

echo "runtime/termination/buffered-exit, syscall, memory, string, strtok, strerror, ctype, bsearch, atoi, errno, strtol, strtoul, allocator, calloc, realloc, getenv, inherited/owned/block/formatted stdio, positioning, compiler-neutrality, and differential probes passed"
