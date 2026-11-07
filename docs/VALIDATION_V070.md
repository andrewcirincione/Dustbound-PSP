# 0.7.0 build and layout review

The PSP executable compiles with the pinned PSPDEV v20261101 toolchain. The publish workflow rebuilds it and checks its SHA-256 against the local package before uploading.

Thirteen prepared 480x272 native-renderer frames cover normal world HUD, inventory comparison, merged health, merged squad management, skill effects, Start sections, item-selected theft, compact loot, the map and all three regional conflict pages. Frames are in `evidence/v070/`. `scripts/render_review_v070.c` reproduces their scene setup; it is a layout capture, not a gameplay test.

No gameplay suites, emulator runs or hardware tests were performed for this release. Previous milestone test records do not qualify this new executable. The regional stories, new input paths and save migration need controller playthrough qualification; no claim of tested balance or full regression coverage is made.

Save v7 appends RegionalState after the unchanged v6 Game prefix. The loader matches version and exact prefix size before copying, zeroes the appended state for older saves, validates new state, and initializes it only after a successful load. Outcome costs/rewards are one-shot; daily stock applies the stored outcome, and loading an already initialized v7 save does not restock or grant rewards.
