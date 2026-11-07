# Combat and skill audit — 0.9.0

Checked against the Kenshi wiki on 2026-11-07. Direct page retrieval was unavailable; indexed wiki passages were retrieved through two search engines. This documents a gameplay adaptation, not a port or exact formula reproduction.

## Combat behavior

| Stat | Training in Dustbound | Practical effect |
|---|---|---|
| Strength | Weapon swings, unarmed strikes, smaller gains from blocks/hits; carrying and mining | Load allowance, heavy-weapon handling, blunt/unarmed damage |
| Dexterity | Attacks; cutting weapon profiles teach more than blunt; modest ranged gain | Timing and cutting damage |
| Melee | Melee attempts; blocked attempts grant half normal XP | Hit probability |
| Defense | Armed defense attempts; a hit grants eight times block XP before opponent scaling | Block probability |
| Toughness | Incoming pre-mitigation damage, scaled against current toughness | Damage reduction; contributes to martial damage |
| Dodge | Unarmed, ranged-weapon or staggered defense; hits grant twice avoided-hit XP | Evasion; overburden reduces effectiveness and XP |
| Martial arts | Unarmed attempts | Unarmed accuracy and damage |
| Weapon classes | Use of that class, with lower gains for blocked strikes and small block practice | +0.25% damage and -0.15% handling delay per level above one |
| Crossbows/ranged | Projectile impact resolution | Ranged accuracy |
| Perception | Discovery, contracts and expedition activities | Ranged accuracy bonus |

Weapon classes: katanas, sabres (including scimitars), hackers, heavy blades (including greatswords), blunt, polearms, rapiers, knives, bows, crossbows and firearms. Class progress is stored independently of the 20 existing core skills, preserving old Actor/save layouts during migration.

Opponent scaling spans 0.1× for a much weaker opponent to 6× for an opponent 50 levels stronger. Attack and defense compare their relevant opposing stats. Weapon-class XP compares class proficiency to opposing defense. The underlying level curve remains Dustbound's quadratic XP requirement.

Wiki references: [Stronger Opponent Logic](https://kenshi.fandom.com/wiki/Stronger_Opponent_Logic), [Strength](https://kenshi.fandom.com/wiki/Strength), [Dexterity](https://kenshi.fandom.com/wiki/Dexterity), [Toughness](https://kenshi.fandom.com/wiki/Toughness), [Dodge](https://kenshi.fandom.com/wiki/Dodge), [Melee Defense](https://kenshi.fandom.com/wiki/Melee_Defense).

## Other existing skills

| Skill | Existing action → benefit |
|---|---|
| Athletics | Walking → travel speed |
| Stealth | Sneaking → reduced detection, stronger disguises |
| Theft | Pickpocketing and owned salvage → success chance |
| Lockpicking | Locks, shackles, escape attempts → success/speed |
| Medicine | Treating wounds → healing effectiveness |
| Labour | Gathering → faster gathering |
| Farming | Planting and harvest → yield |
| Cooking | Cooking recipes → ration yield |
| Smithing | Forging → equipment grade access |
| Engineering | Construction, maintenance and repairs → work/cost benefits |
| Science | Research → research cost benefits |

Reference: [Statistics](https://kenshi.fandom.com/wiki/Statistics), [Lockpicking](https://kenshi.fandom.com/wiki/Lockpicking), [Athletics](https://kenshi.fandom.com/wiki/Athletics), [Training](https://kenshi.fandom.com/wiki/Guide_to_Training_Statistics).

## Deliberate differences and remaining gaps

Dustbound does not implement every Kenshi stat. Swimming, assassination, turrets, precision shooting and separate robotics/armor/crossbow smithing skills are absent or combined into existing skills. Dustbound's carry capacity, strength requirement, damage reduction, lock difficulty, ancestry effects and progression pace differ. There is no playing-dead/get-up toughness bonus, muscle growth, Kenshi wound-degeneration curve, exact racial XP tables or multiple martial move unlocks. Ranged training resolves when a projectile reaches a character; complete misses do not train it. Melee cutting/blunt shares are class profiles, not Kenshi's per-weapon damage data. Existing general skill XP is preserved; new class XP starts at level one.

Automated combat tests exercise strikes, blocks, hits, unarmed and staggered dodge, burden, opponent scaling, armor-independent toughness training, class separation, handling benefits and persistence. Final-EBOOT controller evidence additionally checks that combat increases weapon-class XP.
