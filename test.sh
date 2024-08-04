#!/usr/bin/env bash

for s in scripts/*; do
    b=$(basename "$s")
    ./fe "$s" > out 2> err
    if ! cmp out "tests/$b.out" || ! cmp err "tests/$b.err"; then
        echo "$s failed" > /dev/stderr
        exit 1
    fi
done

rm out err
