#!/usr/bin/env bash
# PSPDEV and its bin directory must be configured. Pass PPSSPPHeadless as argument 1.
set -eu
cd "$(dirname "$0")/.."
emu=$(realpath "$1")
mkdir -p build/emu build/frontier-fixtures
bash scripts/build_native.sh > build/v050-native.txt 2>&1
make clean > build/v050-psp.txt 2>&1
make -j2 >> build/v050-psp.txt 2>&1
python3 scripts/validate_pbp.py EBOOT.PBP > build/v050-pbp-validation.txt
build/dustbound-native --fixtures build/emu
cc -O2 -std=gnu99 -Isrc src/game.c src/content.c src/armor.c src/arsenal.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c tests/make_frontier_fixture.c -lm -o build/make-frontier-fixture
build/make-frontier-fixture build/frontier-fixtures
python3 scripts/emulator_frontier_v050.py "$emu" > build/v050-frontier-playthrough.txt 2>&1
python3 scripts/emulator_v050.py "$emu" > build/v050-normal-playthrough.txt 2>&1
python3 scripts/emulator_v050.py "$emu" --advanced > build/v050-advanced-playthrough.txt 2>&1
python3 scripts/emulator_interiors_v050.py "$emu" > build/v050-interior-playthrough.txt 2>&1
python3 scripts/emulator_travel_v050.py "$emu" > build/v050-travel-playthrough.txt 2>&1
for suite in frontier weapon_art; do
    cc -O1 -g -std=gnu99 -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc src/game.c src/content.c src/armor.c src/arsenal.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c tests/test_${suite}.c -lm -o build/sanitize-${suite}
    ASAN_OPTIONS=detect_leaks=0 build/sanitize-${suite} > build/v050-sanitize-${suite}.txt 2>&1
done
cat build/v050-sanitize-frontier.txt build/v050-sanitize-weapon_art.txt > build/v050-sanitizers.txt
psp-size dustbound.prx > build/v050-memory.txt
cc -O2 -std=gnu99 -Isrc src/game.c src/content.c src/armor.c src/arsenal.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/render.c tests/benchmark_render.c -lm -o build/benchmark-v050
build/benchmark-v050 > build/v050-benchmark-stationary.csv
build/benchmark-v050 scrolling > build/v050-benchmark-scrolling.csv
echo 'All five PSP controller playthroughs, native suites and sanitizer checks passed.'
