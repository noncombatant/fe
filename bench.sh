#!/usr/bin/env bash

set -euo pipefail

build() {
  # A minimal way to get an optimized build that works for every revision:
  clang -o fe -D FE_STANDALONE -O3 ./*.c
}

run_script() {
  local script="$1"
  local n=0
  while [[ $n -lt 10 ]]; do
    /usr/bin/time ./fe "$script" > /dev/null 2>> "$script.times"
    n=$((n + 1))
  done
}

times_to_csv() {
  local times="$1"
  local revision="$2"
  local csv="$3"
  awk -v revision="$revision" '
    {
        real += $1
        user += $3
        sys += $5
    }
    END {
        real /= NR
        user /= NR
        sys /= NR
        printf("%s,%.2f,%.2f,%.2f\n", revision, real, user, sys)
    }' "$times" >> "$csv"
}

for revision in $(git log | awk '/^commit/ { print $2 }'); do
  git checkout "$revision"
  # Unfortunately, some revisions don't build. Arrrgggh
  build || continue
  for script in scripts/{life,macros,mandelbrot}.fe; do
    run_script "$script"
    times="$script.times"
    csv=${times//.times/.csv}
    echo "revision,real,user,sys" > "$csv"
    times_to_csv "$script.times" "$revision" "$csv"
  done
done

git checkout master
