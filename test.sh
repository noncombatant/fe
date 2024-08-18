#!/usr/bin/env bash

failed=0

check_results() {
    local expected_out=${1}
    local expected_err=${2}
    local comment=${3}
    if cmp out "$expected_out" && cmp err "$expected_err"; then
        echo "✅ $comment"
    else
        echo "❌ $comment"
        failed=$(($failed + 1))
    fi
}

run_test() {
    for s in scripts/*; do
        local b=$(basename "$s")
        ./fe "$s" > out 2> err
        check_results "tests/$b.out" "tests/$b.err" "$s"
    done
    ./fe -e '(print "hello, world!")' > out 2> err
    check_results "tests/one-liner.out" "tests/one-liner.err" "one-liner"
    rm out err
}

make clean
make fe
run_test
make clean
RELEASE=1 make fe
run_test

if [[ $failed -eq 0 ]]; then
    echo "✅ all tests passed"
else
    echo "❌ $failed tests failed"
fi

exit "$failed"
