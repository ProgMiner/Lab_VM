#!/usr/bin/env sh


INTERP=./lab_vm
LAMA_ROOT="/opt/opam/lama/.opam-switch/sources/Lama"
LAMAC=lamac

TEST_DIR=test.out


mkdir -p "$TEST_DIR"

find "$LAMA_ROOT/regression" -maxdepth 1 -name '*.lama' -printf '%f\n' \
    | sed 's/\.lama$//1' | sort -n | while read test ; do
        echo Run $test

        cd "$TEST_DIR"
        "$LAMAC" -b "$LAMA_ROOT/regression/$test.lama"

        cd ..
        "$INTERP" "$TEST_DIR/$test.bc" <"$LAMA_ROOT/regression/$test.input" \
            | diff - "$LAMA_ROOT/regression/orig/$test.log" || break
    done
