#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/v170
for pair in v041-outpost:4 v050-expedition:5 v090-migration:9 v110-migration:11 v120-migration:12 v130-migration:13 v140-migration:14; do
  gzip -dc "tests/fixtures/${pair%:*}.sav.gz" > "build/v170/old-v${pair#*:}.sav"
done
sources=(src/game.c src/society.c src/civilization.c src/limbs.c src/population.c src/politics.c
         src/content.c src/armor.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c
         src/depth.c src/depth_combat.c)
cc -O1 -g -std=gnu99 -fsanitize=address,undefined -Isrc "${sources[@]}" tests/test_war.c -lm -o build/v170/test-war
ASAN_OPTIONS=detect_leaks=0 build/v170/test-war "$@"
