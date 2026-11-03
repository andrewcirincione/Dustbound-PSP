#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
mkdir -p build
cc -O2 -std=gnu99 -Wall -Wextra -Wno-misleading-indentation -Isrc src/game.c src/render.c src/native_main.c -lm -o build/dustbound-native
cc -O1 -g -std=gnu99 -Wall -Wextra -Wno-misleading-indentation -Isrc src/game.c tests/test_game.c -lm -o build/test-game
./build/test-game
