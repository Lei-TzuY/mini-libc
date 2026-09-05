#!/bin/sh
set -eu

files="$(find include src -type f \( -name '*.c' -o -name '*.h' \) -print)"
generic_files="$(printf '%s\n' "$files" | grep -v '^include/stdarg\.h$')"

for token in \
    '__SIZE_TYPE__' \
    '__UINTPTR_TYPE__' \
    '__INT_MAX__' \
    '__LONG_MAX__' \
    '__LONG_LONG_MAX__' \
    '__attribute__' \
    '__extension__'
do
    if grep -nH -F "$token" $files; then
        echo "compiler-specific token $token leaked into production C/header surface" >&2
        exit 1
    fi
done

if grep -nH -F '__builtin_' $generic_files; then
    echo "compiler-specific builtin leaked outside the standard varargs header" >&2
    exit 1
fi

stdarg_residue="$(sed \
    -e 's/__builtin_va_list//g' \
    -e 's/__builtin_va_start//g' \
    -e 's/__builtin_va_arg//g' \
    -e 's/__builtin_va_copy//g' \
    -e 's/__builtin_va_end//g' \
    include/stdarg.h)"
if printf '%s\n' "$stdarg_residue" | grep -n -F '__builtin_'; then
    echo "unsupported compiler builtin leaked into include/stdarg.h" >&2
    exit 1
fi

for primitive in \
    '__builtin_va_list' \
    '__builtin_va_start' \
    '__builtin_va_arg' \
    '__builtin_va_copy' \
    '__builtin_va_end'
do
    if ! grep -F "$primitive" include/stdarg.h >/dev/null; then
        echo "missing required stdarg compiler primitive $primitive" >&2
        exit 1
    fi
done

echo "production C/header compiler-neutrality check passed with stdarg-only variadic primitives"
