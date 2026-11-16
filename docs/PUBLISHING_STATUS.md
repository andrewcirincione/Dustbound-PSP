# Publication status

**Dust 1.2.1 — Field repairs is published.**

[Download the PSP package](https://github.com/andrewcirincione/Dustbound-PSP/releases/download/v1.2.1/Dust-1.2.1-PSP.zip) · [Release notes and all assets](https://github.com/andrewcirincione/Dustbound-PSP/releases/tag/v1.2.1)

The release tag points to `45bd9ddbf55a7fea7db0a4c15a8510751892e2a4`. [The full qualification and publication job passed](https://github.com/andrewcirincione/Dustbound-PSP/actions/runs/43938444429), including all 54 native suites, genuine older-save migration, a valid PSP build, and the complete PPSSPP 1.20.4 controller sequence with 39 captures. The independent build also passed. Reviewed fits cover all 30 armor sets, 720 walking poses, eight skin tones, 112 hair/beard/facing combinations and eight backpack types in both directions. No physical PSP hardware test.

The final EBOOT is 12,074,456 bytes, SHA-256 `39d4b1798913e3f2529661cb230ff2c80e25f0de1da77e19fbc740e589fc0b9d`. Local and GitHub builds match exactly. All four public release assets were downloaded again and their sizes/digests checked. The PSP ZIP contains the same tested EBOOT; it contains no saves or QA marker. The evidence ZIP contains the manifest, native results, full controller record and actual renderer/emulator screenshots.

[Build manifest](BUILD_V121.json) · [Native results](NATIVE_RESULTS_V121.json) · [PSP verification](PSP_VERIFICATION_V121.json) · [Completed feedback checklist](FIXES_1_2_1.md)

Replace `PSP/GAME/DUSTBOUND/EBOOT.PBP` or extract the PSP ZIP to the memory-stick root. Keep existing saves: save format 35 upgrades earlier expeditions, including 1.2.0 format 34. Historical build records elsewhere in this repository apply only to their named versions.
