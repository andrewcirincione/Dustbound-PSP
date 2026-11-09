#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
mkdir -p build
cc -O2 -std=gnu99 -Wall -Wextra -Wno-misleading-indentation -Wno-missing-field-initializers -Isrc src/game.c src/society.c src/civilization.c src/limbs.c src/population.c src/politics.c src/content.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/depth.c src/depth_combat.c src/render.c src/native_main.c -lm -o build/dustbound-native
cc -O1 -g -std=gnu99 -Wall -Wextra -Wno-misleading-indentation -Wno-missing-field-initializers -Isrc src/game.c src/society.c src/civilization.c src/limbs.c src/population.c src/politics.c src/content.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/depth.c src/depth_combat.c tests/test_game.c -lm -o build/test-game
./build/test-game

cc -O1 -g -std=gnu99 -Wall -Wextra -Wno-misleading-indentation -Wno-missing-field-initializers -Isrc src/game.c src/society.c src/civilization.c src/limbs.c src/population.c src/politics.c src/content.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/depth.c src/depth_combat.c tests/test_expansion.c -lm -o build/test-expansion
./build/test-expansion
cc -O1 -g -std=gnu99 -Wall -Wextra -Wno-missing-field-initializers -Isrc src/game.c src/society.c src/civilization.c src/limbs.c src/population.c src/politics.c src/content.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/depth.c src/depth_combat.c src/render.c tests/test_render.c -lm -o build/test-render
./build/test-render
python3 scripts/audit_catalog.py
cc -O2 -std=gnu99 -Isrc src/game.c src/society.c src/civilization.c src/limbs.c src/population.c src/politics.c src/content.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/depth.c src/depth_combat.c tests/test_overhaul.c -lm -o build/test-overhaul
./build/test-overhaul
python3 -c "import gzip,pathlib; pathlib.Path('build/v030-frontier.sav').write_bytes(gzip.open('tests/fixtures/v030-frontier.sav.gz','rb').read())"
mkdir -p build/v172
cc -O2 -std=gnu99 -Isrc src/game.c src/society.c src/civilization.c src/limbs.c src/population.c src/politics.c src/content.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/depth.c src/depth_combat.c tests/test_playability_pass.c -lm -o build/test-playability-pass
./build/test-playability-pass
cc -O2 -std=gnu99 -Isrc src/game.c src/society.c src/civilization.c src/limbs.c src/population.c src/politics.c src/content.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/depth.c src/depth_combat.c tests/test_playability.c -lm -o build/test-playability
./build/test-playability
cc -O2 -std=gnu99 -Wall -Wextra -Wno-misleading-indentation -Wno-missing-field-initializers -Isrc src/game.c src/society.c src/civilization.c src/limbs.c src/population.c src/politics.c src/content.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/depth.c src/depth_combat.c tests/test_weapon_art.c -lm -o build/test-weapon-art
./build/test-weapon-art

python3 -c "import gzip,pathlib; pathlib.Path('build/v041-outpost.sav').write_bytes(gzip.open('tests/fixtures/v041-outpost.sav.gz','rb').read())"
cc -O2 -std=gnu99 -Isrc src/game.c src/society.c src/civilization.c src/limbs.c src/population.c src/politics.c src/content.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/depth.c src/depth_combat.c tests/test_frontier.c -lm -o build/test-frontier
./build/test-frontier

python3 -c "import gzip,pathlib; pathlib.Path('build/v050-expedition.sav').write_bytes(gzip.open('tests/fixtures/v050-expedition.sav.gz','rb').read())"
for suite in backpacks attachments clothing; do
    cc -O2 -std=gnu99 -Wall -Wextra -Wno-misleading-indentation -Wno-missing-field-initializers -Isrc src/game.c src/society.c src/civilization.c src/limbs.c src/population.c src/politics.c src/content.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/depth.c src/depth_combat.c tests/test_${suite}.c -lm -o build/test-${suite}
    build/test-${suite}
done

cc -O2 -std=gnu99 -Wall -Wextra -Wno-missing-field-initializers -Isrc src/game.c src/society.c src/civilization.c src/limbs.c src/population.c src/politics.c src/content.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/depth.c src/depth_combat.c tests/test_audit.c -lm -o build/test-audit
./build/test-audit

cc -O2 -std=gnu99 -Isrc src/game.c src/society.c src/civilization.c src/limbs.c src/population.c src/politics.c src/content.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/depth.c src/depth_combat.c src/render.c tests/test_depth.c -lm -o build/test-depth
./build/test-depth

cc -O2 -std=gnu99 -Isrc src/game.c src/society.c src/civilization.c src/limbs.c src/population.c src/politics.c src/content.c src/world.c src/expansion.c src/interior.c src/playability.c src/audio.c src/depth.c src/depth_combat.c tests/test_action_feedback.c -lm -o build/test-action-feedback
./build/test-action-feedback
