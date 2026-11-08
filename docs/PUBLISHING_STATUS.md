# Publication status

The current release target is **0.10.0 — Peoples, courts and personal arms**.

The `[publish-only]` workflow runs the 0.8.0, 0.8.1, 0.9.0 and 0.10.0 native regression suites, independently compiles with pinned PSPDEV v20261101, validates the PBP, and requires the executable to match `evidence/v100-build-manifest.json` before publishing. The qualified EBOOT SHA-256 is `02af5224b66e80639a2ee3d5c7faec5b77c76141907dd09ab368bd733dd9bf09` (1,634,502 bytes).

Local PPSSPP controller qualification covers 82 completed framebuffer captures. CI verifies that it rebuilt that same executable; it does not rerun the emulator. See [validation](VALIDATION_V100.md) for the prepared-scenario scope and remaining physical-hardware limitations.

The workflow publishes `EBOOT.PBP`, `Dustbound-0.10.0-PSP.zip`, `Dustbound-0.10.0-Screenshots.zip` and `SHA256SUMS-0.10.0.txt` to the [0.10.0 release](https://github.com/andrewcirincione/Dustbound-PSP/releases/tag/v0.10.0). Use those release assets for installation; older files in `dist/` belong to historical releases.
