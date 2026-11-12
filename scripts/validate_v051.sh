#!/usr/bin/env bash
# PSPDEV and its bin directory must be configured. Pass PPSSPPHeadless as argument 1.
set -eu
cd "$(dirname "$0")/.."
emu=$(realpath "$1")
mkdir -p build/emu build/frontier-fixtures build/attachment-fixtures build/clothing-fixtures
bash scripts/build_native.sh > build/v051-native.txt 2>&1
make clean > build/v051-psp.txt 2>&1
make -j2 >> build/v051-psp.txt 2>&1
python3 scripts/validate_pbp.py EBOOT.PBP > build/v051-pbp-validation.txt
build/dustbound-native --fixtures build/emu
cc -O2 -std=gnu99 -Isrc src/game.c src/content.c src/armor.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c tests/make_frontier_fixture.c -lm -o build/make-frontier-fixture
build/make-frontier-fixture build/frontier-fixtures
cc -O2 -std=gnu99 -Isrc src/game.c src/content.c src/armor.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c tests/make_attachment_fixture.c -lm -o build/make-attachment-fixture
build/make-attachment-fixture build/attachment-fixtures
python3 scripts/emulator_attachments_v051.py "$emu" > build/v051-attachments-playthrough.txt 2>&1
python3 scripts/emulator_frontier_v051.py "$emu" > build/v051-frontier-playthrough.txt 2>&1
python3 scripts/emulator_v051.py "$emu" > build/v051-normal-playthrough.txt 2>&1
python3 scripts/emulator_v051.py "$emu" --advanced > build/v051-advanced-playthrough.txt 2>&1
python3 scripts/emulator_interiors_v051.py "$emu" > build/v051-interior-playthrough.txt 2>&1
python3 scripts/emulator_travel_v051.py "$emu" > build/v051-travel-playthrough.txt 2>&1
for suite in frontier weapon_art backpacks attachments clothing; do
    cc -O1 -g -std=gnu99 -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc src/game.c src/content.c src/armor.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c tests/test_${suite}.c -lm -o build/sanitize-${suite}
    ASAN_OPTIONS=detect_leaks=0 build/sanitize-${suite} > build/v051-sanitize-${suite}.txt 2>&1
done
cat build/v051-sanitize-frontier.txt build/v051-sanitize-weapon_art.txt build/v051-sanitize-backpacks.txt build/v051-sanitize-attachments.txt build/v051-sanitize-clothing.txt > build/v051-sanitizers.txt
psp-size dustbound.prx > build/v051-memory.txt
cc -O2 -std=gnu99 -Isrc src/game.c src/content.c src/armor.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/render.c tests/benchmark_render.c -lm -o build/benchmark-v051
build/benchmark-v051 > build/v051-benchmark-stationary.csv
build/benchmark-v051 scrolling > build/v051-benchmark-scrolling.csv
echo 'All thirteen PSP controller scenarios, native suites and sanitizer checks passed.'
