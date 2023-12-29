#!/usr/bin/env sh


INTERP=./lab_vm
LAMA_ROOT="/opt/opam/lama/.opam-switch/sources/Lama"
LAMAC=lamac

TEST_DIR=test.out


run_tests() {
    find "$LAMA_ROOT/regression/$1" -maxdepth 1 -name '*.lama' -printf '%f\n' \
        | sed 's/\.lama$//1' | sort -n | while read test ; do
            echo Run "$1/$test"

            cd "$TEST_DIR"
            "$LAMAC" -b "$LAMA_ROOT/regression/$1/$test.lama"

            cd ..
            "$INTERP" "$TEST_DIR/$test.bc" <"$LAMA_ROOT/regression/$1/$test.input" \
                | diff - "$LAMA_ROOT/regression/$1/orig/$test.log" || break
        done
}


mkdir -p "$TEST_DIR"

run_tests
run_tests expressions
run_tests deep-expressions
