#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/v100
python3 -c 'import gzip,pathlib; pathlib.Path("build/old-v9.sav").write_bytes(gzip.decompress(pathlib.Path("tests/fixtures/v090-migration.sav.gz").read_bytes()))'
sources=(src/game.c src/society.c src/civilization.c src/limbs.c src/population.c src/content.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/depth.c src/depth_combat.c)
cc -O1 -g -std=gnu99 -fsanitize=address,undefined -Isrc "${sources[@]}" tests/test_civilization.c -lm -o build/test-civilization
ASAN_OPTIONS=detect_leaks=0 build/test-civilization
cc -O1 -g -std=gnu99 -fsanitize=address,undefined -Isrc "${sources[@]}" tests/test_weapon_variants.c -lm -o build/test-weapon-variants
ASAN_OPTIONS=detect_leaks=0 build/test-weapon-variants
for suite in weapon_art attachments crafting; do
  cc -O2 -std=gnu99 -Isrc "${sources[@]}" "tests/test_${suite}.c" -lm -o "build/test-${suite}-v100"
  "build/test-${suite}-v100"
done
