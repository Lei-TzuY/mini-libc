#!/bin/sh
set -eu

files="$(find include src -type f \( -name '*.c' -o -name '*.h' \) -print)"

for token in \
    '__SIZE_TYPE__' \
    '__UINTPTR_TYPE__' \
    '__INT_MAX__' \
    '__LONG_MAX__' \
    '__LONG_LONG_MAX__' \
    '__attribute__' \
    '__builtin_' \
    '__extension__'
do
    if grep -nH -F "$token" $files; then
        echo "compiler-specific token $token leaked into production C/header surface" >&2
        exit 1
    fi
done

echo "production C/header compiler-neutrality check passed"
