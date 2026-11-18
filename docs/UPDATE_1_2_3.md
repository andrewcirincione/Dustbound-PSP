# Dust 1.2.3 — release qualification

The user approved the revised in-game previews on 2026-11-18 and authorized
regression testing and publication.

| Feedback | Approved result |
| --- | --- |
| Right-facing boots sit left of the legs | Correct the painted boot position while preserving its solved ankle and walking motion. |
| Greatsword resembles a two-handed instrument hold | Rest it on the shoulder at 45 degrees, held by the front hand with a 90-degree elbow. Let the rear arm hang and swing naturally. |

The exact accepted preview is preserved in `evidence/shoulder-carry-preview`
and `evidence/alignment-preview/shoulder-carry`. The release uses the same
approved geometry with version labels updated to 1.2.3.

The new carry regression covers 630 passive poses, 630 two-handed guards and
1,260 one-arm fallback poses across 21 greatswords, five species, three builds
and both facings. The historical pose reflection test uses bare feet because
the right-facing boot-art registration is deliberately asymmetric; its full
anatomy, armor and weapon comparisons remain in place.

The release workflow requires every native suite, the 600-case painted-face
audit, and a complete PSP control run including carry/guard/return transitions
and walking before it packages or publishes the executable. The evidence
archive binds capture records to the EBOOT SHA-256 and runtime source hashes.

Save format 36 and existing expeditions are retained. Physical PSP hardware
was not tested.
