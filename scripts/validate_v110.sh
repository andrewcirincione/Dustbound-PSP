#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/v110
python3 -c 'import gzip,pathlib; pathlib.Path("build/old-v10.sav").write_bytes(gzip.decompress(pathlib.Path("tests/fixtures/v100-migration.sav.gz").read_bytes()))'
sources=(src/game.c src/society.c src/civilization.c src/limbs.c src/population.c src/politics.c src/content.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/depth.c src/depth_combat.c)
cc -O1 -g -std=gnu99 -fsanitize=address,undefined -Isrc "${sources[@]}" tests/test_limbs.c -lm -o build/test-limbs
ASAN_OPTIONS=detect_leaks=0 build/test-limbs
