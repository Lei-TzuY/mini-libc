#!/bin/sh
set -eu

: "${MINICC:?set MINICC to a tiny-c-compiler minicc executable}"

CC=${CC:-cc}
AR=${AR:-ar}
LD=${LD:-ld}
OUT=${OUT:-build/tiny-c-integration}

rm -rf "$OUT"
mkdir -p "$OUT/obj"

objects=""
for source in $(find src -type f -name '*.c' -print | sort); do
    name=$(printf '%s' "$source" | tr '/.' '__')
    object="$OUT/obj/$name.o"
    "$MINICC" -nostdinc -Iinclude -c "$source" -o "$object"
    objects="$objects $object"
done

"$CC" -fno-pie -c src/syscall/syscall.S -o "$OUT/syscall.o"
"$CC" -fno-pie -c src/stdio/format_entry.S -o "$OUT/format_entry.o"
"$CC" -fno-pie -c src/stdio/scan_entry.S -o "$OUT/scan_entry.o"
"$CC" -fno-pie -c src/control/setjmp.S -o "$OUT/setjmp.o"
"$CC" -fno-pie -c src/thread/thread_entry.S -o "$OUT/thread-entry.o"
"$CC" -fno-pie -c src/fenv/fenv_asm.S -o "$OUT/fenv-asm.o"
"$CC" -fno-pie -c src/math/sqrt.S -o "$OUT/math-sqrt.o"
"$CC" -fno-pie -c src/crt/crt0.S -o "$OUT/crt0.o"
"$AR" rcs "$OUT/libc.a" $objects "$OUT/syscall.o" \
    "$OUT/format_entry.o" "$OUT/scan_entry.o" "$OUT/setjmp.o" \
    "$OUT/thread-entry.o" "$OUT/fenv-asm.o" "$OUT/math-sqrt.o"

"$MINICC" -nostdinc -Iinclude -c tests/tiny_c_integration.c \
    -o "$OUT/integration.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_buffering_integration.c \
    -o "$OUT/buffering.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_pathname_integration.c \
    -o "$OUT/pathname.o"
"$MINICC" -nostdinc -Iinclude -c tests/posix_fd_probe.c \
    -o "$OUT/posix-fd.o"
"$MINICC" -nostdinc -Iinclude -c tests/descriptor_control_probe.c \
    -o "$OUT/descriptor-control.o"
"$MINICC" -nostdinc -Iinclude -c tests/cwd_state_probe.c \
    -o "$OUT/cwd-state.o"
"$MINICC" -nostdinc -Iinclude -c tests/pipe_ipc_probe.c \
    -o "$OUT/pipe-ipc.o"
"$MINICC" -nostdinc -Iinclude -c tests/poll_readiness_probe.c \
    -o "$OUT/poll-readiness.o"
"$MINICC" -nostdinc -Iinclude -c tests/process_orchestration_probe.c \
    -o "$OUT/process-orchestration.o"
"$MINICC" -nostdinc -Iinclude -c tests/exec_child_probe.c \
    -o "$OUT/exec-child.o"
"$MINICC" -nostdinc -Iinclude -c tests/exec_transition_probe.c \
    -o "$OUT/exec-transition.o"
"$MINICC" -nostdinc -Iinclude -c tests/spawn_child_probe.c \
    -o "$OUT/spawn-child.o"
"$MINICC" -nostdinc -Iinclude -c tests/posix_spawn_probe.c \
    -o "$OUT/posix-spawn.o"
"$MINICC" -nostdinc -Iinclude -c tests/process_control_probe.c \
    -o "$OUT/process-control.o"
"$MINICC" -nostdinc -Iinclude -c tests/process_group_probe.c \
    -o "$OUT/process-group.o"
"$MINICC" -nostdinc -Iinclude -c tests/session_hierarchy_probe.c \
    -o "$OUT/session-hierarchy.o"
"$MINICC" -nostdinc -Iinclude -c tests/atfork_probe.c \
    -o "$OUT/atfork.o"
"$MINICC" -nostdinc -Iinclude -c tests/resource_limit_probe.c \
    -o "$OUT/resource-limit.o"
"$MINICC" -nostdinc -Iinclude -c tests/credential_child_probe.c \
    -o "$OUT/credential-child.o"
"$MINICC" -nostdinc -Iinclude -c tests/credential_policy_probe.c \
    -o "$OUT/credential-policy.o"
"$MINICC" -nostdinc -Iinclude -c tests/posix_path_probe.c \
    -o "$OUT/posix-path.o"
"$MINICC" -nostdinc -Iinclude -c tests/metadata_probe.c \
    -o "$OUT/metadata.o"
"$MINICC" -nostdinc -Iinclude -c tests/dirent_probe.c \
    -o "$OUT/dirent.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_rebind_integration.c \
    -o "$OUT/rebind.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_time_integration.c \
    -o "$OUT/time.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_termination_integration.c \
    -o "$OUT/termination.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_setjmp_integration.c \
    -o "$OUT/setjmp-test.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_thread_integration.c \
    -o "$OUT/thread.o"
"$MINICC" -nostdinc -Iinclude -c tests/thread_locale_probe.c \
    -o "$OUT/thread-locale.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_condition_integration.c \
    -o "$OUT/condition.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_mutex_integration.c \
    -o "$OUT/mutex.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_atomic_integration.c \
    -o "$OUT/atomic-test.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_math_integration.c \
    -o "$OUT/math-test.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_locale_state_integration.c \
    -o "$OUT/locale-state.o"
"$MINICC" -nostdinc -Iinclude -c tests/newlocale_probe.c \
    -o "$OUT/newlocale.o"
"$MINICC" -nostdinc -Iinclude -c tests/wctype_locale_probe.c \
    -o "$OUT/wctype-l.o"
"$MINICC" -nostdinc -Iinclude -c tests/ctype_locale_probe.c \
    -o "$OUT/ctype-l.o"

if [ -n "${MINI_ELF_LINKER:-}" ]; then
    "$MINI_ELF_LINKER" link -o "$OUT/integration" \
        "$OUT/integration.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/buffering" \
        "$OUT/buffering.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/pathname" \
        "$OUT/pathname.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/posix-fd" \
        "$OUT/posix-fd.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/descriptor-control" \
        "$OUT/descriptor-control.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/cwd-state" \
        "$OUT/cwd-state.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/pipe-ipc" \
        "$OUT/pipe-ipc.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/poll-readiness" \
        "$OUT/poll-readiness.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/process-orchestration" \
        "$OUT/process-orchestration.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/exec-child" \
        "$OUT/exec-child.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/exec-transition" \
        "$OUT/exec-transition.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/spawn-child" \
        "$OUT/spawn-child.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/posix-spawn" \
        "$OUT/posix-spawn.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/process-control" \
        "$OUT/process-control.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/process-group" \
        "$OUT/process-group.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/session-hierarchy" \
        "$OUT/session-hierarchy.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/atfork" \
        "$OUT/atfork.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/resource-limit" \
        "$OUT/resource-limit.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/credential-child" \
        "$OUT/credential-child.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/credential-policy" \
        "$OUT/credential-policy.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/posix-path" \
        "$OUT/posix-path.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/metadata" \
        "$OUT/metadata.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/dirent" \
        "$OUT/dirent.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/rebind" \
        "$OUT/rebind.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/time" \
        "$OUT/time.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/termination" \
        "$OUT/termination.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/setjmp-test" \
        "$OUT/setjmp-test.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/thread" \
        "$OUT/thread.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/thread-locale" \
        "$OUT/thread-locale.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/condition" \
        "$OUT/condition.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/mutex" \
        "$OUT/mutex.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/atomic-test" \
        "$OUT/atomic-test.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/math-test" \
        "$OUT/math-test.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/locale-state" \
        "$OUT/locale-state.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/newlocale" \
        "$OUT/newlocale.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/wctype-l" \
        "$OUT/wctype-l.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/ctype-l" \
        "$OUT/ctype-l.o" "$OUT/crt0.o" "$OUT/libc.a"
    linker_name="mini-elf-toolchain"
else
    "$LD" -static -e _start --build-id=none -o "$OUT/integration" \
        "$OUT/integration.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/buffering" \
        "$OUT/buffering.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/pathname" \
        "$OUT/pathname.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/posix-fd" \
        "$OUT/posix-fd.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/descriptor-control" \
        "$OUT/descriptor-control.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/cwd-state" \
        "$OUT/cwd-state.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/pipe-ipc" \
        "$OUT/pipe-ipc.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/poll-readiness" \
        "$OUT/poll-readiness.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/process-orchestration" \
        "$OUT/process-orchestration.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/exec-child" \
        "$OUT/exec-child.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/exec-transition" \
        "$OUT/exec-transition.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/spawn-child" \
        "$OUT/spawn-child.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/posix-spawn" \
        "$OUT/posix-spawn.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/process-control" \
        "$OUT/process-control.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/process-group" \
        "$OUT/process-group.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/session-hierarchy" \
        "$OUT/session-hierarchy.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/atfork" \
        "$OUT/atfork.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/resource-limit" \
        "$OUT/resource-limit.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/credential-child" \
        "$OUT/credential-child.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/credential-policy" \
        "$OUT/credential-policy.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/posix-path" \
        "$OUT/posix-path.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/metadata" \
        "$OUT/metadata.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/dirent" \
        "$OUT/dirent.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/rebind" \
        "$OUT/rebind.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/time" \
        "$OUT/time.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/termination" \
        "$OUT/termination.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/setjmp-test" \
        "$OUT/setjmp-test.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/thread" \
        "$OUT/thread.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/thread-locale" \
        "$OUT/thread-locale.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/condition" \
        "$OUT/condition.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/mutex" \
        "$OUT/mutex.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/atomic-test" \
        "$OUT/atomic-test.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/math-test" \
        "$OUT/math-test.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/locale-state" \
        "$OUT/locale-state.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/newlocale" \
        "$OUT/newlocale.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/wctype-l" \
        "$OUT/wctype-l.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/ctype-l" \
        "$OUT/ctype-l.o" "$OUT/crt0.o" "$OUT/libc.a"
    linker_name="GNU ld"
fi

io_path="$OUT/owned-file.tmp"
rm -f "$io_path"
output=$(printf '1 2 3 4 5 6 1.5 -2.5e2' | \
    LANG=C LC_CTYPE=C.UTF-8 MINI_TINY_C=yes MINI_IO_PATH="$io_path" \
    "$OUT/integration" arg)
if [ "$output" != "tiny-c-integration-ok:+00007:0x2a:-5000000000:11:22:33:1.5" ]; then
    echo "unexpected tiny-c-compiler integration output: $output" >&2
    rm -f "$io_path"
    exit 1
fi
if [ "$(cat "$io_path")" != "012345XY89" ]; then
    echo "unexpected tiny-c-compiler positioned file contents" >&2
    rm -f "$io_path"
    exit 1
fi
rm -f "$io_path"

buffering_path="$OUT/buffering-file.tmp"
rm -f "$buffering_path"
buffering_output=$("$OUT/buffering" "$buffering_path")
if [ "$buffering_output" != "tiny-buffering-ok" ]; then
    echo "unexpected tiny-c buffering output: $buffering_output" >&2
    rm -f "$buffering_path"
    exit 1
fi
expected_buffering=$(printf 'abcdeF\nGHI')
if [ "$(cat "$buffering_path")" != "$expected_buffering" ]; then
    echo "unexpected tiny-c buffering file contents" >&2
    rm -f "$buffering_path"
    exit 1
fi
rm -f "$buffering_path"

pathname_source="$OUT/pathname-source.tmp"
pathname_target="$OUT/pathname-target.tmp"
pathname_dir="$OUT/pathname-dir.tmp"
rm -f "$pathname_source" "$pathname_target"
rmdir "$pathname_dir" 2>/dev/null || true
mkdir "$pathname_dir"
pathname_output=$("$OUT/pathname" "$pathname_source" "$pathname_target" "$pathname_dir")
if [ "$pathname_output" != "tiny-pathname-ok" ]; then
    echo "unexpected tiny-c pathname output: $pathname_output" >&2
    rm -f "$pathname_source" "$pathname_target"
    rmdir "$pathname_dir" 2>/dev/null || true
    exit 1
fi
if [ -e "$pathname_source" ] || [ -e "$pathname_target" ] || [ -e "$pathname_dir" ]; then
    echo "tiny-c pathname integration left filesystem state behind" >&2
    rm -f "$pathname_source" "$pathname_target"
    rmdir "$pathname_dir" 2>/dev/null || true
    exit 1
fi

posix_fd_path="$OUT/posix-fd.tmp"
rm -f "$posix_fd_path"
posix_fd_output=$("$OUT/posix-fd" "$posix_fd_path")
if [ "$posix_fd_output" != "posix-fd-ok" ]; then
    echo "unexpected tiny-c POSIX descriptor output: $posix_fd_output" >&2
    rm -f "$posix_fd_path"
    exit 1
fi
if [ -e "$posix_fd_path" ]; then
    echo "tiny-c POSIX descriptor integration left filesystem state behind" >&2
    rm -f "$posix_fd_path"
    exit 1
fi

descriptor_source="$OUT/descriptor-control-source.tmp"
descriptor_target="$OUT/descriptor-control-target.tmp"
rm -f "$descriptor_source" "$descriptor_target"
descriptor_output=$("$OUT/descriptor-control" "$descriptor_source" "$descriptor_target")
if [ "$descriptor_output" != "descriptor-control-ok" ]; then
    echo "unexpected tiny-c descriptor control output: $descriptor_output" >&2
    rm -f "$descriptor_source" "$descriptor_target"
    exit 1
fi
if [ -e "$descriptor_source" ] || [ -e "$descriptor_target" ]; then
    echo "tiny-c descriptor control integration left filesystem state behind" >&2
    rm -f "$descriptor_source" "$descriptor_target"
    exit 1
fi

cwd_root="$OUT/cwd-state-root.tmp"
rm -rf "$cwd_root"
mkdir -p "$cwd_root/child"
cwd_output=$("$OUT/cwd-state" "$cwd_root")
if [ "$cwd_output" != "cwd-state-ok" ]; then
    echo "unexpected tiny-c cwd state output: $cwd_output" >&2
    rm -rf "$cwd_root"
    exit 1
fi
if ! rmdir "$cwd_root/child" "$cwd_root"; then
    echo "tiny-c cwd state integration left filesystem state behind" >&2
    rm -rf "$cwd_root"
    exit 1
fi

pipe_output=$("$OUT/pipe-ipc")
if [ "$pipe_output" != "pipe-ipc-ok" ]; then
    echo "unexpected tiny-c pipe IPC output: $pipe_output" >&2
    exit 1
fi

poll_output=$("$OUT/poll-readiness")
if [ "$poll_output" != "poll-readiness-ok" ]; then
    echo "unexpected tiny-c poll readiness output: $poll_output" >&2
    exit 1
fi

process_output=$("$OUT/process-orchestration")
if [ "$process_output" != "process-orchestration-ok" ]; then
    echo "unexpected tiny-c process orchestration output: $process_output" >&2
    exit 1
fi

exec_output=$("$OUT/exec-transition" "$OUT/exec-child")
if [ "$exec_output" != "exec-transition-ok" ]; then
    echo "unexpected tiny-c exec transition output: $exec_output" >&2
    exit 1
fi

spawn_output=$("$OUT/posix-spawn" "$OUT/spawn-child")
if [ "$spawn_output" != "posix-spawn-ok" ]; then
    echo "unexpected tiny-c posix spawn output: $spawn_output" >&2
    exit 1
fi

control_output=$("$OUT/process-control")
if [ "$control_output" != "process-control-ok" ]; then
    echo "unexpected tiny-c process control output: $control_output" >&2
    exit 1
fi

group_output=$("$OUT/process-group")
if [ "$group_output" != "process-group-ok" ]; then
    echo "unexpected tiny-c process group output: $group_output" >&2
    exit 1
fi

session_output=$("$OUT/session-hierarchy")
if [ "$session_output" != "session-hierarchy-ok" ]; then
    echo "unexpected tiny-c session hierarchy output: $session_output" >&2
    exit 1
fi

atfork_output=$("$OUT/atfork")
if [ "$atfork_output" != "atfork-coordination-ok" ]; then
    echo "unexpected tiny-c atfork coordination output: $atfork_output" >&2
    exit 1
fi

resource_path="$OUT/resource-limit.tmp"
rm -f "$resource_path"
resource_output=$("$OUT/resource-limit" "$resource_path")
if [ "$resource_output" != "resource-limit-ok" ]; then
    echo "unexpected tiny-c resource limit output: $resource_output" >&2
    rm -f "$resource_path"
    exit 1
fi
if [ -e "$resource_path" ]; then
    echo "tiny-c resource limit integration left filesystem state behind" >&2
    rm -f "$resource_path"
    exit 1
fi

credential_parent="$OUT/credential-parent.tmp"
credential_child_file="$OUT/credential-child.tmp"
rm -f "$credential_parent" "$credential_child_file"
credential_output=$("$OUT/credential-policy" "$OUT/credential-child" "$credential_parent" "$credential_child_file")
if [ "$credential_output" != "credential-policy-ok" ]; then
    echo "unexpected tiny-c credential policy output: $credential_output" >&2
    rm -f "$credential_parent" "$credential_child_file"
    exit 1
fi
if [ -e "$credential_parent" ] || [ -e "$credential_child_file" ]; then
    echo "tiny-c credential policy integration left filesystem state behind" >&2
    rm -f "$credential_parent" "$credential_child_file"
    exit 1
fi

posix_path_source="$OUT/posix-path-source.tmp"
posix_path_target="$OUT/posix-path-target.tmp"
posix_path_dir="$OUT/posix-path-dir.tmp"
posix_remove_dir="$OUT/posix-remove-dir.tmp"
rm -f "$posix_path_source" "$posix_path_target"
rmdir "$posix_path_dir" "$posix_remove_dir" 2>/dev/null || true
mkdir "$posix_path_dir" "$posix_remove_dir"
posix_path_output=$("$OUT/posix-path" "$posix_path_source"     "$posix_path_target" "$posix_path_dir" "$posix_remove_dir")
if [ "$posix_path_output" != "posix-path-ok" ]; then
    echo "unexpected tiny-c POSIX pathname output: $posix_path_output" >&2
    rm -f "$posix_path_source" "$posix_path_target"         "$posix_path_dir/source" "$posix_path_dir/target"
    rmdir "$posix_path_dir" "$posix_remove_dir" 2>/dev/null || true
    exit 1
fi
if [ -e "$posix_path_source" ] || [ -e "$posix_path_target" ] ||
   [ -e "$posix_path_dir" ] || [ -e "$posix_remove_dir" ]; then
    echo "tiny-c POSIX pathname integration left filesystem state behind" >&2
    rm -f "$posix_path_source" "$posix_path_target"         "$posix_path_dir/source" "$posix_path_dir/target"
    rmdir "$posix_path_dir" "$posix_remove_dir" 2>/dev/null || true
    exit 1
fi

metadata_path="$OUT/metadata.tmp"
rm -f "$metadata_path"
metadata_output=$("$OUT/metadata" "$metadata_path" "$OUT")
if [ "$metadata_output" != "metadata-ok" ]; then
    echo "unexpected tiny-c metadata output: $metadata_output" >&2
    rm -f "$metadata_path"
    exit 1
fi
if [ -e "$metadata_path" ]; then
    echo "tiny-c metadata integration left filesystem state behind" >&2
    rm -f "$metadata_path"
    exit 1
fi

dirent_root="$OUT/dirent-root.tmp"
rm -rf "$dirent_root"
mkdir -p "$dirent_root/subdir"
dirent_i=0
while [ "$dirent_i" -lt 180 ]; do
    : > "$dirent_root/filler-$dirent_i"
    dirent_i=$((dirent_i + 1))
done
dirent_output=$("$OUT/dirent" "$dirent_root")
if [ "$dirent_output" != "dirent-ok" ]; then
    echo "unexpected tiny-c dirent output: $dirent_output" >&2
    rm -rf "$dirent_root"
    exit 1
fi
rm -f "$dirent_root"/filler-*
if ! rmdir "$dirent_root"; then
    echo "tiny-c dirent integration left filesystem state behind" >&2
    rm -rf "$dirent_root"
    exit 1
fi

rebind_old="$OUT/rebind-old.tmp"
rebind_new="$OUT/rebind-new.tmp"
rm -f "$rebind_old" "$rebind_new"
rebind_output=$("$OUT/rebind" "$rebind_old" "$rebind_new")
if [ "$rebind_output" != "tiny-rebind-ok" ]; then
    echo "unexpected tiny-c rebind output: $rebind_output" >&2
    rm -f "$rebind_old" "$rebind_new"
    exit 1
fi
if [ "$(cat "$rebind_old")" != "OLD" ] || [ "$(cat "$rebind_new")" != "NEW" ]; then
    echo "unexpected tiny-c rebind file contents" >&2
    rm -f "$rebind_old" "$rebind_new"
    exit 1
fi
rm -f "$rebind_old" "$rebind_new"

time_output=$("$OUT/time")
if [ "$time_output" != "tiny-time-ok" ]; then
    echo "unexpected tiny-c time output: $time_output" >&2
    exit 1
fi

setjmp_output=$("$OUT/setjmp-test")
if [ "$setjmp_output" != "tiny-setjmp-ok" ]; then
    echo "unexpected tiny-c setjmp output: $setjmp_output" >&2
    exit 1
fi

thread_output=$("$OUT/thread")
if [ "$thread_output" != "tiny-threads-ok" ]; then
    echo "unexpected tiny-c thread output: $thread_output" >&2
    exit 1
fi

thread_locale_output=$("$OUT/thread-locale")
if [ "$thread_locale_output" != "thread-locale-ok" ]; then
    echo "unexpected tiny-c thread locale output: $thread_locale_output" >&2
    exit 1
fi

condition_output=$("$OUT/condition")
if [ "$condition_output" != "tiny-conditions-ok" ]; then
    echo "unexpected tiny-c condition output: $condition_output" >&2
    exit 1
fi

mutex_output=$("$OUT/mutex")
if [ "$mutex_output" != "tiny-mutex-ok" ]; then
    echo "unexpected tiny-c mutex output: $mutex_output" >&2
    exit 1
fi

atomic_output=$("$OUT/atomic-test")
if [ "$atomic_output" != "tiny-atomics-ok" ]; then
    echo "unexpected tiny-c atomics output: $atomic_output" >&2
    exit 1
fi

math_output=$("$OUT/math-test")
if [ "$math_output" != "tiny-math-ok" ]; then
    echo "unexpected tiny-c math output: $math_output" >&2
    exit 1
fi

locale_state_output=$("$OUT/locale-state")
if [ "$locale_state_output" != "tiny-locale-state-ok" ]; then
    echo "unexpected tiny-c locale state output: $locale_state_output" >&2
    exit 1
fi

newlocale_output=$("$OUT/newlocale")
if [ "$newlocale_output" != "newlocale-ok" ]; then
    echo "unexpected tiny-c newlocale output: $newlocale_output" >&2
    exit 1
fi

newlocale_env_output=$(env -i LANG=C LC_CTYPE=C.UTF-8 \
    "$OUT/newlocale" env C.UTF-8)
if [ "$newlocale_env_output" != "newlocale-env-ok" ]; then
    echo "unexpected tiny-c newlocale environment output: $newlocale_env_output" >&2
    exit 1
fi

wctype_l_output=$("$OUT/wctype-l")
if [ "$wctype_l_output" != "wctype-l-ok" ]; then
    echo "unexpected tiny-c explicit-locale wctype output: $wctype_l_output" >&2
    exit 1
fi

ctype_l_output=$("$OUT/ctype-l")
if [ "$ctype_l_output" != "ctype-l-ok" ]; then
    echo "unexpected tiny-c explicit-locale ctype output: $ctype_l_output" >&2
    exit 1
fi

set +e
termination_registry_output=$(timeout 5s "$OUT/termination" registry)
termination_registry_status=$?
set -e
if [ "$termination_registry_status" -ne 0 ] || [ "$termination_registry_output" != R ]; then
    echo "unexpected tiny-c termination registry: status=$termination_registry_status output='$termination_registry_output'" >&2
    exit 1
fi

set +e
termination_last_thread_output=$(timeout 5s "$OUT/termination" last-thread)
termination_last_thread_status=$?
set -e
if [ "$termination_last_thread_status" -ne 0 ] || [ "$termination_last_thread_output" != BH ]; then
    echo "unexpected tiny-c last-thread termination: status=$termination_last_thread_status output='$termination_last_thread_output'" >&2
    exit 1
fi

set +e
termination_quick_output=$("$OUT/termination" quick)
termination_quick_status=$?
set -e
if [ "$termination_quick_status" -ne 41 ] || [ "$termination_quick_output" != 21 ]; then
    echo "unexpected tiny-c quick termination: status=$termination_quick_status output='$termination_quick_output'" >&2
    exit 1
fi

set +e
termination_abort_output=$("$OUT/termination" abort 2>/dev/null)
termination_abort_status=$?
set -e
if [ "$termination_abort_status" -ne 134 ] || [ "$termination_abort_output" != H ]; then
    echo "unexpected tiny-c abort termination: status=$termination_abort_status output='$termination_abort_output'" >&2
    exit 1
fi

./tests/verify-no-host-libc.sh "$OUT/integration"
./tests/verify-no-host-libc.sh "$OUT/buffering"
./tests/verify-no-host-libc.sh "$OUT/pathname"
./tests/verify-no-host-libc.sh "$OUT/posix-fd"
./tests/verify-no-host-libc.sh "$OUT/descriptor-control"
./tests/verify-no-host-libc.sh "$OUT/cwd-state"
./tests/verify-no-host-libc.sh "$OUT/pipe-ipc"
./tests/verify-no-host-libc.sh "$OUT/poll-readiness"
./tests/verify-no-host-libc.sh "$OUT/process-orchestration"
./tests/verify-no-host-libc.sh "$OUT/exec-child"
./tests/verify-no-host-libc.sh "$OUT/exec-transition"
./tests/verify-no-host-libc.sh "$OUT/spawn-child"
./tests/verify-no-host-libc.sh "$OUT/posix-spawn"
./tests/verify-no-host-libc.sh "$OUT/process-control"
./tests/verify-no-host-libc.sh "$OUT/process-group"
./tests/verify-no-host-libc.sh "$OUT/session-hierarchy"
./tests/verify-no-host-libc.sh "$OUT/atfork"
./tests/verify-no-host-libc.sh "$OUT/resource-limit"
./tests/verify-no-host-libc.sh "$OUT/credential-child"
./tests/verify-no-host-libc.sh "$OUT/credential-policy"
./tests/verify-no-host-libc.sh "$OUT/posix-path"
./tests/verify-no-host-libc.sh "$OUT/metadata"
./tests/verify-no-host-libc.sh "$OUT/dirent"
./tests/verify-no-host-libc.sh "$OUT/rebind"
./tests/verify-no-host-libc.sh "$OUT/time"
./tests/verify-no-host-libc.sh "$OUT/termination"
./tests/verify-no-host-libc.sh "$OUT/setjmp-test"
./tests/verify-no-host-libc.sh "$OUT/thread"
./tests/verify-no-host-libc.sh "$OUT/thread-locale"
./tests/verify-no-host-libc.sh "$OUT/condition"
./tests/verify-no-host-libc.sh "$OUT/mutex"
./tests/verify-no-host-libc.sh "$OUT/atomic-test"
./tests/verify-no-host-libc.sh "$OUT/math-test"
./tests/verify-no-host-libc.sh "$OUT/locale-state"
./tests/verify-no-host-libc.sh "$OUT/newlocale"
./tests/verify-no-host-libc.sh "$OUT/wctype-l"
./tests/verify-no-host-libc.sh "$OUT/ctype-l"

echo "tiny-c-compiler -> mini-libc -> $linker_name integration passed"
