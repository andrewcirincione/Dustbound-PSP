# Validation — Dustbound 0.1.0

Validated 2026-11-03. This is emulator and host-simulation evidence; no physical PSP was available.

## Executable

The release EBOOT was compiled with PSPDEV v20261101 (GCC 15.2.0), targeting the PSP user-mode PRX format, 12 MiB heap, native 480 × 272 framebuffer and a 30 Hz presentation cadence. Actual hardware frame rate, battery use and suspend/resume remain unqualified.

SHA-256 of the emulator-tested EBOOT:

`35a5fb4c95e0223b9dec102d16b3b44e83e625213e78b45c84c3871e56220801`

`scripts/validate_pbp.py` checks the PBP section table, PARAM.SFO title, embedded PNG dimensions and PSP ELF payload. The package contains no QA marker or prepared saves.

## Native simulation

61 assertions pass in the normal build and in an AddressSanitizer + UndefinedBehaviorSanitizer build. Coverage includes transactions, inventory capacity, recruitment, movement and weight, prosthetics, bleeding and knockout, ammunition, food, healing, crafting stations and costs, research, gathering, lockpicking, controller menus, pause, travel orders, save round trips, corrupt-save rejection and 1,000 simulated seconds of world evolution. Two regression checks cover healthy followers intercepting loot while preserving first-aid access to injured followers. Logs are in `evidence/native-tests.txt` and `evidence/sanitizers.txt`.

LeakSanitizer is disabled (`ASAN_OPTIONS=detect_leaks=0`) because the hosted ptrace environment does not support it. This does not disable address or undefined-behavior checks. Passing tests are not a guarantee of complete game balance or bug-free long saves.

## Actual EBOOT playthrough

PPSSPP 1.20.4 was built from its official source archive with the software renderer and headless debugger. Two headless harness settings were changed: `coreParameter.fastForward = false` for controlled input timing, and `g_Config.bIgnoreBadMemAccess = false` so invalid emulated memory access is not hidden. No emulation-core or game-memory patches were used to advance the playthrough.

The automated sequence uses PPSSPP's debugger controller-input API. It boots the EBOOT, starts a new Wanderer expedition, opens inventory/skills/body/map, walks to Ivo, recruits him, walks to Mara, buys food, walks to the free salvage crate, loots and equips leather, walks to an iron node and gathers, saves the expedition, and reloads it. Assertions check money (650 → 350 → 315), squad count (1 → 2), the four equipped item IDs, gathered ore, screen states and persistence after loading. During this journey a hostile drifter reaches the town and combat occurs naturally.

Screens 01–07 and 13–17 are from that fresh-start playthrough. Screens 08–12 use reproducible **prepared test saves** for a three-person armored combat encounter and an eight-structure outpost. They demonstrate the same running EBOOT and menu controls, but do not imply that the outpost was built through a complete natural progression playthrough. The fixture generator is in `src/native_main.c`.

The headless screenshot endpoint did not expose the display image, so the script pauses the emulated CPU, reads the current display pointer and emulated VRAM through the official debugger API, decodes the PSP 8888 framebuffer at its 512-pixel stride, crops to the native 480 × 272 display, and resumes. These are actual game frames from the PSP executable, with no compositing or generated imagery. State reads are assertions, not game-state writes. A `DUST_QA` file enables pointer telemetry only in the test folder.

Reproduce after building the native utility and EBOOT:

```sh
mkdir -p build/emu
./build/dustbound-native --fixtures build/emu
cp EBOOT.PBP build/emu/EBOOT.PBP
python3 scripts/emulator_test.py /path/to/PPSSPPHeadless
```

Python needs Pillow and websockets. See `evidence/emulator-checks.json` and `evidence/emulator-playthrough.txt` for the assertions and input log. Screenshot naming is by scenario, not execution order.

## Release gates still open

Physical PSP-1000/2000/3000/Go tests, suspend/resume, storage-removal behavior, long multi-hour campaigns, balancing, memory pressure at actor/building caps, and complete natural progression through every crafting/research/raid system. The current renderer is intentionally simple and has no audio. Full Kenshi mechanics and content are not delivered by this alpha.
