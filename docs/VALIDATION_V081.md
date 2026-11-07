# 0.8.1 atlas validation

The world overview is now a named destination browser; local view retains free cursor travel. The default map opens on your position. This intentionally changes world-map D-pad and shoulder-button behavior.

- `scripts/validate_v081.sh`: AddressSanitizer/UndefinedBehaviorSanitizer checks for discovered-town filtering, list wraparound, town/event coordinates, empty event lists, local/world selection preservation, exact travel coordinates, no self-travel order, precise contract pins for undiscovered towns, and no character movement from atlas navigation.
- `scripts/validate_v080.sh`: existing 61 core, 50 expansion and living-world event/save checks. The core arbitrary-coordinate travel fixture now opens local view first, matching the UI's documented behavior.
- `scripts/emulator_v081.py`: PPSSPP 1.20.4, final EBOOT, prepared saves 4/5. Captures unexplored geography, your position, selected town, local zoom and selected event; drives category selection, scrolling and actual travel through emulated controller buttons. Frame capture reads the EBOOT's completed framebuffer without changing game state.
- `scripts/validate_pbp.py`: PSP package structure. The publication job recompiles with pinned PSPDEV v20261101 and requires the exact emulated EBOOT hash.

No full natural campaign or physical PSP performance qualification is claimed. A 206-pixel overview still compresses nearby settlements; the named list resolves their selection, and local view remains the tool for precise movement.
