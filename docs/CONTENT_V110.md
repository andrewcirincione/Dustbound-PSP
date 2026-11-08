# Dustbound 0.11.0: limb loss, replacements and surgery

Each arm and leg is independent. Heavy cutting injuries can sever an organic limb, leave a physical severed limb on the ground, and cause bleeding. Bandages stop bleeding; they cannot regrow the limb. Mechanical limbs can be destroyed by cutting or blunt damage and leave salvage parts rather than bleeding. Beasts do not use the replacement system.

One missing leg changes the character to a crawling pose and 22% of otherwise calculated movement speed. Two missing legs reduce this to 12%. Fitting both required leg replacements restores upright movement. One missing arm transfers the weapon to the remaining hand. Two-handed weapons suffer 40% less damage, 25 less accuracy and 65% longer attack delay, before strength, equipment and injury effects. One-handed weapons avoid these missing-arm penalties. With no arms, weapons are unusable and remaining kicks deal 35% damage; without arms or legs the actor cannot attack.

## Replacement grades

These bonuses apply per functioning replacement. Arm bonuses affect effective strength/dexterity, not trained XP. Leg bonuses add across installed legs. Damaged replacements at zero HP supply no positive stat bonuses. Shop prices vary by region, reputation and economy.

| Grade | Max HP | Base price | Arm STR | Arm DEX | Leg speed | Leg carrying allowance |
|---|---:|---:|---:|---:|---:|---:|
| Scrap | 70 | 250 | -2 | -2 | -8% | +0 |
| Standard | 100 | 700 | +0 | +0 | +0% | +0 |
| Reinforced | 130 | 1,450 | +4 | +0 | +3% | +4 |
| Precision | 145 | 2,800 | +2 | +5 | +10% | +2 |
| Relic | 180 | 5,200 | +6 | +6 | +15% | +6 |

Ten replacement items cover arms and legs. Either side accepts the corresponding arm or leg item. Standard replacements retain the old mechanical-arm/leg item IDs. Each grade has its own metal tint. This is original procedural rendering based on the existing painted mechanical anatomy, not ten newly illustrated sprite sheets.

## Where to buy and fit

- **Dusk Haven:** Surgeon Venn and the market sell Scrap and Standard limbs.
- **Copper Cross and Red Gulch:** through Reinforced grade.
- **Iron Gate and Shogun's Rest:** through Precision grade.
- **Black Mesa:** all five grades, including Relic.

Use the market's **Mechanical Limbs** filter. These shops also stock machine parts. Select **Health → R → Limbs** or use a replacement in the pack. Choose the specific left/right socket, then a compatible replacement. The screen compares old and new maximum HP and bonuses. Fitting costs **two machine parts** and requires an awake, free character out of combat. The old fitted part returns to the pack. A full pack rejects the swap without consuming anything. Detaching also returns the part; fitting it again still costs two parts. Ordinary machine-part repair restores metal health. Wound dressings and beds do not repair prostheses.

## Surgeon Venn

Venn is the Dusk Haven medic, marked **Limb surgeon**. Removal costs **400 silver per limb**. Select an intact arm or leg and confirm the permanent removal. Venn closes the wound. Surgery requires Venn nearby, conscious and safe; it cannot remove the head or chest, remove an already missing natural limb, or charge twice for the same removal. Buy a replacement and parts before removing a leg. Canceling the confirmation changes nothing.

## Wanderer equipment

| Wanderer | Fitted limb |
|---|---|
| Glass Ronin | Precision right arm |
| Nine Knives | Precision left leg |
| Iron Pilgrim | Relic right arm |
| The Bellmaker | Reinforced left leg |
| Red Surveyor | Precision right arm |
| Hollow Prince | Precision left leg |

These are actual equipped parts, displayed as **FITTED** in the fallen owner's loot screen. Taking one removes it from that owner and transfers it to your pack; it is not a separate generated reward. Existing theft, witness and ownership rules apply. Defeated owners must be within ordinary salvage reach. Looted parts do not respawn when reloading.

Save format v11 preserves individual loss, grade and stolen ownership, including stored traveling event actors. Actual v10 saves migrate once, preserving the old layout and prior equipment. Living wandering owners receive their authored replacements once; missing limbs on already injured owners are not silently restored.
