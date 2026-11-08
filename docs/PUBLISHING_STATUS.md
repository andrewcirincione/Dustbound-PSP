# Publication status

The release target is **0.16.0 — Weight and motion**.

The implementation and native validation were checkpointed on `animation-recovery-0.16.0`. Completed emulator evidence and the release manifest are included with this release checkpoint. No gameplay source was changed after the audited executable was built.

The qualified EBOOT is 1,786,886 bytes. Its SHA-256 is `1300d97fbfb5ff597d8a95bf98951b39e4ed518c05381b8ad18cf95eaadcccd7`; a clean local rebuild matched it exactly. The controller audit completed 39 cases and 9,360 frames. See [validation](VALIDATION_V160.md) for scope and limitations.

The `[publish-only]` workflow on main reruns the v080–v160 regression gates, compiles with pinned PSPDEV v20261101, validates the PBP, and verifies build-input, evidence and executable hashes against `evidence/v160-build-manifest.json`. It publishes only if those checks pass; it does not rerun PPSSPP.

The [0.16.0 release](https://github.com/andrewcirincione/Dustbound-PSP/releases/tag/v0.16.0) receives `EBOOT.PBP`, `Dustbound-0.16.0-PSP.zip`, `Dustbound-0.16.0-Screenshots.zip` and `SHA256SUMS-0.16.0.txt`. Use those assets for installation. Files under `dist/` belong to historical releases.
