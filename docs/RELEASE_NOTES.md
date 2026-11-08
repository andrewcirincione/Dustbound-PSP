# Dustbound 0.15.0 — Fight and recover

- Enemy health bars return during combat. They show remaining knockout reserve from head, chest and blood, using the actual knockout thresholds. Arm and leg injuries still affect fighting and movement without falsely emptying the bar.
- Clearer fighting: brighter outlined bars, visible windup marks, brief hit/block/dodge feedback, one focused role label during combat and less foreground vegetation around combatants.
- At most two AI melee attacks prepare or strike against one target simultaneously. Other attackers reposition; ranged enemies remain dangerous. Passive stance cancels your attack and explicit attack order, with recovery time before another attack.
- Bare and ragged torsos have slimmer profiles; heavy armor has broader silhouettes. Appearance still reflects actual equipment and loot. Nearby identification remains role, faction color and hostility.
- Six roadside rock passes provide collision and projectile cover, open central routes and outer bypasses. Six free wayfarer beds have finite medicine, food and parts caches. Discovered shelters provide recovery directions when wounded or exhausted.
- Emergency recovery: consume two cloth or destroy worn rags to stop bleeding without healing. Nearby friendly medics and pilgrims approach and help endangered squad members when safe, spending actual medicine or machine parts.
- New governments reposition surviving guards, displace some ordinary civilians and move medics to relief posts. Faction banners and policy signs show the settlement's new government. Named equipment owners are not respawned.

- Long field-guide chapters now page with left/right, keeping recovery instructions readable.

There is **no map or group auto-travel**. Map choices place reference pins; movement remains manual.

Extract `Dustbound-0.15.0-PSP.zip` to the memory-stick root, or replace `PSP/GAME/DUSTBOUND/EBOOT.PBP`. Keep saves before upgrading. Save v15 reads earlier formats; a genuine v14 PSP save is included in migration tests. Existing equipment, injuries and world state remain intact; roadside additions initialize once.

[Details](https://github.com/andrewcirincione/Dustbound-PSP/blob/main/docs/CONTENT_V150.md) · [Screenshots and review](https://github.com/andrewcirincione/Dustbound-PSP/blob/main/docs/FEEDBACK_V150.md) · [Validation](https://github.com/andrewcirincione/Dustbound-PSP/blob/main/docs/VALIDATION_V150.md)

Tested with native regression/sanitizer suites and controller-driven PPSSPP checks of the release executable. Playable alpha; physical PSP performance and full-campaign balance remain unqualified.
