#!/usr/bin/env bash
# Run against the final EBOOT. PPSSPPHeadless is argument 1; PSPDEV must be configured.
set -eu
cd "$(dirname "$0")/.."
emu=$(realpath "$1")
mkdir -p build/depth-fixtures build/audit-fixtures build/audit-layout build/emu
rm -f build/v060-qualification.txt
bash scripts/build_native.sh > build/v060-native.txt 2>&1
make -j2 > build/v060-psp.txt 2>&1
python3 scripts/validate_pbp.py EBOOT.PBP > build/v060-pbp-validation.txt
build/test-depth build/depth-fixtures > build/v060-depth.txt
build/dustbound-native --fixtures build/emu
sources="src/game.c src/content.c src/armor.c src/arsenal.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/depth.c src/depth_combat.c"
cc -O2 -std=gnu99 -Isrc $sources src/render.c tests/make_audit_fixture.c -lm -o build/make-audit-fixture
build/make-audit-fixture build/audit-fixtures build/audit-layout
for suite in depth render audit playability; do
    cc -O1 -g -std=gnu99 -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc $sources src/render.c tests/test_${suite}.c -lm -o build/sanitize-${suite}
    ASAN_OPTIONS=detect_leaks=0 build/sanitize-${suite} > build/v060-sanitize-${suite}.txt 2>&1
done
psp-size dustbound.prx > build/v060-memory.txt
cc -O2 -std=gnu99 -Isrc $sources src/render.c tests/benchmark_render.c -lm -o build/benchmark-v060
build/benchmark-v060 > build/v060-benchmark-stationary.csv
build/benchmark-v060 scrolling > build/v060-benchmark-scrolling.csv
python3 scripts/emulator_depth_v060.py "$emu" > build/v060-depth-controller.txt 2>&1
python3 scripts/emulator_v060.py "$emu" > build/v060-normal-playthrough.txt 2>&1
python3 scripts/emulator_v060.py "$emu" --advanced > build/v060-advanced-playthrough.txt 2>&1
python3 scripts/emulator_audit_v060.py "$emu" > build/v060-audit-playthrough.txt 2>&1
python3 scripts/emulator_interiors_v060.py "$emu" > build/v060-interior-playthrough.txt 2>&1
python3 scripts/emulator_travel_v060.py "$emu" > build/v060-travel-playthrough.txt 2>&1
python3 scripts/emulator_campaign_v060.py "$emu" > build/v060-campaign-playthrough.txt 2>&1
printf 'Final native, sanitizers, PSP package and controller scenarios passed.\n' | tee build/v060-qualification.txt
