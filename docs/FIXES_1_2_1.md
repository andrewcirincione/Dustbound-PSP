# Dust 1.2.1 feedback checkpoint

Base: published 1.2.0, source c9706bf; main 04b1e7a.
Status: complete and published as Dust 1.2.1. Release source `45bd9ddbf55a7fea7db0a4c15a8510751892e2a4`; all downloads independently retrieved and verified.

## Required changes

- [x] Match exposed arm and face skin; anchor and vary hair and beards.
- [x] Correct backpack sizes/facing and Iron Labourer headwear/boots.
- [x] Correct katana grip, greatsword shoulder clearance, walking leg proportions, axe artifacts.
- [x] Quest descriptions before acceptance; abandon accepted contracts.
- [x] Camp theft targets individual chests; remove unavailable camp actions and enemy restrictions.
- [x] Forging previews show actual resulting equipment stats.
- [x] Loot actions show remaining/taken percentage; vital beast salvage kills immediately.
- [x] Public depot services only at actual depots.
- [x] Sell at half listed item value, with local market modifiers; raise weapon/armor prices and weights.
- [x] Explain and make useful leatherworking, farming, cooking, engineering, science.
- [x] Quote bed fees, scaled by settlement importance.
- [x] Slow bleeding; show consciousness recovery; block all-downed autosaves.
- [x] Diagnose and correct fight-mode approach/attack stalls.
- [x] Replace resting countdown with meaningful recovery information.
- [x] Equipped armor/weapons remain visible and marked (E) in item lists, including selling.
- [x] Include equipped weight; injuries progressively reduce carrying capacity.
- [x] Improve rain appearance.

## Qualification

- [x] Targeted gameplay regression coverage and existing suites pass.
- [x] Native visual review covers affected equipment and character variants.
- [x] PSP build and package validation pass.
- [x] Actual release EBOOT tested through ordinary save loads and controller input in PPSSPP 1.20.4; 39 screenshots captured and affected screens visually inspected.
- [x] Source checkpoints pushed; all four release asset sizes and SHA-256 digests verified; 1.2.1 published.

## Final provenance

- [Published release](https://github.com/andrewcirincione/Dustbound-PSP/releases/tag/v1.2.1)
- [Successful qualification and publication](https://github.com/andrewcirincione/Dustbound-PSP/actions/runs/43938444429)
- [Independent successful build](https://github.com/andrewcirincione/Dustbound-PSP/actions/runs/43938444903)
- [Exact build manifest](BUILD_V121.json), [54-suite results](NATIVE_RESULTS_V121.json), [full PSP controller record](PSP_VERIFICATION_V121.json)
- EBOOT: 12,074,456 bytes; SHA-256 `39d4b1798913e3f2529661cb230ff2c80e25f0de1da77e19fbc740e589fc0b9d`.
- Local and GitHub builds are byte-identical. Downloaded PSP ZIP contains that same executable, with no save files or QA marker.
- All screenshots and review sheets are in `Dust-1.2.1-Evidence.zip` on the release. No physical PSP hardware test.
