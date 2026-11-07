# 0.8.0 validation

- tests/test_living.c: 1,200 generations covering every one of 32 mechanic/condition pairs; runtime expiry/renewal across 5,000 director ticks; 16-member materialization and garrison-blocked occupation; off-screen/restored wounds and inventory; deliberate assault/retaliation and suppression of combat dialogue prompts; one-shot delivery rewards; full-pack cache rollback; bounded actor pool; v7 stock/region migration; v8 round trip. Run under AddressSanitizer and UndefinedBehaviorSanitizer.
- tests/test_game.c: core simulation regression checks. Stationary combat fixtures now use nonsquad PLAYER actors because civilians intentionally flee; obsolete shoulder-button page navigation assertions were replaced with the current member/Skills controls.
- tests/test_expansion.c: catalog, crafting, faction, save and service regression checks. Catalog expectations include the additional 24 pieces while checking the original generated catalog boundary separately.
- scripts/emulator_v080.py: final EBOOT loaded in PPSSPP 1.20.4; real emulated buttons verify D-pad information access without movement, neutral assault/bounty, target cancellation, living journal, map views and a sixteen-member formation fixture. Captures come from the executable's completed framebuffer through its read-only QA capture path.
- scripts/validate_pbp.py: package structure and SHA-256.

Reproduce the native suite with scripts/validate_v080.sh. It writes the two prepared saves required by the emulator script. Pass PPSSPPHeadless's absolute path to scripts/emulator_v080.py after building EBOOT.PBP. Controller evidence is evidence/v080-controller-checks.json; PSP screenshots use evidence/v080-psp-*.png. Native prepared frames are separately in evidence/v080/.

This is focused qualification of the changed systems. The older milestone suites were not all rerun; no full natural campaign, physical PSP performance test, or exhaustive balance qualification is claimed. The release workflow rebuilds and checks the exact locally qualified EBOOT hash.
