# Dust 1.2.2 work checkpoint

Base: published 1.2.1, `db2341042a5c6d40aaeee345d271db7f6e9d48fe`.

Requested scope (2026-11-16):

- Louder menu navigation; preserve selection sound.
- Bend the greatsword carrying elbow, lift the grip, lower the blade angle and keep the face clear in both facings.
- Increase Toughness training; increase attack/defence-related XP 50%; double field medicine XP.
- Remove the inventory `(E) EQUIPPED` legend; retain `(E)` on worn items.
- Remove Farming, Cooking, Engineering and Science as playable skills. Keep only bonfire and bedroll in player camp construction. Preserve old save field/item IDs.
- Shared storage is available only at actual depots and owned-home chests, never abandoned storehouses or other wilderness sites.
- Restore ordinary material item descriptions; remove the unsolicited leatherworking definition.
- Add WT beside QTY and VAL in inventory and trading lists.
- Remove limping text; preserve the Fight/Passive stance display.
- Rework Redoubt Nine and audit other expedition interiors: practical layouts, barracks beds, distinct floors and rewards, location at top left only, investigate and reduce indoor rendering/simulation cost.
- Clear stolen tags after two in-game days, with persistent timing across saves and transfers.

Status: implementation started; source checkout recovered. No 1.2.2 build or tests claimed yet.

Release requires native regressions, old-save migration, actual EBOOT controller checks, visual inspection, and published GitHub assets. Physical PSP verification is unavailable.

Implementation checkpoint:

- All requested gameplay/UI changes are implemented, including separate theft deadlines per stack and v36 save migration.
- Redoubt Nine has 8 sleeping berths on the barracks floor, a separate armory/maintenance floor and a command floor. All 46 expedition sites use floor-specific layouts and useful role/tier-based rewards; exhausted loot is never refilled by migration.
- Shared storage access now excludes player-built stores as well as ruins/outposts/storehouses; automatic workers also honor the depot allowlist.
- Native tests: 54 passing; one wilderness pacing regression remains under investigation (130-second gap against a 120-second limit). The 106-floor connectivity test passes.
- Two-day theft acceptance tests pass through equipping, unequipping, passing, dropping/picking up, home storage and save/reload. Separate stolen batches do not refresh each other's deadlines.
- Redoubt native standing render medians improved from 0.70–0.89 ms to 0.08–0.23 ms; tick p95 from 0.57–0.61 ms to 0.13–0.14 ms. These are host measurements, not PSP hardware measurements.
- Initial EBOOT compilation passed. Final pose changes, emulator verification and release publication remain pending.

Final qualification checkpoint:

- The inventory legend and injury stance text are removed. Weight columns are checked in both normal and pictured trading layouts. The creator also replaces the removed Engineering field with Smithing.
- All 20 added greatswords × 2 facings × 3 body types × 5 species pass an actual face-pixel occlusion audit (34,570 face pixels, zero covered). The coat/blade/leading-arm ordering is preserved, with the resting blade behind the chin. Joint, grip, attack and world-cache checks pass.
- Every storage entry point now checks the depot allowlist, including exterior contexts, indoor counters, clerk dialogues, transfers and automatic work.
- Theft ages also survive partial food/medicine use, forging/crafting, deployed bedrolls, seized property and temporary weapon confiscation. These append to v36; all released save prefixes stay unchanged.
- A complete initial native run passed 55 suites. The final regression run is in progress after the final carry/layering and theft-container checks.
- The PSP controller development pass reached every new scenario successfully. One earlier pass used an outdated stair action label and another was intentionally rejected because the source EBOOT changed during the run. Neither is release qualification. The final full run is now using a frozen executable and the corrected exact action labels.
- Matched native before/after benchmarks show 73–83% less standing rendering time and 21–27% less scrolling rendering time in Redoubt. These are host component measurements, not hardware frame rates; the raw record is in evidence/v122/interior-performance.json.
- Resume with scripts/emulator_v122.py, scripts/review_feedback_v122.py and scripts/package_v122.py. The new .github/workflows/release-v122.yml builds, tests and publishes only from release-1.2.2. Do not call a partial controller record a complete release.

Final screenshot/interaction follow-up: remove the obsolete iron/research readouts from the two-choice camp screen. An actual bed-use probe also caught expedition infirmaries inheriting private-house restrictions; the exception is now corrected, with native and PSP controller acceptance for sleeping in the ward. The full controller target is 62 captures.

A final footprint audit found ground-floor corner caches overlapping stairs at 42 sites. Those caches are repositioned, and all 106 floors now have non-overlapping stairs/chests/furniture in addition to the existing connected-path tests.
