# Dust 1.2.3 — completed

Published release: https://github.com/andrewcirincione/Dustbound-PSP/releases/tag/v1.2.3

Release commit: 20ceb27e7d09e2d00a71fb5089d0830c2bdfac20

EBOOT SHA-256: b4d72e4b17bce9c582edb9da875ca853c6535cbe644048d96f1c3a0546c95298

The user approved the revised in-game previews on 2026-11-18. The release
retains that exact approved pose geometry with its version updated to 1.2.3.

| Feedback | Published behavior |
| --- | --- |
| Boots sit left of the legs when facing right | Correct their right-facing registration while following the solved ankle during standing and walking. |
| Greatsword should rest on the shoulder | Passive carry uses the front hand, a 90-degree elbow and a 45-degree blade, with the rear arm relaxed. The painted lower edge fits the shoulder for each blade. Combat retains its two-handed guard and attacks. |

All 56 native suites passed in the release workflow.
The new pose regression covers 630 passive carries, 630 combat guards and
1,260 one-arm fallback poses across 21 greatswords, five species, three builds
and both facings. Strict pose reflection uses bare feet because boot-art
registration intentionally differs by facing; anatomy, armor, weapons and
attack poses are still compared.

The complete PPSSPP run passed with 78 actual captures,
including four carry models in both facings, walking, guard/return transitions
and the historical gameplay checks. Its record is bound to the release EBOOT
hash. The 600-case production-renderer audit covered
34,560 painted face pixels with
zero occluded. Physical PSP hardware was not tested.

The tested release artifact was independently downloaded. Every included
asset matches the digest and size reported by the public release API. The
installed executable matches the standalone EBOOT; the evidence's runtime
source hashes match this release source.

Save format 36 and existing expeditions remain compatible. The approved preview
is preserved in evidence/shoulder-carry-preview; current release captures are
in evidence/v123 and the downloadable evidence archive.
