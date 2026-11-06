#include "game.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

int npc_role(const Actor *a) {
    if (a->voice == ROLE_MERCENARY)
        return ROLE_MERCENARY;
    if (a->kind == TRADER)
        return ROLE_MERCHANT;
    if (a->kind == RECRUIT)
        return ROLE_RECRUIT;
    if (a->voice >= ROLE_INNKEEPER && a->voice <= ROLE_SCOUT)
        return a->voice;
    if (a->kind == GUARD)
        return ROLE_MARSHAL;
    if (a->kind == CARAVAN)
        return ROLE_SCOUT;
    if (strstr(a->name, "Inn"))
        return ROLE_INNKEEPER;
    return ROLE_CITIZEN;
}
const char *npc_role_name(const Actor *a) {
    if ((uintptr_t)a >= (uintptr_t)g.actors && (uintptr_t)a < (uintptr_t)(g.actors + MAX_ACTORS)) {
        int n = depth_named((int)(a - g.actors));
        if (n == NPC_SORA)
            return "ENGINEER";
        if (n == NPC_NELL)
            return "QUARTERMASTER";
        if (n == NPC_RELL)
            return "TOLL WARLORD";
        if (n == NPC_ORIN || n == NPC_TAMSIN)
            return "CHAIN YARD SURVIVOR";
    }
    if (a->kind == RAIDER)
        return "RAIDER";
    if (a->kind == BEAST)
        return "BEAST";
    static const char *names[] = {"RESIDENT", "MERCHANT", "RECRUIT", "MARSHAL", "INNKEEPER",
                                  "MEDIC",    "SMITH",    "BARBER",  "SCOUT",   "CONTRACT GUARD"};
    return names[npc_role(a)];
}
static const int services[][6] = {
    {6, 12, 8, -1},    {0, 6, 12, 8, -1}, {6, 12, 8, -1},    {1, 4, 6, 8, -1}, {2, 6, 12, 8, -1},
    {3, 6, 12, 8, -1}, {7, 6, 12, 8, -1}, {5, 6, 12, 8, -1}, {6, 12, 8, -1},   {9, 10, 6, 12, 8}};
int dialogue_count(int actor) {
    if (actor < 0 || actor >= g.actorCount)
        return 0;
    int named = depth_named(actor);
    if (named >= 0 && named <= NPC_TAMSIN)
        return named == NPC_NELL ? 5 : named == NPC_RELL ? 4 : 3;
    int n = 0;
    while (n < 5 && services[npc_role(&g.actors[actor])][n] >= 0)
        n++;
    return n;
}
int dialogue_action(int actor, int row) {
    if (row < 0 || row >= dialogue_count(actor))
        return 8;
    if (npc_role(&g.actors[actor]) == ROLE_MERCENARY && merc_slot(&g.actors[actor]) >= 0 && row == 0)
        return 11;
    return services[npc_role(&g.actors[actor])][row];
}
const char *dialogue_option(int actor, int row) {
    static const char *labels[] = {"Show me your stock.",
                                   "Any paying work?",
                                   "Rent a bed. 25 silver",
                                   "Treat my wounds. 80 silver",
                                   "Bounties and local news.",
                                   "Change my appearance.",
                                   "What have you heard?",
                                   "Use the smithy.",
                                   "Safe travels.",
                                   "Escort me. 240 silver / day",
                                   "Guard my camp. 180 silver / day",
                                   "End contract. No refund.",
                                   "Tell me about your faction."};
    if (depth_named(actor) >= 0 && depth_named(actor) <= NPC_TAMSIN)
        return depth_option(actor, row);
    int action = dialogue_action(actor, row);
    if (action == 6 && npc_role(&g.actors[actor]) == ROLE_RECRUIT)
        return "Join my expedition. 300 silver";
    return labels[action];
}
float context_distance(void) {
    Actor *a = &g.actors[g.selected];
    int n = ui.contextIndex;
    if (n < 0)
        return 999;
    if (ui.contextKind == 4)
        return distance(a->x, a->y, g.depth.siteX[n], g.depth.siteY[n]);
    if (ui.contextKind == 0)
        return distance(a->x, a->y, g.actors[n].x, g.actors[n].y);
    if (ui.contextKind == 1)
        return distance(a->x, a->y, g.nodes[n].x, g.nodes[n].y);
    Building *b = &g.buildings[n];
    return distance(a->x, a->y, b->x + (ui.contextKind == 3 ? 1.45f : 0),
                    b->y + (ui.contextKind == 3 ? .65f : 0));
}
float context_reach(void) {
    return ui.contextKind == 1 ? WORK_REACH : ui.contextKind == 0 ? TALK_REACH : 1.7f;
}
const char *context_name(void) {
    int n = ui.contextIndex;
    if (n < 0)
        return "NO NEARBY TARGET";
    if (ui.contextKind == 4)
        return landmarkNames[n];
    if (ui.contextKind == 0)
        return g.actors[n].name;
    if (ui.contextKind == 1) {
        static const char *s[] = {"IRON DEPOSIT",  "COPPER DEPOSIT", "DEADWOOD",
                                  "SALVAGE CHEST", "LOCKED CHEST",   "OLD RUINS"};
        return s[g.nodes[n].type];
    }
    static const char *s[] = {"FRONTIER HOME",  "TRADING POST", "INN",     "WATCH HOUSE", "WINDPUMP",
                              "WELL",           "CAMPFIRE",     "BEDROLL", "CROP FIELD",  "SMITHY",
                              "RESEARCH BENCH", "PALISADE",     "DEPOT"};
    return s[g.buildings[n].type];
}
const char *context_action(void) {
    int n = ui.contextIndex;
    if (n < 0)
        return "WALK TO A PERSON, DOOR OR RESOURCE";
    if (ui.contextKind == 4)
        return "INSPECT LANDMARK";
    if ((ui.contextKind == 2 || ui.contextKind == 3) && !depth_build_ready(n))
        return "CONTINUE CONSTRUCTION";
    if (ui.contextKind == 3)
        return "ENTER";
    if (ui.contextKind == 0) {
        Actor *b = &g.actors[n];
        if (depth_named(n) >= 0 && depth_named(n) <= NPC_TAMSIN)
            return "STORY / RESCUE";
        return b->squad                        ? "RESCUE OPTIONS"
               : b->dead || b->unconscious > 0 ? "LOOT"
               : g.actors[g.selected].sneaking ? "PICKPOCKET"
                                               : "TALK";
    }
    if (ui.contextKind == 1) {
        Node *b = &g.nodes[n];
        return b->locked                ? "PICK LOCK"
               : b->type <= NODE_COPPER ? "MINE"
               : b->type == NODE_WOOD   ? "GATHER"
                                        : "TAKE";
    }
    int t = g.buildings[n].type;
    return t == WELL                   ? "DRAW WATER"
           : t == FARM                 ? (g.buildings[n].progress >= 100 ? "HARVEST" : "WATER CROP")
           : t == BED || t == CAMPFIRE ? "REST"
           : t == FORGE                ? "CRAFT"
           : t == BENCH                ? "RESEARCH"
                                       : "OPEN STORAGE";
}

/* Expand travel distances fourfold without enlarging towns or sprite scale.
   The exploration bitmap remains 384 squared: one cell covers four world units.
   Version 4 preserves the v3 byte layout, but records this new coordinate system. */
static Town oldTowns[TOWNS];
static int captured, expanded;
void world_layout(int large) {
    if (!captured) {
        memcpy(oldTowns, towns, sizeof(oldTowns));
        captured = 1;
    }
    for (int t = 0; t < TOWNS; t++) {
        towns[t] = oldTowns[t];
        if (large) {
            towns[t].x = oldTowns[t].x * 4 - 78;
            towns[t].y = oldTowns[t].y * 4 - 81;
        }
    }
    expanded = large;
}
int world_is_expanded(void) { return expanded; }
void place_wilderness_encounters(void) {
    for (int i = 0; i < g.actorCount; i++) {
        Actor *a = &g.actors[i];
        if (a->squad || (a->kind != RAIDER && a->kind != BEAST))
            continue;
        int town = -1;
        for (int t = 0; t < TOWNS; t++)
            if (towns[t].faction != HOSTILE && distance(a->x, a->y, towns[t].x, towns[t].y) < 32) {
                town = t;
                break;
            }
        if (town < 0)
            continue;
        float angle = atan2f(a->y - towns[town].y, a->x - towns[town].x);
        for (int k = 0; k < 24; k++) {
            float radius = 36 + k * .5f, an = angle + k * .55f;
            float x = towns[town].x + cosf(an) * radius, y = towns[town].y + sinf(an) * radius;
            if (!can_walk(x, y))
                continue;
            int safe = 1;
            for (int t = 0; t < TOWNS; t++)
                if (towns[t].faction != HOSTILE && distance(x, y, towns[t].x, towns[t].y) < 32)
                    safe = 0;
            if (!safe)
                continue;
            a->x = a->homeX = a->tx = x;
            a->y = a->homeY = a->ty = y;
            break;
        }
    }
}
typedef struct {
    int baseExists;
    float baseX, baseY;
} OldBase;
static void expand_point(float *x, float *y, const OldBase *old) {
    float ox = *x, oy = *y;
    if (old->baseExists && distance(ox, oy, old->baseX, old->baseY) < 24) {
        *x = old->baseX * 4 - 78 + ox - old->baseX;
        *y = old->baseY * 4 - 81 + oy - old->baseY;
    } else {
        int t = -1;
        float best = 18;
        for (int j = 0; j < TOWNS; j++) {
            float d = distance(ox, oy, oldTowns[j].x, oldTowns[j].y);
            if (d < best) {
                best = d;
                t = j;
            }
        }
        *x = t >= 0 ? oldTowns[t].x * 4 - 78 + ox - oldTowns[t].x : ox * 4 - 78;
        *y = t >= 0 ? oldTowns[t].y * 4 - 81 + oy - oldTowns[t].y : oy * 4 - 81;
    }
    *x = fminf(WORLD - 3, fmaxf(3, *x));
    *y = fminf(WORLD - 3, fmaxf(3, *y));
}
void spread_frontier(Game *save) {
    world_layout(1);
    /* Only base coordinates are read from the old state during point mapping. */
    OldBase base = {0};
    base.baseExists = save->baseExists;
    base.baseX = save->baseX;
    base.baseY = save->baseY;
    for (int i = 0; i < save->actorCount; i++) {
        Actor *a = &save->actors[i];
        expand_point(&a->x, &a->y, &base);
        expand_point(&a->homeX, &a->homeY, &base);
        expand_point(&a->tx, &a->ty, &base);
        a->routeLen = a->routePos = 0;
        a->routeClock = 0;
    }
    for (int i = 0; i < save->buildingCount; i++)
        expand_point(&save->buildings[i].x, &save->buildings[i].y, &base);
    for (int i = 0; i < save->nodeCount; i++)
        expand_point(&save->nodes[i].x, &save->nodes[i].y, &base);
    save->baseX = fmaxf(3, base.baseX * 4 - 78);
    save->baseY = fmaxf(3, base.baseY * 4 - 81);
    /* Old cells represented individual world units. Reproject explored cells,
       including compact settlements, into the new four-unit survey grid. */
    unsigned char oldSeen[sizeof(save->seen)];
    memcpy(oldSeen, save->seen, sizeof(oldSeen));
    memset(save->seen, 0, sizeof(save->seen));
    for (int y = 0; y < FOG_SIZE; y++)
        for (int x = 0; x < FOG_SIZE; x++) {
            int k = y * FOG_SIZE + x;
            if (!(oldSeen[k >> 3] & (1 << (k & 7))))
                continue;
            float wx = x + .5f, wy = y + .5f;
            expand_point(&wx, &wy, &base);
            int nk = ((int)wy / 4) * FOG_SIZE + (int)wx / 4;
            save->seen[nk >> 3] |= 1 << (nk & 7);
        }
    for (int i = 0; i < save->actorCount; i++)
        if (save->actors[i].squad) {
            int x = (int)save->actors[i].x / 4, y = (int)save->actors[i].y / 4;
            for (int yy = y - 2; yy <= y + 2; yy++)
                for (int xx = x - 2; xx <= x + 2; xx++)
                    if (xx >= 0 && yy >= 0 && xx < FOG_SIZE && yy < FOG_SIZE) {
                        int k = yy * FOG_SIZE + xx;
                        save->seen[k >> 3] |= 1 << (k & 7);
                    }
        }
    /* Add specialists to migrated expeditions as well as new games. */
    for (int i = 0; i < save->actorCount; i++) {
        Actor *a = &save->actors[i];
        if (!strcmp(a->name, "Gate guard") || !strcmp(a->name, "Gate warden")) {
            a->kind = SETTLER;
            a->voice = ROLE_MEDIC;
            snprintf(a->name, 16, "Town medic");
        } else if (!strcmp(a->name, "Wayfarer")) {
            a->voice = ROLE_BARBER;
            snprintf(a->name, 16, "Trail barber");
        } else if (a->kind == CARAVAN) {
            a->kind = SETTLER;
            a->voice = ROLE_SMITH;
            snprintf(a->name, 16, "Frontier smith");
        }
    }
}

/* Frontier identity and contracts. All storage is fixed-size and saved in v5. */
extern int add_actor(const char *, int, int, float, float, int);
extern int conscious(const Actor *);
static const int doctrine[FACTIONS][4] = {{8, 9, 16, 19},   {29, 8, 84, 30}, {85, 12, 21, 11},
                                          {87, 11, 23, 20}, {0, 2, 86, 5},   {16, 5, 14, 86},
                                          {30, 7, 87, 9},   {23, 14, 15, 20}};
const char *faction_identity(int f) {
    static const char *l[] = {
        "FREE DRIFTERS: independent recruits and hired blades. No ruler, no uniform law.",
        "COMPACT: rifle wardens and greatswords. Protects wells; arrests wanted drifters.",
        "RUST CARAVANS: salvage, mauls and work gear. Bring ore; leave with tools.",
        "RED VULTURES: heavy scavenged blades. Attacks outsiders. Avoid their settlements.",
        "SHOGUNATE: lamellar patrols, katanas and glaives. Honor buys trade; infamy draws steel.",
        "ASH MONASTERY: robes, staves and medicine. Recovery supplies and pilgrim equipment.",
        "BLACK MESA: pistols, hooks and contraband. Fences accept stolen goods.",
        "WILD PACKS: territorial predators. No trade, contracts or diplomatic protection."};
    return l[f >= 0 && f < FACTIONS ? f : 0];
}
const char *npc_rumor(const Actor *a) {
    switch (npc_role(a)) {
    case ROLE_MERCHANT:
        return a->faction == UNION ? "Compact stock includes greatswords and rifles. Copper Cross sells "
                                     "heavy salvage tools. Save silver for food before steel."
               : a->faction == SCAVENGERS
                   ? "Railway mauls punch through armor, but weak arms swing slowly. "
                     "Look for our field-grade stock; masterwork rotates daily."
               : a->faction == SHOGUN
                   ? "Our glaives outrange swords. Heavy armor slows a wounded traveler. "
                     "Black Mesa trades stranger weapons, if you survive the road."
               : a->faction == OUTLAWS
                   ? "Stolen goods move here. Anchor cleavers bleed enemies, but their "
                     "weight can leave you open. No refunds for dead clients."
                   : "Stock follows our faction's craft. Your standing affects prices. "
                     "Inspect weight, reach and strength before buying.";
    case ROLE_MARSHAL:
        return "Bring contracts back to the journal for payment. Guards protect this town, not the "
               "whole "
               "road. Hire an escort before leaving alone.";
    case ROLE_MEDIC:
        return "First aid stops bleeding; a bed finishes recovery. Injured arms slow heavy weapons even "
               "when "
               "your strength is high.";
    case ROLE_SMITH:
        return "Greatswords need strength; mauls defeat armor; glaives trade impact for reach. Choose a "
               "weapon your injured arms can still lift.";
    case ROLE_INNKEEPER:
        return "Recruits join for good. Contract guards work one day and then return home. They wait "
               "outside "
               "when you enter buildings.";
    case ROLE_RECRUIT:
        return "I join permanently and take your orders. A hired guard keeps their own gear, fights "
               "independently and leaves when the contract ends.";
    case ROLE_MERCENARY:
        return "One guard, one day: 1800 seconds of moving world time. Escort follows your selected "
               "survivor; camp duty requires an outpost. No refunds.";
    case ROLE_SCOUT:
        return "Copper Cross has mauls. Shogunate markets carry glaives. Survey the atlas before "
               "choosing a "
               "long route; towns are islands of safety.";
    default:
        return faction_identity(a->faction);
    }
}
void frontier_restock(int t) {
    int f = g.townOwner[t];
    if (f < 0 || f >= FACTIONS)
        f = towns[t].faction;
    for (int it = BASE_ITEMS; it < ITEMS; it++)
        g.stock[t][it] = 0;
    for (int j = 0; j < 4; j++) {
        g.stock[t][gear_id(doctrine[f][j], 1, 1)] = 2;
        g.stock[t][gear_id(doctrine[f][j], (t + g.day + j) % MATERIALS, 2 + (g.day + j) % 2)] = 1;
    }
    static const int torso[] = {33, 44, 46, 37, 34, 39, 45, 37},
                     head[] = {48, 49, 56, 55, 50, 57, 52, 55};
    for (int j = 0; j < 18; j++) {
        int family = j % 6 == 0   ? torso[f]
                     : j % 6 == 1 ? head[f]
                     : j % 6 == 2 ? 60 + (f + j) % 8
                     : j % 6 == 3 ? 68 + (f + j) % 8
                                  : 76 + (f + j) % 8;
        g.stock[t][gear_id(family, (j / 6 + t + g.day) % 6, (j + g.day) % 3)] = 1;
    }
    // A usable choice must not depend on the rotating high-grade stock roll.
    static const int packs[] = {76, 78, 79, 82};
    for (int j = 0; j < 4; j++)
        g.stock[t][gear_id(packs[j], 0, 0)] = 2;
    if (f == MONKS) {
        g.stock[t][MEDKIT] = 30;
        g.stock[t][TONIC] = 12;
    }
    if (f == SCAVENGERS) {
        g.stock[t][IRONBAR] = 20;
        g.stock[t][TOOLS] = 8;
    }
}
static void identity_outfit(Actor *a) {
    int f = a->faction, role = npc_role(a), m = f == SHOGUN ? 4 : f == OUTLAWS ? 0 : 1;
    static const int torso[] = {33, 44, 46, 37, 34, 39, 45, 37},
                     head[] = {48, 49, 56, 55, 50, 57, 52, 55};
    int combat = role == ROLE_MARSHAL || role == ROLE_MERCENARY;
    a->gear[0] = gear_id(combat               ? doctrine[f][(a->homeTown + f) % 4]
                         : role == ROLE_SMITH ? 85
                         : role == ROLE_MEDIC ? 16
                                              : 3,
                         m, combat ? 2 : 1);
    a->gear[1] = gear_id(role == ROLE_MEDIC      ? 39
                         : role == ROLE_SMITH    ? 35
                         : role == ROLE_MERCHANT ? 32
                                                 : torso[f],
                         m, combat ? 2 : 1);
    a->gear[2] = gear_id(role == ROLE_MEDIC ? 57 : role == ROLE_MERCHANT ? 56 : head[f], m, 1);
    a->gear[3] = gear_id(f == SHOGUN ? 63 : f == MONKS ? 61 : 60, m, 1);
    a->gear[4] = gear_id(f == SHOGUN ? 68 : f == MONKS ? 73 : 69, m, 1);
    a->gear[5] = gear_id(role == ROLE_MEDIC       ? 78
                         : role == ROLE_MERCHANT  ? 79
                         : role == ROLE_MERCENARY ? 82
                                                  : 77,
                         m, 1);
    if (weapon_ranged(a->gear[0]))
        inventory_add(a, gear_family(a->gear[0]) >= 29 ? CARTRIDGE : BOLTS, 60);
}
void frontier_identity(void) {
    if (g.identityVersion >= 2)
        return;
    if (g.identityVersion == 1) {
        // One-time 0.5.0 market supplement. Keep all other stock, purchases,
        // outfits and contracts intact; loading again cannot restock goods.
        static const int packs[] = {76, 78, 79, 82};
        for (int t = 0; t < TOWNS; t++)
            for (int j = 0; j < 4; j++) {
                int it = gear_id(packs[j], 0, 0);
                if (!g.stock[t][it])
                    g.stock[t][it] = 2;
            }
        g.identityVersion = 2;
        return;
    }
    for (int i = 0; i < g.actorCount; i++) {
        Actor *a = &g.actors[i];
        if (a->squad || a->dead || a->kind == BEAST || a->kind == RAIDER)
            continue;
        identity_outfit(a);
        /* A service must have its own approach, not overlap the market keeper. */
        if (npc_role(a) == ROLE_MEDIC) {
            int t = town_near(a->homeX, a->homeY, 14);
            if (t >= 0) {
                a->x = a->homeX = towns[t].x - 7;
                a->y = a->homeY = towns[t].y + 3;
                a->routeLen = a->routePos = 0;
            }
        }
    }
    const int hubs[] = {0, 1, 5, 8};
    for (int k = 0; k < 4; k++) {
        int t = hubs[k],
            n = add_actor("Contract guard", GUARD, FREE, towns[t].x + 4, towns[t].y + 7, 22);
        if (n < 0)
            break;
        Actor *a = &g.actors[n];
        a->voice = ROLE_MERCENARY;
        a->homeTown = t;
        identity_outfit(a);
        a->gear[0] = gear_id(84 + k, 1, 1);
        inventory_add(a, MEDKIT, 4);
    }
    for (int t = 0; t < TOWNS; t++)
        frontier_restock(t);
    g.identityVersion = 2;
}
int merc_slot(const Actor *a) {
    for (int j = 0; j < 4; j++)
        if (g.mercActor[j] > 0 && g.mercActor[j] <= g.actorCount && &g.actors[g.mercActor[j] - 1] == a &&
            g.mercUntil[j] > g.time)
            return j;
    return -1;
}
int merc_hire(int n, int mode) {
    if (n < 0 || n >= g.actorCount || mode < 0 || mode > 1)
        return 0;
    Actor *a = &g.actors[n], *p = &g.actors[g.selected];
    if (npc_role(a) != ROLE_MERCENARY || !conscious(a) ||
        distance(a->x, a->y, p->x, p->y) > TALK_REACH || actor_inside(p))
        return 0;
    if (merc_slot(a) >= 0) {
        log_msg("Already under contract. No extra payment taken.");
        return 0;
    }
    if (mode && !g.baseExists) {
        log_msg("Build an outpost before hiring a camp guard.");
        return 0;
    }
    int cost = mode ? 180 : 240, slot = -1;
    for (int i = 0; i < 4; i++)
        if (!g.mercActor[i] || g.mercUntil[i] <= g.time) {
            slot = i;
            break;
        }
    if (slot < 0 || g.money < cost) {
        log_msg("Need %d silver and a free contract slot.", cost);
        return 0;
    }
    g.money -= cost;
    g.mercActor[slot] = n + 1;
    g.mercUntil[slot] = g.time + 1800;
    g.mercMode[slot] = mode;
    log_msg(mode ? "Camp guard hired for one day."
                 : "Escort hired for one day. Independent, not a recruit.");
    return 1;
}
void merc_dismiss(int n) {
    if (n < 0 || n >= g.actorCount)
        return;
    int s = merc_slot(&g.actors[n]);
    if (s >= 0) {
        g.mercActor[s] = 0;
        g.mercUntil[s] = 0;
        log_msg("Contract ended. Guard returns home; no refund.");
    }
}
void frontier_tick(float dt) {
    (void)dt;
    for (int j = 0; j < 4; j++) {
        int n = g.mercActor[j] - 1;
        if (n < 0 || n >= g.actorCount)
            continue;
        Actor *a = &g.actors[n];
        if (g.time >= g.mercUntil[j] || a->dead) {
            g.mercActor[j] = 0;
            g.mercUntil[j] = 0;
            log_msg(a->dead ? "A hired guard has died." : "Guard contract expired. Returning home.");
        } else if (a->target < 0 && conscious(a)) {
            if (a->bleed > 0 && inventory_remove(a, MEDKIT, 1))
                a->bleed = 0;
            for (int i = 0; i < g.actorCount; i++) {
                Actor *ally = &g.actors[i];
                if (ally->squad && !ally->dead && ally->bleed > 0 && !actor_inside(ally) &&
                    distance(a->x, a->y, ally->x, ally->y) < 2.5f && inventory_remove(a, MEDKIT, 1)) {
                    ally->bleed = 0;
                    gain(a, MEDICINE, 8);
                    log_msg("Hired guard bandaged %s.", ally->name);
                    break;
                }
            }
        }
    }
}

int nearby_service(int role) {
    Actor *a = &g.actors[g.selected];
    if (!conscious(a) || actor_inside(a))
        return -1;
    int best = -1;
    float d = TALK_REACH;
    for (int i = 0; i < g.actorCount; i++) {
        Actor *b = &g.actors[i];
        float dd = distance(a->x, a->y, b->x, b->y);
        if (!b->squad && conscious(b) && !actor_inside(b) && npc_role(b) == role && dd <= d &&
            g.rep[b->faction] >= -25) {
            best = i;
            d = dd;
        }
    }
    return best;
}
int loot_take(int slot, int all) {
    Actor *a = &g.actors[g.selected];
    if (ui.lootActor < 0 || ui.lootActor >= g.actorCount || slot < 0 || slot >= PACK + GEARS)
        return 0;
    Actor *b = &g.actors[ui.lootActor];
    if (!conscious(a) || (!b->dead && b->unconscious <= 0) || depth_distance(a, b) > TALK_REACH) {
        log_msg("Salvage needs a fallen target within reach.");
        return 0;
    }
    int it = slot < PACK ? b->pack[slot].item : b->gear[slot - PACK];
    int qty = slot < PACK && all ? b->pack[slot].qty : 1;
    int owner = slot < PACK ? b->stolen[slot] : b->gearStolen[slot - PACK];
    int theft = !b->squad && b->faction != HOSTILE && b->faction != WILDLIFE;
    if (theft)
        owner = b->faction;
    if (!it || !inventory_add_owned(a, it, qty, owner)) {
        log_msg("No item, or no room for that stack.");
        return 0;
    }
    if (slot < PACK)
        inventory_remove_slot(b, slot, qty);
    else {
        b->gear[slot - PACK] = 0;
        b->gearStolen[slot - PACK] = 0;
    }
    if (theft) {
        for (int i = 0; i < g.actorCount; i++) {
            Actor *w = &g.actors[i];
            if (!w->squad && w->faction == b->faction && conscious(w) &&
                depth_distance(a, w) < (a->sneaking ? 3 : 6)) {
                a->bounty += 20;
                g.rep[b->faction] -= 4;
                log_msg("Witnessed looting! Bounty +20; stolen goods stay marked.");
                return 1;
            }
        }
    }
    log_msg("Took %d %s%s.", qty, itemDefs[it].name, owner ? " (stolen)" : "");
    return 1;
}
const char *skill_training(int s) {
    static const char *text[SKILLS] = {
        "Travel on foot. Higher athletics increases walking speed.",
        "Carry weight and mine. Strength raises load allowance and handles heavy weapons.",
        "Land weapon hits. Dexterity improves attack timing.",
        "Fight with melee weapons. Practice improves your chance to hit.",
        "Face melee attacks. Defence improves blocks; heavy packs penalize it.",
        "Survive blows. Each toughness level reduces incoming damage by 0.2%, up to 20%.",
        "Evade attacks while unarmed. Dodge replaces weapon defence when your weapon slot is empty.",
        "Fight with an empty weapon slot. Martial arts improves unarmed attacks.",
        "Walk while sneaking. Stealth reduces detection; bulky gear penalizes it.",
        "Pick pockets and steal salvage. Failure can trigger bounties and guards.",
        "Open locked salvage with tools. Failed attempts still teach you.",
        "Treat injuries with field dressings. Medicine restores more limb health.",
        "Mine ore and gather supplies. Work completes only within reach.",
        "Water and harvest crops. Every 30 farming levels adds one grain per harvest.",
        "Cook at a campfire. Every 35 cooking levels adds one ration per batch.",
        "Smelt and forge. Smithing unlocks higher equipment quality.",
        "Build and make prosthetics. Every 30 engineering levels saves one timber per building.",
        "Research at a bench. Every 5 science levels reduces research silver cost by 1%.",
        "Land ranged shots. Crossbows improves accuracy; bring matching ammunition.",
        "Discover settlements and finish contracts. Perception improves ranged accuracy."};
    return s >= 0 && s < SKILLS ? text[s] : "";
}
int needs_aid(const Actor *a) {
    if (a->dead)
        return 0;
    if (a->bleed > .1f)
        return 1;
    for (int i = 0; i < LIMBS; i++)
        if (a->health[i] < 60 && depth_can_heal(a, i))
            return 1;
    return 0;
}
const char *job_status(const Actor *a) {
    if ((uintptr_t)a >= (uintptr_t)g.actors && (uintptr_t)a < (uintptr_t)(g.actors + MAX_ACTORS)) {
        int i = a - g.actors;
        if (g.depth.actors[i].captive)
            return "CAPTIVE - escape or send a rescuer";
        if (g.depth.actors[i].paused)
            return "PAUSED - resume queue in Orders";
    }
    if (a->dead)
        return "DEAD - recover equipment or dismiss";
    if (a->prisonTime > 0)
        return "IN CUSTODY - settle bail or escape";
    if (a->unconscious > 0)
        return "UNCONSCIOUS - needs rescue";
    if (actor_inside(a))
        return "INDOORS - jobs resume outside";
    if (a->target >= 0 && a->autoattack)
        return "FIGHTING - work waits for safety";
    if (a->taskKind == 3)
        return "RESTING - move to wake";
    if (a->taskKind)
        return "WORKING - moving cancels this task";
    if (a->job == JOB_HOLD)
        return "HOLDING - defends if combat stance is on";
    if (a->job == JOB_FOLLOW)
        return a == &g.actors[g.selected] ? "LEADING - direct movement" : "FOLLOWING this group leader";
    if (a->job == JOB_MEDIC) {
        if (!inventory_count(a, MEDKIT))
            return "BLOCKED - no field dressings";
        for (int j = 0; j < g.actorCount; j++) {
            Actor *b = &g.actors[j];
            if (b->squad && !b->dead && needs_aid(b) && actor_inside(a) == actor_inside(b))
                return "SEEKING injured squad member";
        }
        return "READY - watching for serious injuries";
    }
    if (a->job == JOB_MINE) {
        for (int i = 0; i < g.nodeCount; i++)
            if (g.nodes[i].type <= NODE_COPPER && g.nodes[i].remaining &&
                distance(a->x, a->y, g.nodes[i].x, g.nodes[i].y) < 12) {
                Actor copy = *a;
                return inventory_add(&copy, g.nodes[i].type == NODE_ORE ? ORE : COPPER, 1)
                           ? "SEEKING ore within 12 paces"
                           : "BLOCKED - pack full; unload ore";
            }
        return "BLOCKED - no ore within 12 paces";
    }
    int type = a->job == JOB_FARM    ? FARM
               : a->job == JOB_SMELT ? FORGE
               : a->job == JOB_COOK  ? CAMPFIRE
                                     : STORE;
    int found = 0;
    for (int i = 0; i < g.buildingCount; i++)
        if (g.buildings[i].type == type && distance(a->x, a->y, g.buildings[i].x, g.buildings[i].y) < 15)
            found = 1;
    if (!found)
        return "BLOCKED - no required station within 15 paces";
    if (a->job == JOB_FARM)
        return inventory_count(a, WATER) ? "TENDING nearby farm; waiting for harvest"
                                         : "NEEDS WATER unless rain irrigates the field";
    if (a->job == JOB_HAUL)
        return "HAULING raw materials; supplies stay in pack";
    if (a->job == JOB_SMELT && inventory_count(a, ORE) < 3)
        return "FETCHING 3 ore; stock the nearby depot";
    if (a->job == JOB_COOK && inventory_count(a, WATER) < 1)
        return "FETCHING water; stock the nearby depot";
    if (a->job == JOB_COOK && inventory_count(a, GRAIN) < 3 && inventory_count(a, MEAT) < 2)
        return "FETCHING grain or meat; stock the depot";
    return "WORKING at station; unload finished goods";
}
