#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
output_dir="${1:-build/v160}"
mkdir -p build "$output_dir"
sources=(src/game.c src/society.c src/civilization.c src/limbs.c src/population.c src/politics.c src/content.c src/armor.c src/arsenal.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/depth.c src/depth_combat.c)
cc -O1 -g -std=gnu99 -fsanitize=address,undefined -Isrc "${sources[@]}" tests/test_animation.c -lm -o build/test-animation
ASAN_OPTIONS=detect_leaks=0 build/test-animation "$output_dir"
