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

posix_fd_path=build/posix-fd-probe.tmp
rm -f "$posix_fd_path"
posix_fd_output="$(./build/posix_fd_probe "$posix_fd_path")"
if [ "$posix_fd_output" != "posix-fd-ok" ]; then
    echo "unexpected POSIX descriptor output: $posix_fd_output" >&2
    rm -f "$posix_fd_path"
    exit 1
fi
if [ -e "$posix_fd_path" ]; then
    echo "POSIX descriptor probe left filesystem state behind" >&2
    rm -f "$posix_fd_path"
    exit 1
fi

posix_path_source=build/posix-path-source.tmp
posix_path_target=build/posix-path-target.tmp
posix_path_dir=build/posix-path-dir.tmp
posix_remove_dir=build/posix-remove-dir.tmp
rm -f "$posix_path_source" "$posix_path_target"
rmdir "$posix_path_dir" "$posix_remove_dir" 2>/dev/null || true
mkdir "$posix_path_dir" "$posix_remove_dir"
posix_path_output="$(./build/posix_path_probe "$posix_path_source"     "$posix_path_target" "$posix_path_dir" "$posix_remove_dir")"
if [ "$posix_path_output" != "posix-path-ok" ]; then
    echo "unexpected POSIX pathname output: $posix_path_output" >&2
    rm -f "$posix_path_source" "$posix_path_target"         "$posix_path_dir/source" "$posix_path_dir/target"
    rmdir "$posix_path_dir" "$posix_remove_dir" 2>/dev/null || true
    exit 1
fi
if [ -e "$posix_path_source" ] || [ -e "$posix_path_target" ] ||
   [ -e "$posix_path_dir" ] || [ -e "$posix_remove_dir" ]; then
    echo "POSIX pathname probe left filesystem state behind" >&2
    rm -f "$posix_path_source" "$posix_path_target"         "$posix_path_dir/source" "$posix_path_dir/target"
    rmdir "$posix_path_dir" "$posix_remove_dir" 2>/dev/null || true
    exit 1
fi

descriptor_source=build/descriptor-control-source.tmp
descriptor_target=build/descriptor-control-target.tmp
rm -f "$descriptor_source" "$descriptor_target"
descriptor_output="$(./build/descriptor_control_probe "$descriptor_source" "$descriptor_target")"
if [ "$descriptor_output" != "descriptor-control-ok" ]; then
    echo "unexpected descriptor control output: $descriptor_output" >&2
    rm -f "$descriptor_source" "$descriptor_target"
    exit 1
fi
if [ -e "$descriptor_source" ] || [ -e "$descriptor_target" ]; then
    echo "descriptor control probe left filesystem state behind" >&2
    rm -f "$descriptor_source" "$descriptor_target"
    exit 1
fi

cwd_root=build/cwd-state-root.tmp
rm -rf "$cwd_root"
mkdir -p "$cwd_root/child"
cwd_output="$(./build/cwd_state_probe "$cwd_root")"
if [ "$cwd_output" != "cwd-state-ok" ]; then
    echo "unexpected cwd state output: $cwd_output" >&2
    rm -rf "$cwd_root"
    exit 1
fi
if ! rmdir "$cwd_root/child" "$cwd_root"; then
    echo "cwd state probe left filesystem state behind" >&2
    rm -rf "$cwd_root"
    exit 1
fi

pipe_output="$(./build/pipe_ipc_probe)"
if [ "$pipe_output" != "pipe-ipc-ok" ]; then
    echo "unexpected pipe IPC output: $pipe_output" >&2
    exit 1
fi

poll_output="$(./build/poll_readiness_probe)"
if [ "$poll_output" != "poll-readiness-ok" ]; then
    echo "unexpected poll readiness output: $poll_output" >&2
    exit 1
fi

process_output="$(./build/process_orchestration_probe)"
if [ "$process_output" != "process-orchestration-ok" ]; then
    echo "unexpected process orchestration output: $process_output" >&2
    exit 1
fi

exec_output="$(./build/exec_transition_probe ./build/exec_child_probe)"
if [ "$exec_output" != "exec-transition-ok" ]; then
    echo "unexpected exec transition output: $exec_output" >&2
    exit 1
fi

spawn_output="$(./build/posix_spawn_probe ./build/spawn_child_probe)"
if [ "$spawn_output" != "posix-spawn-ok" ]; then
    echo "unexpected posix spawn output: $spawn_output" >&2
    exit 1
fi

control_output="$(./build/process_control_probe)"
if [ "$control_output" != "process-control-ok" ]; then
    echo "unexpected process control output: $control_output" >&2
    exit 1
fi

group_output="$(./build/process_group_probe)"
if [ "$group_output" != "process-group-ok" ]; then
    echo "unexpected process group output: $group_output" >&2
    exit 1
fi

session_output="$(./build/session_hierarchy_probe)"
if [ "$session_output" != "session-hierarchy-ok" ]; then
    echo "unexpected session hierarchy output: $session_output" >&2
    exit 1
fi

atfork_output="$(./build/atfork_probe)"
if [ "$atfork_output" != "atfork-coordination-ok" ]; then
    echo "unexpected atfork coordination output: $atfork_output" >&2
    exit 1
fi

resource_path=build/resource-limit.tmp
rm -f "$resource_path"
resource_output="$(./build/resource_limit_probe "$resource_path")"
if [ "$resource_output" != "resource-limit-ok" ]; then
    echo "unexpected resource limit output: $resource_output" >&2
    rm -f "$resource_path"
    exit 1
fi
if [ -e "$resource_path" ]; then
    echo "resource limit probe left filesystem state behind" >&2
    rm -f "$resource_path"
    exit 1
fi

credential_parent=build/credential-parent.tmp
credential_child=build/credential-child.tmp
rm -f "$credential_parent" "$credential_child"
credential_output="$(./build/credential_policy_probe ./build/credential_child_probe "$credential_parent" "$credential_child")"
if [ "$credential_output" != "credential-policy-ok" ]; then
    echo "unexpected credential policy output: $credential_output" >&2
    rm -f "$credential_parent" "$credential_child"
    exit 1
fi
if [ -e "$credential_parent" ] || [ -e "$credential_child" ]; then
    echo "credential policy probe left filesystem state behind" >&2
    rm -f "$credential_parent" "$credential_child"
    exit 1
fi

pty_output="$(./build/pty_job_control_probe)"
if [ "$pty_output" != "pty-job-control-ok" ]; then
    echo "unexpected PTY job-control output: $pty_output" >&2
    exit 1
fi

termios_output="$(./build/termios_canonical_probe)"
if [ "$termios_output" != "termios-canonical-ok" ]; then
    echo "unexpected termios canonical output: $termios_output" >&2
    exit 1
fi

tty_signal_output="$(./build/tty_control_signal_probe)"
if [ "$tty_signal_output" != "tty-control-signals-ok" ]; then
    echo "unexpected tty control signal output: $tty_signal_output" >&2
    exit 1
fi

metadata_path=build/metadata-probe.tmp
rm -f "$metadata_path"
metadata_output="$(./build/metadata_probe "$metadata_path" build)"
if [ "$metadata_output" != "metadata-ok" ]; then
    echo "unexpected metadata probe output: $metadata_output" >&2
    rm -f "$metadata_path"
    exit 1
fi
if [ -e "$metadata_path" ]; then
    echo "metadata probe left filesystem state behind" >&2
    rm -f "$metadata_path"
    exit 1
fi

dirent_root=build/dirent-root.tmp
rm -rf "$dirent_root"
mkdir -p "$dirent_root/subdir"
dirent_i=0
while [ "$dirent_i" -lt 180 ]; do
    : > "$dirent_root/filler-$dirent_i"
    dirent_i=$((dirent_i + 1))
done
dirent_output="$(./build/dirent_probe "$dirent_root")"
if [ "$dirent_output" != "dirent-ok" ]; then
    echo "unexpected dirent probe output: $dirent_output" >&2
    rm -rf "$dirent_root"
    exit 1
fi
rm -f "$dirent_root"/filler-*
if ! rmdir "$dirent_root"; then
    echo "dirent probe left filesystem state behind" >&2
    rm -rf "$dirent_root"
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
    echo "unexpected strtok output: $strtok_output" >&2
    exit 1
fi

strerror_output="$(./build/strerror_probe)"
if [ "$strerror_output" != "strerror-ok" ]; then
    echo "unexpected strerror output: $strerror_output" >&2
    exit 1
fi

ctype_output="$(./build/ctype_probe)"
if [ "$ctype_output" != "ctype-ok" ]; then
    echo "unexpected ctype output: $ctype_output" >&2
    exit 1
fi

bsearch_output="$(./build/bsearch_probe)"
if [ "$bsearch_output" != "bsearch-ok" ]; then
    echo "unexpected bsearch output: $bsearch_output" >&2
    exit 1
fi

atoi_output="$(./build/atoi_probe)"
if [ "$atoi_output" != "atoi-ok" ]; then
    echo "unexpected atoi output: $atoi_output" >&2
    exit 1
fi

errno_output="$(./build/errno_probe)"
if [ "$errno_output" != "errno-ok" ]; then
    echo "unexpected errno output: $errno_output" >&2
    exit 1
fi

strtol_output="$(./build/strtol_probe)"
if [ "$strtol_output" != "strtol-ok" ]; then
    echo "unexpected strtol output: $strtol_output" >&2
    exit 1
fi

strtoul_output="$(./build/strtoul_probe)"
if [ "$strtoul_output" != "strtoul-ok" ]; then
    echo "unexpected strtoul output: $strtoul_output" >&2
    exit 1
fi

strtod_output="$(./build/strtod_probe)"
if [ "$strtod_output" != "strtod-ok" ]; then
    echo "unexpected strtod output: $strtod_output" >&2
    exit 1
fi

time_output="$(./build/time_probe)"
if [ "$time_output" != "time-ok" ]; then
    echo "unexpected time output: $time_output" >&2
    exit 1
fi

allocator_output="$(./build/allocator_probe)"
if [ "$allocator_output" != "allocator-ok" ]; then
    echo "unexpected allocator output: $allocator_output" >&2
    exit 1
fi

calloc_output="$(./build/calloc_probe)"
if [ "$calloc_output" != "calloc-ok" ]; then
    echo "unexpected calloc output: $calloc_output" >&2
    exit 1
fi

realloc_output="$(./build/realloc_probe)"
if [ "$realloc_output" != "realloc-ok" ]; then
    echo "unexpected realloc output: $realloc_output" >&2
    exit 1
fi

getenv_output="$(env -i MINI_GETENV_ALPHA=value MINI_GETENV_EMPTY= \
    MINI_GETENV_ALPHA_SUFFIX=suffix ./build/getenv_probe)"
if [ "$getenv_output" != "getenv-ok" ]; then
    echo "unexpected getenv output: $getenv_output" >&2
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
min:-2147483648:-9223372036854775808
stdio-ok'
if [ "$stdio_output" != "$expected_stdio_output" ]; then
    echo "unexpected stdio stdout:" >&2
    printf '%s\n' "$stdio_output" >&2
    exit 1
fi
expected_stdio_stderr='stderr-ok
format-err:0x00002a:Q   
fprintf-stack:1:2:3:4:5'
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
if [ "$(cat "$file_stream_path")" != "OLD" ]; then
    echo "unexpected owned file contents after stream rebinding" >&2
    rm -f "$file_stream_path"
    exit 1
fi
rm -f "$file_stream_path"

block_io_path=build/block_io_probe.tmp
input_buffer_path=build/input_buffer_probe.tmp
rm -f "$block_io_path" "$input_buffer_path"
set +e
block_io_output="$(./build/block_io_probe)"
block_io_status=$?
set -e
if [ "$block_io_status" -ne 0 ]; then
    echo "block I/O probe returned $block_io_status" >&2
    rm -f "$block_io_path" "$input_buffer_path"
    exit 1
fi
if [ "$block_io_output" != "block-io-ok" ]; then
    echo "unexpected block I/O output: $block_io_output" >&2
    rm -f "$block_io_path" "$input_buffer_path"
    exit 1
fi
if [ "$(cat "$block_io_path")" != "ABCDE" ]; then
    echo "unexpected block I/O file contents" >&2
    rm -f "$block_io_path" "$input_buffer_path"
    exit 1
fi
rm -f "$block_io_path" "$input_buffer_path"

scan_path=build/scan_probe.tmp
rm -f "$scan_path"
set +e
scan_output="$(printf '41 token Q 1.5 -2.5e2' | ./build/scan_probe)"
scan_status=$?
set -e
if [ "$scan_status" -ne 0 ]; then
    echo "formatted input probe returned $scan_status" >&2
    rm -f "$scan_path"
    exit 1
fi
if [ "$scan_output" != "scan-ok" ]; then
    echo "unexpected formatted input output: $scan_output" >&2
    rm -f "$scan_path"
    exit 1
fi
rm -f "$scan_path"

sh ./tests/verify-buffering.sh

host_cc=${CC:-cc}
"$host_cc" -Iinclude -std=c11 -O2 -Wall -Wextra -Werror -pedantic \
    -fno-builtin -fno-pie -c tests/freopen_test.c -o build/freopen_test.o
"$host_cc" -no-pie -o build/freopen_test build/freopen_test.o \
    build/stdio_test_impl.o build/file_stream_test_impl.o build/errno.o \
    build/locale_test_impl.o build/orientation.o build/byte_sync.o build/file_sync.o \
    build/position_test_impl.o build/stdio_write_lseek_fake.o \
    build/stdio_lock_fake.o
./build/freopen_test

./build/memory_differential
./build/string_differential
./build/strtok_differential
./build/bsearch_differential
./build/atoi_differential
./build/strtol_differential
./build/strtoul_differential
./build/strtod_differential
./build/allocator_failure_test
./build/stdio_write_test
./build/stdio_block_test
./build/stdio_scan_test
./build/time_test

echo "runtime/termination/buffered-exit, syscall, memory, string, strtok, strerror, ctype, bsearch, atoi, errno, strtol, strtoul, strtof/strtod, time, allocator, calloc, realloc, getenv, inherited/owned/rebound/block/formatted stdio, configurable buffering, buffered input/pushback, formatted input, positioning, compiler-neutrality, and differential probes passed"
