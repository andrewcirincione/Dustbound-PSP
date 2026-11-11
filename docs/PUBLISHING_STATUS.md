# Publication status

The current release target is **Dust 1.0.1 — Character fit**, based on the published 1.0.0 release commit `d115b2e4d3ed0b17a19308c1cab2d50447596af7`.

The `release-1.0.1` branch builds with pinned PSPDEV v20261101, runs the current native suites, validates the PBP and embedded assets, and exercises the compiled PSP executable in official PPSSPP 1.20.4. Packaging requires the emulator-tested EBOOT hash. The workflow uploads a draft release, verifies each asset's GitHub SHA-256 digest, then publishes it.

The release contains `EBOOT.PBP`, `Dust-1.0.1-PSP.zip`, `Dust-1.0.1-Evidence.zip` and `SHA256SUMS-1.0.1.txt`. Installation remains `PSP/GAME/DUSTBOUND/EBOOT.PBP`, with the same v29 save format.

See [validation](VALIDATION_V101.md). Older `dist/` files and historical manifests belong to their original releases.
