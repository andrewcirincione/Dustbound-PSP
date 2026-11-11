# Publication status

The current release target is **Dust 1.0.1 — Character fit**, based on the published 1.0.0 release commit `d115b2e4d3ed0b17a19308c1cab2d50447596af7`.

The PSP executable compiled successfully with pinned PSPDEV v20261101. All 41 native suites passed. The recovered CI artifact and PBP/embedded assets validate. Runtime sources are unchanged from the successful build; see [build provenance](BUILD_V101.json).

Fresh PPSSPP verification is pending: the initial harness path issue is corrected, but the retry is blocked by GitHub runner startup errors. The direct package explicitly records that limitation. The automated workflow continues to require emulator verification before it publishes; the explicit recovered-build packaging mode verifies the recorded CI artifact and marks emulator verification pending.

The release contains `EBOOT.PBP`, `Dust-1.0.1-PSP.zip`, `Dust-1.0.1-Evidence.zip` and `SHA256SUMS-1.0.1.txt`. Installation remains `PSP/GAME/DUSTBOUND/EBOOT.PBP`, with the same v29 save format.

See [validation](VALIDATION_V101.md). Older `dist/` files and historical manifests belong to their original releases.
