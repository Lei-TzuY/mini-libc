#!/bin/sh
set -eu

signal_output="$(./build/signal_probe)"
if [ "$signal_output" != "signal-ok" ]; then
    echo "unexpected signal output: $signal_output" >&2
    exit 1
fi

./build/signal_test

echo "signal handler delivery, ignore, thread-directed raise, errno, and raw ABI probes passed"
