# 0.8.0 critical review

## Findings addressed in this pass

- Finite authored disputes cannot sustain a sandbox. A persistent director now keeps eight incident slots renewing indefinitely. It combines eight mechanics, four conditions, factions, locations, strength tiers and role-specific equipment. Recent mechanic/condition pairs are avoided. This is unending recombination, not a claim of infinitely many authored mechanics.
- Random headlines without consequences are hollow. Caravans deliver supplies; raiders and predators damage prosperity; aid replenishes stock; caches yield equipment; wounded pilgrims require treatment; armies fight and can change town ownership; warlords carry valuable gear. Failed raids/invasions can produce local relief appeals.
- Actor limits can silently make a living world stop appearing. Up to sixteen members form an army, ten a raiding column, nine a warlord retinue, seven a beast pack, or six a caravan. Distant formations use stored state and release their physical actor slots. Their wounds, inventory, ownership and hostility survive returning. Town residents and authored NPCs are never recycled by the director. Visible units must physically reach the destination before completing an advance; a visible invasion must defeat the defending garrison; the formation waits for its leader instead of outrunning it.
- Strong enemies should be worth fighting. Five encounter tiers determine skills and equipment. Strong leaders wear signature gear; their followers have equipment appropriate to their tier. Eight roles have different weapon/armor and supply tables. Twenty-four additional equipment definitions include sixteen specialist pieces and eight signature pieces. They reuse the established sprite families with distinct stats/trim; they are not twenty-four new sprite silhouettes.
- Passive/autoattack alone leaves neutral interaction incomplete. L + Square or Start > Squad > Choose opponent opens a nearby opponent list. Cross commits assault; neutral/friendly assault carries an explicit bounty/standing cost. Their group retaliates, and noncombatant settlers/recruits flee. Square cancels the deliberate order. It does not remove the victim's hostility.
- Relationships and combat focus must be readable without labels covering people. Teal line = friendly; gold dot = neutral; red chevron = hostile. Four small red corners identify the current opponent. No persistent TARGET sign. Active combat opponents no longer offer contradictory talk/pickpocket prompts.
- The atlas distorted distances and buried purpose in brown blocks. Local/world views now preserve aspect ratio, use cartographic colors, soften biome edges, show local contours, preserve roads and distinguish incidents. The sidebar describes the nearest town to the cursor, avoiding a stale town name after panning. Whole-world contours and excess number labels were removed after visual review because they added noise. Redundant player/destination labels were removed after they collided with nearby markers.

## Remaining critiques

1. Eight incident mechanics will become recognizable during long play. More authored mechanics and interactions between factions would deepen the generator; multiplying adjective combinations alone will not.
2. Group routing still uses the existing local pathfinder. Narrow crossings and crowded towns need longer multi-group playthroughs, and actual PSP hardware needs a performance pass. Off-screen battles are strategic abstractions, not frame-by-frame army simulation.
3. New equipment has meaningful mechanical differences but shares existing visual families. New silhouettes would make signature loot feel more distinctive before reading its name.
4. The full-world atlas has to fit a 480x272 screen. It is useful for broad orientation; local zoom remains necessary for choosing an approach.

## Review evidence

The final EBOOT was driven in PPSSPP through neutral assault, cancellation, D-pad information access, the living journal, local/world maps, and a prepared sixteen-member army scene. These are focused controller checks, not a natural full campaign. Native sanitizer checks cover event renewal, all mechanic/condition pairs, finite actor bounds, formation state preservation, one-shot rewards, full-pack rollback and save migration. See VALIDATION_V080.md for exact scope.
