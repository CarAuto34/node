#!/bin/sh
# Runs every native benchmark binary built in benchmark/native/build.
set -e
cd "$(dirname "$0")/build"
for bench in bench_*; do
  if [ -f "$bench" ] && [ -x "$bench" ]; then
    "./$bench"
  fi
done
