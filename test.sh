#!/usr/bin/env sh

INTERP=./lama
LAMA_ROOT="$HOME/.opam/lama/.opam-switch/sources/Lama"

TEST_DIR=test.out


run_tests() {
    find "$LAMA_ROOT/regression/$1" -maxdepth 1 -name '*.lama' -printf '%f\n' \
        | sed 's/\.lama$//1' | sort -n | while read test ; do
            echo Run "$1/$test"

            "$INTERP" "$LAMA_ROOT/regression/$1/$test.lama" <"$LAMA_ROOT/regression/$1/$test.input" \
                | diff - "$LAMA_ROOT/regression/$1/orig/$test.log" || exit 1
        done
}


set -e

mkdir -p "$TEST_DIR"

run_tests
run_tests expressions
run_tests deep-expressions
