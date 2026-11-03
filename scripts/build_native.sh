#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
mkdir -p build
cc -O2 -std=gnu99 -Wall -Wextra -Wno-misleading-indentation -Wno-missing-field-initializers -Isrc src/game.c src/content.c src/world.c src/expansion.c src/audio.c src/render.c src/native_main.c -lm -o build/dustbound-native
cc -O1 -g -std=gnu99 -Wall -Wextra -Wno-misleading-indentation -Wno-missing-field-initializers -Isrc src/game.c src/content.c src/world.c src/expansion.c src/audio.c tests/test_game.c -lm -o build/test-game
./build/test-game

cc -O1 -g -std=gnu99 -Wall -Wextra -Wno-misleading-indentation -Wno-missing-field-initializers -Isrc src/game.c src/content.c src/world.c src/expansion.c src/audio.c tests/test_expansion.c -lm -o build/test-expansion
./build/test-expansion
cc -O1 -g -std=gnu99 -Wall -Wextra -Wno-missing-field-initializers -Isrc src/game.c src/content.c src/world.c src/expansion.c src/audio.c src/render.c tests/test_render.c -lm -o build/test-render
./build/test-render
python3 scripts/audit_catalog.py
