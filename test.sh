#!/usr/bin/env sh

INTERP=./lama
LAMA_ROOT="$HOME/.opam/lama/.opam-switch/sources/Lama"


run_tests() {
    skip_list=$2

    find "$LAMA_ROOT/regression/$1" -maxdepth 1 -name '*.lama' -printf '%f\n' \
        | sed 's/\.lama$//1' | sort -n | while read -r test ; do
            case "$skip_list" in
            "$test" ) continue ;;
            *" $test" ) continue ;;
            *" $test "* ) continue ;;
            "$test "* ) continue ;;
            esac

            echo Run "$1/$test"

            "$INTERP" "$LAMA_ROOT/regression/$1/$test.lama" <"$LAMA_ROOT/regression/$1/$test.input" \
                | diff - "$LAMA_ROOT/regression/$1/orig/$test.log" || exit 1
        done
}


set -e

# TODO: fix closures - test094, test096
#       add eta - test105
#       add infix declarations - test092, test095, test098
#       add patterns in function declaration - test106

run_tests '' 'test092 test094 test095 test096 test098 test105 test106 test111'
run_tests expressions
run_tests deep-expressions
