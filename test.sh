#!/usr/bin/env bash

failed=0
for s in scripts/*; do
    b=$(basename "$s")
    ./fe -i "$s" > out 2> err
    if cmp out "tests/$b.out" && cmp err "tests/$b.err"; then
        echo "✅ $s"
    else
        echo "❌ $s"
        failed=$(($failed + 1))
    fi
done

./fe -e '(print "hello, world!")' > out 2> err
if cmp out "tests/one-liner.out" && cmp err "tests/one-liner.err"; then
    echo "✅ one-liner"
else
    echo "❌ one-liner"
    failed=$(($failed + 1))
fi

rm out err

if [[ $failed -eq 0 ]]; then
    echo "✅ all tests passed"
    exit 0
else
    echo "❌ $failed tests failed"
    exit 1
fi
