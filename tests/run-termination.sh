#!/bin/sh
set -eu

check_case() {
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

check_case c11-quick 31 321
check_case quick-capacity 32 XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
check_case quick-normal-isolation 33 A

set +e
abort_output="$(./build/runtime_probe abort-handler 2>/dev/null)"
abort_status=$?
set -e
if [ "$abort_status" -ne 134 ]; then
    echo "abort handler mode returned $abort_status, expected SIGABRT status 134" >&2
    exit 1
fi
if [ "$abort_output" != H ]; then
    echo "abort handler mode output '$abort_output', expected 'H'" >&2
    exit 1
fi

echo "quick/abnormal termination lifecycle passed"
