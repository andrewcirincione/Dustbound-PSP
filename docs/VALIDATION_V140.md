# 0.14.0 validation

## Native gates

`validate_v080.sh`, `validate_v081.sh`, `validate_v090.sh`, `validate_v100.sh`, `validate_v110.sh`, `validate_v120.sh`, `validate_v130.sh` and `validate_v140.sh` cover current game/expansion, atlas, society/rescue/training, civilization/equipment/crafting, limb, population, world behavior and this release's changes. Sanitizer suites use AddressSanitizer and UndefinedBehaviorSanitizer. Results are retained in `evidence/v140-native-checks.txt`.

The new gate checks stationary map pins, removed R+Cross routes, clearing stale group travel, analog movement with a retained pin, the actual v13 inn save and exact bed cost, persistent historical sightings, concealed ambushers, corpse observation not tracking distant survivors, invalid snapshot rejection, real equipment loadouts, three-wide military placement, framebuffer bounds, distinct council names/policies, one-time spending and notification priority.

The behavior regression includes 12,000 full simulation ticks (20 simulated minutes) and 18,000 coarse world ticks (five simulated hours / ten game days), with save/reload and state validation. These are simulation tests, not controller playtime.

## PSP executable

Pinned PSPDEV `v20261101`, normal release build, 480×272 completed framebuffers. `scripts/emulator_v140.py` uses PPSSPP software rendering and normal button/analog input. It reads QA telemetry and requests completed-frame captures; it does not write gameplay memory.

- Fresh character creation, a map pin followed by 180 stationary frames, analog walking outside Dusk Haven, and save/reload.
- Prepared ordinary saves for the old inn/bed case, starving bandits, army formation, elite duelists, downed visibility, concealed/revealed ambush, last-seen ledger and council.
- Bed charges exactly 25 and recovers health; ambush changes from concealed to sprung; pinning a historical sighting leaves the player stationary; repeated council pledge does not double-charge.
- Sixteen screenshots reviewed for labels, silhouettes, map controls and council/faction layout. Results and the tested EBOOT hash are in `evidence/v140-controller-checks.json` and `evidence/v140-build-manifest.json`.

GitHub's publication job reruns all eight gates, rebuilds with the pinned toolchain, validates the PBP and rejects an executable whose SHA-256 differs from the emulated build before packaging/publishing it.

## Limits

No physical PSP benchmark. This focused controller pass does not establish natural full-campaign balance or an exhaustive human playthrough of all routes. Offline battles remain simplified. Historical validation scripts that require removed automatic travel are superseded by the stationary-pin tests; the release gates are explicitly listed above. Existing saved equipment is preserved, while new encounter generations receive the new outfits.
