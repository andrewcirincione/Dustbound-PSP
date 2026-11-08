#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
mkdir -p build evidence/v081
cc -O1 -g -std=gnu99 -fsanitize=address,undefined -Isrc src/game.c src/society.c src/civilization.c src/limbs.c src/content.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/depth.c src/depth_combat.c src/render.c tests/test_atlas.c -lm -o build/test-atlas
ASAN_OPTIONS=detect_leaks=0 build/test-atlas
