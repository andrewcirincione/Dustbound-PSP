#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/v090 evidence/v090
python3 -c 'import gzip,pathlib; pathlib.Path("build/old-v8.sav").write_bytes(gzip.decompress(pathlib.Path("tests/fixtures/v081-migration.sav.gz").read_bytes()))'
sources=(src/game.c src/society.c src/civilization.c src/limbs.c src/population.c src/politics.c src/content.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/depth.c src/depth_combat.c)
cc -O1 -g -std=gnu99 -fsanitize=address,undefined -Isrc "${sources[@]}" src/render.c tests/test_society.c -lm -o build/test-society
ASAN_OPTIONS=detect_leaks=0 build/test-society

cc -O1 -g -std=gnu99 -fsanitize=address,undefined -Isrc "${sources[@]}" src/render.c tests/test_render.c -lm -o build/test-render-v090
ASAN_OPTIONS=detect_leaks=0 build/test-render-v090

cc -O1 -g -std=gnu99 -fsanitize=address,undefined -Isrc "${sources[@]}" tests/test_progression.c -lm -o build/test-progression
ASAN_OPTIONS=detect_leaks=0 build/test-progression

cc -O1 -g -std=gnu99 -fsanitize=address,undefined -Isrc "${sources[@]}" tests/test_rescue.c -lm -o build/test-rescue
ASAN_OPTIONS=detect_leaks=0 build/test-rescue
