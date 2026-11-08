#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
mkdir -p build evidence/v080
sources=(src/game.c src/society.c src/civilization.c src/content.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/depth.c src/depth_combat.c)
cc -O1 -g -std=gnu99 -fsanitize=address,undefined -Isrc "${sources[@]}" src/render.c tests/test_living.c -lm -o build/test-living
ASAN_OPTIONS=detect_leaks=0 build/test-living
for suite in game expansion; do
  cc -O2 -std=gnu99 -Isrc "${sources[@]}" "tests/test_${suite}.c" -lm -o "build/test-${suite}"
  "build/test-${suite}"
done
