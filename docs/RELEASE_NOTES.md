# Dust 1.0.1 — Character fit

Shorter arms now share common shoulder positions across outfits and stances. Generated garments fit their painted collar, waist and arm openings. Elbows and wrists stay connected, and the rear arm stays behind the torso in mirrored left/right views.

- Passive arms rest at the sides, with smaller heads, fitted headwear and weapons closer to the back.
- Brigandine is narrower and centred over the hips. Resting and crawling use the same fitted attachments.
- Revised sabre, greatsword, tetsubo and unarmed poses; stable naginata hand placement and extended bow draws.

Extract **Dust-1.0.1-PSP.zip** to the memory-stick root, or replace `PSP/GAME/DUSTBOUND/EBOOT.PBP`. Keep existing saves; save format remains v29. DustAmbient.mp3 is embedded in the executable.

Validation: all 41 native suites passed, and the PSP executable compiled successfully and passed PBP and embedded-asset checks. The Evidence ZIP contains build provenance, test results and the approved production-renderer galleries covering sixteen torso styles and thirty-nine weapon/unarmed sets.

Fresh PPSSPP verification is pending. The first emulator attempt failed because of test-harness paths; the corrected retry is blocked by GitHub runner startup errors. This package makes no successful emulator-test claim. Physical PSP performance and audible output have not been measured. See `docs/BUILD_V101.json` and `docs/VALIDATION_V101.md`.
