#!/usr/bin/env bash
# PSPDEV and its bin directory must be configured. Pass PPSSPPHeadless as argument 1.
set -eu
cd "$(dirname "$0")/.."
emu=$(realpath "$1")
mkdir -p build/audit-fixtures build/audit-layout build/emu build/frontier-fixtures build/attachment-fixtures build/clothing-fixtures
bash scripts/build_native.sh > build/v052-native.txt 2>&1
make clean > build/v052-psp.txt 2>&1
make -j2 >> build/v052-psp.txt 2>&1
python3 scripts/validate_pbp.py EBOOT.PBP > build/v052-pbp-validation.txt
build/dustbound-native --fixtures build/emu
cc -O2 -std=gnu99 -Isrc src/game.c src/content.c src/armor.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c tests/make_frontier_fixture.c -lm -o build/make-frontier-fixture
build/make-frontier-fixture build/frontier-fixtures
cc -O2 -std=gnu99 -Isrc src/game.c src/content.c src/armor.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c tests/make_attachment_fixture.c -lm -o build/make-attachment-fixture
build/make-attachment-fixture build/attachment-fixtures
cc -O2 -std=gnu99 -Isrc src/game.c src/content.c src/armor.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/render.c tests/make_audit_fixture.c -lm -o build/make-audit-fixture
build/make-audit-fixture build/audit-fixtures build/audit-layout
python3 scripts/emulator_audit_v052.py "$emu" > build/v052-audit-playthrough.txt 2>&1
python3 scripts/emulator_attachments_v052.py "$emu" > build/v052-attachments-playthrough.txt 2>&1
python3 scripts/emulator_frontier_v052.py "$emu" > build/v052-frontier-playthrough.txt 2>&1
python3 scripts/emulator_v052.py "$emu" > build/v052-normal-playthrough.txt 2>&1
python3 scripts/emulator_v052.py "$emu" --advanced > build/v052-advanced-playthrough.txt 2>&1
python3 scripts/emulator_interiors_v052.py "$emu" > build/v052-interior-playthrough.txt 2>&1
python3 scripts/emulator_travel_v052.py "$emu" > build/v052-travel-playthrough.txt 2>&1
for suite in audit frontier weapon_art backpacks attachments clothing; do
    cc -O1 -g -std=gnu99 -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc src/game.c src/content.c src/armor.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c tests/test_${suite}.c -lm -o build/sanitize-${suite}
    ASAN_OPTIONS=detect_leaks=0 build/sanitize-${suite} > build/v052-sanitize-${suite}.txt 2>&1
done
cc -O1 -g -std=gnu99 -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc src/game.c src/content.c src/armor.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/render.c tests/test_render.c -lm -o build/sanitize-render
ASAN_OPTIONS=detect_leaks=0 build/sanitize-render > build/v052-sanitize-render.txt 2>&1
cat build/v052-sanitize-audit.txt build/v052-sanitize-render.txt build/v052-sanitize-frontier.txt build/v052-sanitize-weapon_art.txt build/v052-sanitize-backpacks.txt build/v052-sanitize-attachments.txt build/v052-sanitize-clothing.txt > build/v052-sanitizers.txt
psp-size dustbound.prx > build/v052-memory.txt
cc -O2 -std=gnu99 -Isrc src/game.c src/content.c src/armor.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/render.c tests/benchmark_render.c -lm -o build/benchmark-v052
build/benchmark-v052 > build/v052-benchmark-stationary.csv
build/benchmark-v052 scrolling > build/v052-benchmark-scrolling.csv
echo 'All fourteen PSP controller scenarios, native suites and sanitizer checks passed.'
