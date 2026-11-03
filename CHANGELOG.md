# Changelog

## 0.1.0 — 2026-11-03

First playable PSP alpha. Added overworld simulation, isometric rendering, controller UI, squad recruiting and jobs, 20 skills, combat and injuries, visible equipment, trading and salvage, camp construction/crafting/research, and persistent saves.

Playtest corrections: relocated the starting loot crate out of a solid building; preserved healthy body parts during knockout recovery; added knockout from ongoing blood loss; prevented recruit-dialogue navigation from selecting the wrong actor; exposed autosave slot 0 in the load menu; corrected enlarged equipment/body portraits overlapping their headers. PSP rendering uses two 512-stride VRAM buffers with explicit vblank synchronization.

Scope: playable alpha. Full Kenshi parity, physical PSP qualification, audio, full pathfinding and deeper faction simulation remain unfinished.

- Fixed healthy squad followers intercepting nearby loot interactions; retained first aid for injured companions.
