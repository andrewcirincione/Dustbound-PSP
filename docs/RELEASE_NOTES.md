# Dustbound 0.8.1 — Readable world atlas

The old atlas squeezed the world into a small square, required numeric lookups, and showed a nearest-town panel even when selecting wilderness. This patch replaces the world view with a larger, named destination browser.

- World geography expands from 136×136 to 206×206 pixels, preserving proportions. The map opens on your position.
- D-pad Up/Down selects a named destination. L/R switches towns and active events. Only discovered towns appear. Empty event lists are explicit.
- One on-map name and a gold outline identify the selected destination. Blue triangle = you; white square = town; gold diamond = event. There are no town numbers or E-number lookups in the world view.
- The selected destination shows ownership or event purpose, plus direct distance and compass bearing. Nothing claims to be a planned route before you issue travel.
- Cross travels to the selected town/event. Selecting your own position does not issue a travel order. Triangle opens the local cursor map at that location; Triangle returns to the corresponding world selection.
- Temporary notifications stay in the detail area instead of covering atlas controls.
- Square still consumes a purchased survey chart. Dark terrain is unexplored; pale lines are roads. Sites and other fine details remain in local view.

Install by extracting PSP from Dustbound-0.8.1-PSP.zip to the memory-stick root, or replace PSP/GAME/DUSTBOUND/EBOOT.PBP. Save format remains v8.

Verification: native sanitizer atlas checks, existing core/expansion/event regressions, final-EBOOT controller checks in PPSSPP with prepared saves, visual review of captured frames, and release hash matching. No physical PSP performance qualification or full natural campaign is claimed.
