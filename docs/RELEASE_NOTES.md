# Dustbound 0.14.0 — Read the frontier

- Removed map and group auto-travel. Maps place reference pins; walking is controlled with the analog stick.
- Starving bandits wear incomplete clothing and carry poor weapons. Soldiers wear coordinated equipment and march in formation. Elite duelists wear full superior gear. Appearance and loot use the same equipment.
- Nearby labels show role, faction color and a red **!** for hostility. No enemy health bars or encounter tier readouts. The faction menu includes matching colors, reputation and wanted standing.
- World activity records actual last-seen locations and times, with map references that do not follow distant groups.
- Fixed beds/workstations being intercepted by nearby NPC dialogue; improved character silhouettes through scenery.
- Added named claimants and policy previews to councils. Quieted successful autosaves and routine skill notices; prioritized contract rewards.

Extract `Dustbound-0.14.0-PSP.zip` to the memory-stick root, or replace `PSP/GAME/DUSTBOUND/EBOOT.PBP`. Keep saves before upgrading. Save v14 reads earlier formats; a real v13 save is included in migration tests. Existing saved gear remains intact; newly generated encounters use the new outfit rules.

[Details](https://github.com/andrewcirincione/Dustbound-PSP/blob/main/docs/CONTENT_V140.md) · [Screenshots and review](https://github.com/andrewcirincione/Dustbound-PSP/blob/main/docs/FEEDBACK_V140.md) · [Validation](https://github.com/andrewcirincione/Dustbound-PSP/blob/main/docs/VALIDATION_V140.md)

Tested with native sanitizer/regression suites and controller-driven PPSSPP checks of the release executable. Playable alpha; physical PSP performance and full-campaign balance remain unqualified.
