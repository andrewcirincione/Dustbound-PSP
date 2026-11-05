#include "game.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int add_actor(const char *, int, int, float, float, int);
extern int add_build(float, float, int, int, int);
extern void add_node(float, float, int, int, int, int);
extern int conscious(const Actor *);
extern int nearby_station(int, float);
extern void restock(void);
static unsigned wrand(void) {
    unsigned x = g.rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return g.rng = x ? x : 17;
}
static unsigned whash(int x, int y) {
    unsigned v = (unsigned)x * 374761393u + (unsigned)y * 668265263u + 73819;
    v = (v ^ (v >> 13)) * 1274126177u;
    return v ^ (v >> 16);
}
static float limit(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }
int biome(float x, float y) {
    if (world_is_expanded()) {
        x = (x + 78) / 4;
        y = (y + 81) / 4;
    }
    if (x < 128 && y < 128) {
        if (x > 74 && y > 62)
            return 3;
        if (y > 69)
            return 2;
        if (x > 58 && y < 49)
            return 1;
        if (distance(x, y, 61, 62) < 16)
            return 4;
        return 0;
    }
    if (y > 285 && x > 240)
        return 6;
    if (y > 280 && x < 100)
        return 7;
    if (x > 280 && y < 175)
        return y < 80 ? 4 : 8;
    if (y > 180 && x < 110)
        return 3;
    if (y > 250 && x < 230)
        return 2;
    if (x > 205)
        return 5;
    if (y > 110 && y < 215)
        return 8;
    return 1;
}
int terrain_road(float x, float y) {
    /* A sparse road graph: each town joins the nearest earlier settlement. */
    static int neighbors[TOWNS], ready;
    if (!ready) {
        for (int t = 1; t < TOWNS; t++) {
            int near = 0;
            float best = 1e10f;
            for (int j = 0; j < t; j++) {
                float dx = towns[t].x - towns[j].x, dy = towns[t].y - towns[j].y, d = dx * dx + dy * dy;
                if (d < best) {
                    best = d;
                    near = j;
                }
            }
            neighbors[t] = near;
        }
        ready = 1;
    }
    for (int t = 1; t < TOWNS; t++) {
        int near = neighbors[t];
        float ax = towns[t].x, ay = towns[t].y, dx = towns[near].x - ax, dy = towns[near].y - ay;
        float u = ((x - ax) * dx + (y - ay) * dy) / (dx * dx + dy * dy);
        if (u >= 0 && u <= 1) {
            float xx = x - ax - u * dx, yy = y - ay - u * dy;
            if (xx * xx + yy * yy < 1.7f)
                return 1;
        }
    }
    return 0;
}
int terrain_height(float x, float y) {
    if (town_near(x, y, 12) >= 0)
        return 0;
    int b = biome(x, y);
    if (b == 0 || b == 2 || b == 3)
        return 0;
    int ix = (int)x / 24, iy = (int)y / 24;
    float u = (x - ix * 24) / 24, v = (y - iy * 24) / 24;
    float a = (whash(ix, iy) % 13) * (1 - u) + (whash(ix + 1, iy) % 13) * u;
    float c = (whash(ix, iy + 1) % 13) * (1 - u) + (whash(ix + 1, iy + 1) % 13) * u;
    return (int)((a * (1 - v) + c * v) * (b == 5 || b == 8 ? 1.6f : .7f));
}
int terrain_water(float x, float y) {
    float wx = x, wy = y;
    if (world_is_expanded()) {
        x = (x + 78) / 4;
        y = (y + 81) / 4;
    }
    if (x < 128 && y < 128)
        return 0;
    float r = 194 + 12 * sinf(y * .025f), s = 139 + 9 * sinf(x * .025f);
    int wet = fabsf(x - r) < 2.2f || (x < 210 && fabsf(y - s) < 1.6f) || distance(x, y, 107, 242) < 12;
    if (!wet)
        return 0;
    if (town_near(wx, wy, 12) >= 0 || terrain_road(wx, wy) || terrain_road(wx + 1, wy) ||
        terrain_road(wx - 1, wy))
        return 0;
    return 1;
}
void route_toward(Actor *a, float x, float y, float dt) {
    if (!conscious(a) || a->prisonTime > 0)
        return;
    float dx = x - a->x, dy = y - a->y, d = sqrtf(dx * dx + dy * dy);
    if (d < .3f)
        return;
    int clear = 1;
    for (int j = 1; j <= 4; j++) {
        float k = fminf(d, 3) * j * .25f / d;
        if (!can_walk(a->x + dx * k, a->y + dy * k)) {
            clear = 0;
            break;
        }
    }
    if (clear) {
        a->routeLen = 0;
        move_actor(a, dx, dy, dt);
        return;
    }
    a->routeClock -= dt;
    if (a->routePos >= a->routeLen || a->routeClock <= 0 ||
        distance(x, y, a->routeTargetX, a->routeTargetY) > 2) {
        enum { N = 33, C = N * N };
        static short parent[C], queue[C], back[C];
        int ox = (int)a->x - N / 2, oy = (int)a->y - N / 2, st = N / 2 * N + N / 2;
        for (int i = 0; i < C; i++)
            parent[i] = -2;
        int head = 0, tail = 0, goal = st;
        float best = 1e10f;
        queue[tail++] = st;
        parent[st] = -1;
        while (head < tail) {
            int n = queue[head++], xx = n % N, yy = n / N;
            float ex = ox + xx + .5f - x, ey = oy + yy + .5f - y, score = ex * ex + ey * ey;
            if (score < best) {
                best = score;
                goal = n;
            }
            if (score < .7f)
                break;
            const int dd[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
            for (int j = 0; j < 4; j++) {
                int nx = xx + dd[j][0], ny = yy + dd[j][1], z = ny * N + nx;
                if (nx < 0 || ny < 0 || nx >= N || ny >= N || parent[z] != -2)
                    continue;
                if (!can_walk(ox + nx + .5f, oy + ny + .5f))
                    continue;
                parent[z] = n;
                queue[tail++] = z;
            }
        }
        int count = 0;
        for (int z = goal; z != st && z >= 0 && count < C; z = parent[z])
            back[count++] = z;
        a->routeLen = count > 24 ? 24 : count;
        a->routePos = 0;
        for (int i = 0; i < a->routeLen; i++) {
            int z = back[count - 1 - i];
            a->routeX[i] = ox + z % N;
            a->routeY[i] = oy + z / N;
        }
        a->routeClock = 7;
        a->routeTargetX = x;
        a->routeTargetY = y;
    }
    if (a->routePos < a->routeLen) {
        float xx = a->routeX[a->routePos] + .5f, yy = a->routeY[a->routePos] + .5f;
        if (distance(a->x, a->y, xx, yy) < .35f)
            a->routePos++;
        else
            move_actor(a, xx - a->x, yy - a->y, dt);
    }
}
static void outfit(Actor *a, int n) {
    if (a->kind == BEAST)
        return;
    int f = a->faction, m = (n + f) % MATERIALS, q = a->kind == GUARD ? 2 : n % 2;
    a->gear[0] = gear_id(a->kind == TRADER   ? 3
                         : a->kind == RAIDER ? (n % 24)
                         : f == SHOGUN       ? (n % 7)
                         : f == MONKS        ? 16
                                             : n % 32,
                         m, q);
    a->gear[1] = gear_id(32 + (n + f * 3) % 16, m, q);
    a->gear[2] = gear_id(48 + (n + f) % 12, m, q);
    a->gear[3] = gear_id(60 + n % 8, m, q);
    a->gear[4] = gear_id(68 + n % 8, m, q);
    if (n % 3 == 0)
        a->gear[5] = gear_id(76 + n % 8, m, q);
    if (weapon_ranged(a->gear[0]))
        inventory_add(a, gear_family(a->gear[0]) >= 29 ? CARTRIDGE : BOLTS, 24);
}
void init_expansion(void) {
    for (int t = 0; t < TOWNS; t++) {
        g.townOwner[t] = towns[t].faction;
        g.prosperity[t] = 60;
    }
    g.weatherTimer = 160;
    g.eventTimer = 220;
    g.difficulty = 1;
    snprintf(g.worldNews, sizeof(g.worldNews), "The old road is open. Work waits in the frontier towns.");
    for (int t = 0; t < TOWNS; t++) {
        float x = towns[t].x, y = towns[t].y;
        int f = towns[t].faction;
        if (t >= 5) {
            add_build(x - 4, y - 3, HOUSE, f, -1);
            add_build(x, y - 5, TOWER, f, -1);
            add_build(x + 4, y - 3, SHOP, f, -1);
            add_build(x - 4, y + 3, BAR, f, -1);
            add_build(x + 4, y + 3, HOUSE, f, -1);
            add_build(x + 7, y, WINDMILL, f, -1);
            add_build(x, y + 1, WELL, f, -1);
            add_actor("Gate warden", GUARD, f, x + 2, y - 1, 14 + t);
            add_actor("Road marshal", GUARD, f, x - 1, y + 4, 13 + t);
            add_actor(f == HOSTILE ? "Vulture quarter" : "Market keeper", TRADER, f, x + 2, y - 1.5f, 8 + t);
            static const char *recruits[] = {"KASUMI", "FLINT",  "HANA", "COLE", "REN", "SABLE",
                                             "JIN",    "MORROW", "YUKI", "ADA",  "ROOK"};
            add_actor(recruits[t - 5], RECRUIT, f, x - 2, y + 3.5f, 5 + t / 2);
            add_actor("Inn keeper", SETTLER, f, x - 1, y + 6, 4);
            add_actor("Caravan scout", CARAVAN, f, x + 5, y + 6, 12);
        }
        if (f != HOSTILE) {
            add_build(x - 7, y + 5, BED, f, -1);
            add_build(x + 8, y + 5, FORGE, f, -1);
            add_build(x + 8, y + 9, BENCH, f, -1);
            if (t < 5)
                add_actor("Inn keeper", SETTLER, f, x - 1, y + 6, 5);
        }
        add_node(x + 5, y + 6, NODE_LOCKED, f, gear_id(32 + t % 16, t % 6, (t / 4) % 4), 1);
    }
    for (int n = 1; n < g.actorCount; n++)
        if (!g.actors[n].squad)
            outfit(&g.actors[n], n);
    for (int n = 1; n < g.actorCount; n++) {
        Actor *a = &g.actors[n];
        if (a->squad || a->faction == HOSTILE)
            continue;
        int t = town_near(a->x, a->y, 14);
        if (t < 0)
            continue;
        if (!strcmp(a->name, "Gate guard") || !strcmp(a->name, "Gate warden")) {
            a->kind = SETTLER;
            a->voice = ROLE_MEDIC;
            snprintf(a->name, 16, "Town medic");
        } else if (a->kind == CARAVAN) {
            a->kind = SETTLER;
            a->voice = ROLE_SMITH;
            snprintf(a->name, 16, "Frontier smith");
            a->x = a->homeX = towns[t].x + 7;
            a->y = a->homeY = towns[t].y + 7;
        } else if (!strcmp(a->name, "Wayfarer")) {
            a->voice = ROLE_BARBER;
            snprintf(a->name, 16, "Trail barber");
        }
    }

    for (int i = 0; i < 370; i++) {
        float x = 24 + wrand() % 350, y = 25 + wrand() % 350;
        if (town_near(x, y, 11) >= 0 || !can_walk(x, y))
            continue;
        int type = i % 11 == 0 ? NODE_RUIN : i % 3;
        add_node(x, y, type, 0,
                 type == NODE_RUIN ? (i % 2 ? RELIC : gear_id(i % FAMILIES, (i / 7) % 6, i % 4)) : 0,
                 type == NODE_RUIN ? 1 : 0);
    }
    for (int i = 0; i < 44 && g.actorCount < MAX_ACTORS - 8; i++) {
        float x = 24 + wrand() % 348, y = 25 + wrand() % 348;
        if (i < 30) {
            int t = 1 + i / 2, near = 0;
            float best = 1e9f;
            for (int j = 0; j < t; j++) {
                float d = distance(towns[t].x, towns[t].y, towns[j].x, towns[j].y);
                if (d < best) {
                    best = d;
                    near = j;
                }
            }
            float u = i % 2 ? .7f : .35f;
            x = towns[t].x * (1 - u) + towns[near].x * u;
            y = towns[t].y * (1 - u) + towns[near].y * u + (i % 2 ? 1.0f : -1.0f);
        }
        if (town_near(x, y, 15) >= 0 || !can_walk(x, y))
            continue;
        int b = biome(x, y), beast = i % 4 == 0;
        static const char *foes[] = {"Dust jackal",   "Salt raider", "Red cutthroat", "Ash revenant",
                                     "Iron deserter", "Bone hound",  "Pine stalker",  "Dune prowler"};
        int n = add_actor(foes[(i + b) % 8], beast ? BEAST : RAIDER, beast ? WILDLIFE : HOSTILE, x, y,
                          6 + b * 2 + i % 12);
        if (n >= 0) {
            outfit(&g.actors[n], i);
            if (!beast && i % 9 == 0) {
                g.actors[n].bountyReward = 250 + i * 10;
                inventory_add(&g.actors[n], RELIC, 1);
            }
        }
    }
    for (int n = 0; n < g.actorCount; n++)
        if (g.actors[n].squad) {
            g.actors[n].gear[4] = gear_id(68, 0, 0);
            g.actors[n].gear[5] = gear_id(76, 0, 0);
        }
    restock();
}
int item_price(int town, int it, int selling) {
    if (it <= 0 || it >= ITEMS || town < 0 || town >= TOWNS)
        return 0;
    float mod = 1 + (60 - g.prosperity[town]) * .004f - g.rep[g.townOwner[town]] * .002f;
    mod = limit(mod, .65f, 1.6f);
    return (int)fmaxf(1, itemDefs[it].price * (selling ? .6f : 1) * mod);
}
int shop_item(int index) {
    if (ui.tradeTown < 0 || ui.tradeTown >= TOWNS)
        return NONE;
    for (int i = 1; i < ITEMS; i++)
        if (g.stock[ui.tradeTown][i] > 0) {
            int slot = itemDefs[i].slot;
            if (ui.shopFilter == 1 && slot >= 0)
                continue;
            if (ui.shopFilter == 2 && slot != 0)
                continue;
            if (ui.shopFilter == 3 && slot < 1)
                continue;
            if (index-- == 0)
                return i;
        }
    return NONE;
}
int shop_count(void) {
    int n = 0;
    if (ui.tradeTown < 0 || ui.tradeTown >= TOWNS)
        return 0;
    for (int i = 1; i < ITEMS; i++)
        if (g.stock[ui.tradeTown][i] > 0) {
            int s = itemDefs[i].slot;
            if (ui.shopFilter == 1 && s >= 0)
                continue;
            if (ui.shopFilter == 2 && s != 0)
                continue;
            if (ui.shopFilter == 3 && s < 1)
                continue;
            n++;
        }
    return n;
}
int nearby_workstation(int type) { return nearby_station(type, 6); }
int forge_equipment(int it) {
    Actor *a = &g.actors[g.selected];
    if (it < BASE_ITEMS || it >= ITEMS)
        return 0;
    ItemDef *d = &itemDefs[it];
    if (nearby_workstation(FORGE) < 0) {
        log_msg("Stand within six paces of a forge.");
        return 0;
    }
    int tech = d->material >= 4 ? 3 : d->material >= 2 ? 2 : 1, skill = d->quality * 12;
    if (g.tech < tech || level(a, SMITHING) < skill) {
        log_msg("Requires research %d and smithing %d.", tech, skill);
        return 0;
    }
    int metal = 2 + d->material + d->quality * 2, cloth = d->slot ? 2 + d->quality : 0,
        wood = d->slot == 0 ? 1 : 0;
    int relic = d->material == 5 ? 1 : 0, coal = d->quality >= 2 ? 1 : 0;
    if (inventory_count(a, IRONBAR) < metal || inventory_count(a, CLOTH) < cloth ||
        inventory_count(a, WOOD) < wood || inventory_count(a, RELIC) < relic ||
        inventory_count(a, COAL) < coal) {
        log_msg("Need %d iron, %d cloth, %d wood, %d relic, %d coal.", metal, cloth, wood, relic, coal);
        return 0;
    }
    Actor backup = *a;
    inventory_remove(a, IRONBAR, metal);
    if (cloth)
        inventory_remove(a, CLOTH, cloth);
    if (wood)
        inventory_remove(a, WOOD, wood);
    if (relic)
        inventory_remove(a, RELIC, relic);
    if (coal)
        inventory_remove(a, COAL, coal);
    if (!inventory_add(a, it, 1)) {
        *a = backup;
        log_msg("Pack full. Forging cancelled.");
        return 0;
    }
    gain(a, SMITHING, 45 + d->quality * 20);
    log_msg("Forged %s.", d->name);
    return 1;
}
int storage_transfer(int b, int from, int slot) {
    if (b < 0 || b >= g.buildingCount || g.buildings[b].type != STORE)
        return 0;
    Actor *a = &g.actors[g.selected];
    Building *z = &g.buildings[b];
    if (distance(a->x, a->y, z->x, z->y) > 6)
        return 0;
    if (from) {
        if (slot < 0 || slot >= 16 || !z->storage[slot].item)
            return 0;
        int it = z->storage[slot].item;
        if (!inventory_add(a, it, 1))
            return 0;
        if (--z->storage[slot].qty == 0)
            z->storage[slot].item = 0;
        log_msg("Withdrew %s.", itemDefs[it].name);
        return 1;
    }
    if (slot < 0 || slot >= PACK || !a->pack[slot].item)
        return 0;
    int dest = -1, it = a->pack[slot].item;
    for (int i = 0; i < 16; i++)
        if (z->storage[i].item == it && z->storage[i].qty < 999) {
            dest = i;
            break;
        }
    if (dest < 0)
        for (int i = 0; i < 16; i++)
            if (!z->storage[i].item) {
                dest = i;
                break;
            }
    if (dest < 0) {
        log_msg("Depot has no free stack.");
        return 0;
    }
    if (a->stolen[slot]) {
        log_msg("Fence stolen goods before using a public depot.");
        return 0;
    }
    z->storage[dest].item = it;
    z->storage[dest].qty++;
    inventory_remove(a, it, 1);
    log_msg("Stored %s.", itemDefs[it].name);
    return 1;
}
int start_sleep(int town) {
    Actor *a = &g.actors[g.selected];
    int bed = nearby_station(BED, 12);
    if (bed < 0) {
        log_msg("Find an inn bed or build a shelter.");
        return 0;
    }
    if (g.buildings[bed].owner < 0) {
        if (town < 0 || g.money < 25) {
            log_msg("An inn bed costs 25 silver.");
            return 0;
        }
        g.money -= 25;
    }
    a->taskKind = 3;
    a->taskIndex = bed;
    a->action = 180;
    a->autoattack = 0;
    ui.screen = SCREEN_WORLD;
    log_msg("Sleeping. Move to wake; bleeding must be bandaged.");
    return 1;
}
int doctor_heal(int town) {
    Actor *a = &g.actors[g.selected];
    if (town < 0 || town_near(a->x, a->y, 14) != town || g.money < 80) {
        log_msg("Town treatment costs 80 silver.");
        return 0;
    }
    g.money -= 80;
    a->bleed = 0;
    a->blood = 100;
    a->exposure = 0;
    for (int i = 0; i < LIMBS; i++)
        if (!(a->lostMask & (1 << i)) || (a->prostheticMask & (1 << i)))
            a->health[i] = 100;
    a->unconscious = 0;
    log_msg("Wounds treated. Lost limbs still need prosthetics.");
    return 1;
}
int accept_contract(int town, int kind) {
    if (town < 0 || town >= TOWNS || kind < 0 || kind > 3)
        return 0;
    for (int i = 0; i < CONTRACTS; i++)
        if (g.contracts[i].active == 1 && g.contracts[i].town == town && g.contracts[i].type == kind) {
            log_msg("That contract is already in your journal.");
            return 0;
        }
    int n = -1;
    for (int i = 0; i < CONTRACTS; i++)
        if (g.contracts[i].active != 1) {
            n = i;
            break;
        }
    if (n < 0) {
        log_msg("Journal full. Finish an active contract.");
        return 0;
    }
    Contract *c = &g.contracts[n];
    memset(c, 0, sizeof(*c));
    c->active = 1;
    c->type = kind;
    c->town = town;
    c->target = (town + 1 + g.day % 3) % TOWNS;
    c->item = kind == 1 ? ORE : kind == 3 ? RELIC : NONE;
    c->needed = kind == 1 ? 8 : kind == 2 ? 3 : 1;
    c->reward = kind == 0 ? 220 : kind == 1 ? 240 : kind == 2 ? 420 : 550;
    c->baseline = g.kills;
    log_msg("Contract accepted. Journal tracks the objective.");
    return 1;
}
void contract_description(int n, char *out, int size) {
    if (n < 0 || n >= CONTRACTS || !g.contracts[n].active) {
        snprintf(out, size, "No contract. Speak to town residents for work.");
        return;
    }
    Contract *c = &g.contracts[n];
    if (c->type == 0)
        snprintf(out, size, "Carry dispatch to %s; return to %s. %d silver.", towns[c->target].name,
                 towns[c->town].name, c->reward);
    else if (c->type == 2)
        snprintf(out, size, "Defeat %d hostiles (%d/%d). Report to %s. %d silver.", c->needed, c->progress,
                 c->needed, towns[c->town].name, c->reward);
    else
        snprintf(out, size, "Bring %d %s to %s. Reward %d silver and standing.", c->needed,
                 itemDefs[c->item].name, towns[c->town].name, c->reward);
}
int complete_contract(int n) {
    if (n < 0 || n >= CONTRACTS || g.contracts[n].active != 1)
        return 0;
    Contract *c = &g.contracts[n];
    Actor *a = &g.actors[g.selected];
    if (town_near(a->x, a->y, 14) != c->town) {
        log_msg("Claim at %s.", towns[c->town].name);
        return 0;
    }
    if (c->item) {
        if (inventory_count(a, c->item) < c->needed) {
            log_msg("The required supplies are missing.");
            return 0;
        }
        inventory_remove(a, c->item, c->needed);
    } else if (c->progress < c->needed) {
        log_msg("Contract objective is incomplete.");
        return 0;
    }
    c->completed = 1;
    c->active = 2;
    g.money += c->reward;
    g.contractsDone++;
    g.rep[g.townOwner[c->town]] += 8;
    g.prosperity[c->town] = (int)limit(g.prosperity[c->town] + 5, 0, 100);
    gain(a, PERCEPTION, 70);
    log_msg("Contract paid: %d silver; faction standing +8.", c->reward);
    return 1;
}
const char *dialogue_line(int n, int page) {
    if (n < 0 || n >= g.actorCount)
        return "The road is quiet.";
    Actor *a = &g.actors[n];
    if (a->prisonTime > 0)
        return "The marshal has your name. Pay the bail, wait out your sentence, or try the lock. A failed "
               "escape will cost you.";
    if (page == 1)
        return "Work keeps this town alive. Carry a dispatch, bring ore, clear the roads, or recover a "
               "relic. Read the terms in your journal.";
    if (page == 2)
        return g.worldNews;
    switch (npc_role(a)) {
    case ROLE_MERCHANT:
        return "I sell supplies and equipment. Bring salvage to trade. The innkeeper has beds; the town "
               "medic treats wounds.";
    case ROLE_INNKEEPER:
        return "A bed costs 25 silver. Step inside the inn to rest. Patch bleeding wounds before you sleep.";
    case ROLE_MEDIC:
        return "Eighty silver for wound care. I can stop bleeding and restore health, but lost limbs need "
               "mechanical replacements.";
    case ROLE_SMITH:
        return "The forge is here. Bring iron, cloth and fuel. Your smithing skill and research determine "
               "what you can make.";
    case ROLE_BARBER:
        return "Sit down and tell me how you want to look. I can change hair, beard and appearance without "
               "changing your skills.";
    case ROLE_RECRUIT:
        return "Three hundred silver buys my supplies and my sword. We share the road, the work and the "
               "danger.";
    case ROLE_MARSHAL:
        return "The board has paying work. I also handle bounties. The roads between settlements are long; "
               "buy food before leaving.";
    case ROLE_SCOUT:
        return "Mark a destination on your atlas. Watch for raiders between towns, and keep enough food for "
               "the return trip.";
    }
    static const char *factionLines[FACTIONS] = {
        "No lord owns the road. Find honest work if you can, and keep one hand near your blade.",
        "The Compact keeps the wells open. Do your share, pay your debts, and our gates will shelter you.",
        "Everything has a second life. Iron, coats, people. Have a look at the stock; the caravans bring "
        "different pieces every dawn.",
        "This is Vulture ground. The weak pay in silver. The foolish pay in blood.",
        "A sword is a promise. The Shogunate remembers those who keep it, and those who break it. Mind your "
        "conduct in our streets.",
        "Sit in the shade, traveler. The ash takes everyone in time. Until then, there is work, medicine, "
        "and a little kindness.",
        "Nobody asks where a thing came from in Black Mesa. Keep your voice low and your business "
        "profitable.",
        "A low growl rises from the dust."};
    return factionLines[a->faction % FACTIONS];
}
const char *ambient_line(int n) {
    static const char *lines[24] = {"Storm's coming. Cover your eyes.",
                                    "Fresh iron. Fair prices.",
                                    "Keep your blade sheathed here.",
                                    "The old wells are running dry.",
                                    "Need another sword on the road?",
                                    "Nobody crosses the scar alone.",
                                    "Heard gunfire out by the ford.",
                                    "A bowl of rice beats a grave.",
                                    "The Shogun taxes even the dead.",
                                    "Check your straps before dusk.",
                                    "Black Mesa never asks questions.",
                                    "My brother never came home.",
                                    "Work on the board, if you need it.",
                                    "These plates stopped an arrow once.",
                                    "Rain smells like rust again.",
                                    "No credit. Not after last winter.",
                                    "Keep moving, drifter.",
                                    "The monks still take the wounded.",
                                    "A sharp blade won't fill your belly.",
                                    "Someone's been opening our crates.",
                                    "They say the north is white as bone.",
                                    "Don't sleep in the open tonight.",
                                    "One more caravan, then I'm done.",
                                    "The frontier remembers."};
    if (n < 0 || n >= g.actorCount)
        return "";
    return lines[g.actors[n].speech % 24];
}
void dialogue_open(int n) {
    ui.dialogActor = n;
    ui.dialogPage = 0;
    ui.selection = 0;
    ui.screen = SCREEN_DIALOG;
    if (n >= 0 && n < g.actorCount) {
        g.actors[n].speechTime = 5;
        g.actors[n].speech = (n + g.day) % 24;
    }
}
int pickpocket(int n) {
    Actor *a = &g.actors[g.selected];
    if (n < 0 || n >= g.actorCount || !a->sneaking)
        return 0;
    Actor *b = &g.actors[n];
    if (b->squad || b->dead || b->kind == BEAST || distance(a->x, a->y, b->x, b->y) > 3)
        return 0;
    gain(a, THEFT, 22);
    float chance = .25f + level(a, THEFT) * .007f + level(a, STEALTH) * .003f + gear_bonus(a, 2) * .003f;
    if ((wrand() % 1000) / 1000.f > fminf(.9f, chance)) {
        a->bounty += 80;
        g.rep[b->faction] -= 18;
        b->speech = 19;
        b->speechTime = 5;
        log_msg("Caught stealing! Bounty +80; faction standing -18.");
        return 0;
    }
    int it = NONE;
    for (int i = 0; i < PACK; i++)
        if (b->pack[i].item) {
            it = b->pack[i].item;
            break;
        }
    if (!it) {
        log_msg("Their pockets are empty.");
        return 0;
    }
    if (!inventory_add(a, it, 1)) {
        log_msg("Your pack is full.");
        return 0;
    }
    inventory_remove(b, it, 1);
    for (int i = 0; i < PACK; i++)
        if (a->pack[i].item == it)
            a->stolen[i] = b->faction;
    log_msg("Lifted %s. Find a fence before selling it back.", itemDefs[it].name);
    return 1;
}
static void worker_production(Actor *a, float dt) {
    if (actor_inside(a) || a->job < JOB_SMELT || a->taskKind || !conscious(a))
        return;
    a->action -= dt;
    if (a->action > 0)
        return;
    a->action = 5;
    int depot = -1;
    for (int j = 0; j < g.buildingCount; j++)
        if (g.buildings[j].type == STORE && distance(a->x, a->y, g.buildings[j].x, g.buildings[j].y) < 12) {
            depot = j;
            break;
        }
    int old = g.selected;
    g.selected = (int)(a - g.actors);
    if (a->job == JOB_HAUL && depot >= 0) {
        for (int i = 0; i < PACK; i++)
            if (a->pack[i].item == ORE || a->pack[i].item == COPPER || a->pack[i].item == WOOD ||
                a->pack[i].item == GRAIN || a->pack[i].item == IRONBAR) {
                if (distance(a->x, a->y, g.buildings[depot].x, g.buildings[depot].y) > 5)
                    route_toward(a, g.buildings[depot].x, g.buildings[depot].y, 1);
                else
                    storage_transfer(depot, 0, i);
                break;
            }
    } else if (a->job == JOB_SMELT || a->job == JOB_COOK) {
        int it = a->job == JOB_SMELT ? ORE : GRAIN, need = 3;
        if (depot >= 0 && distance(a->x, a->y, g.buildings[depot].x, g.buildings[depot].y) < 6) {
            for (int j = 0; j < 16; j++)
                if (g.buildings[depot].storage[j].item == it)
                    while (inventory_count(a, it) < need && g.buildings[depot].storage[j].qty > 0)
                        if (!storage_transfer(depot, 1, j))
                            break;
        }
        if (inventory_count(a, it) >= need)
            craft_item(a->job == JOB_SMELT ? 0 : 1);
    }
    g.selected = old;
}
void tick_expansion(float dt) {
    g.weatherTimer -= dt;
    g.eventTimer -= dt;
    g.season = (g.day / 7) % 4;
    if (g.weatherTimer <= 0) {
        g.weatherTimer = 160 + wrand() % 160;
        int b = biome(g.actors[g.selected].x, g.actors[g.selected].y);
        int r = wrand() % 5;
        g.weather = r < 2 ? 0 : b == 6 ? 4 : b == 3 ? 3 : r == 2 ? 2 : 1;
        snprintf(g.worldNews, sizeof(g.worldNews), "%s over the borderlands. Watch your supplies.",
                 weatherNames[g.weather]);
    }
    if (g.eventTimer <= 0) {
        g.eventTimer = 220;
        int t = wrand() % TOWNS, k = wrand() % 4;
        g.townState[t] = k;
        g.prosperity[t] = (int)limit(g.prosperity[t] + (k == 0   ? 12
                                                        : k == 1 ? -12
                                                        : k == 2 ? 5
                                                                 : -18),
                                     15, 95);
        static const char *news[] = {"A caravan arrived in", "Drought is tightening prices in",
                                     "Farmers report rain near", "Border raiders struck"};
        snprintf(g.worldNews, sizeof(g.worldNews), "%s %s.", news[k], towns[t].name);
        if (k == 3) {
            g.factionWar[g.townOwner[t]] = 1;
            if (g.prosperity[t] < 25 && g.day > 2 && t > 0) {
                g.townOwner[t] = g.townOwner[t] == HOSTILE ? towns[t].faction : HOSTILE;
                snprintf(g.worldNews, sizeof(g.worldNews), "%s changed hands. Roads and trade are unsettled.",
                         towns[t].name);
                for (int i = 0; i < g.actorCount; i++)
                    if (!g.actors[i].squad && g.actors[i].homeTown == t)
                        g.actors[i].faction = g.townOwner[t];
                for (int i = 0; i < g.buildingCount; i++)
                    if (g.buildings[i].owner < 0 && town_near(g.buildings[i].x, g.buildings[i].y, 13) == t)
                        g.buildings[i].faction = g.townOwner[t];
            }
        }
    }
    for (int i = 0; i < g.actorCount; i++) {
        Actor *a = &g.actors[i];
        if (!a->active || a->dead)
            continue;
        a->speechTime = fmaxf(0, a->speechTime - dt);
        a->speechCooldown -= dt;
        if (a->speechCooldown <= 0 && !a->squad && a->kind != BEAST) {
            a->speechCooldown = 18 + wrand() % 35;
            if (distance(a->x, a->y, g.actors[g.selected].x, g.actors[g.selected].y) < 8) {
                a->speech = (i + g.day + (int)(g.time / 10)) % 24;
                a->speechTime = 4;
            }
        }
        if (!a->squad)
            continue;
        if (a->prisonTime > 0) {
            a->prisonTime = fmaxf(0, a->prisonTime - dt);
            a->bleed = 0;
            a->unconscious = 0;
            a->blood = fmaxf(35, a->blood);
            for (int j = 0; j < LIMBS; j++)
                if (!(a->lostMask & (1 << j)))
                    a->health[j] = fmaxf(30, a->health[j]);
            if (a->prisonTime <= 0) {
                a->bounty = 0;
                g.rep[g.townOwner[a->prisonTown]] = 0;
                log_msg("%s served their sentence.", a->name);
            }
            continue;
        }
        if (!actor_inside(a) && a->unconscious > 0 && a->bounty >= 80) {
            for (int j = 0; j < g.actorCount; j++) {
                Actor *b = &g.actors[j];
                if (b->kind == GUARD && b->faction != HOSTILE && conscious(b) &&
                    distance(a->x, a->y, b->x, b->y) < 4) {
                    int t = b->homeTown;
                    if (t < 0)
                        continue;
                    a->prisonTown = t;
                    a->prisonTime = 90 + a->bounty * .25f;
                    a->x = towns[t].x - 6;
                    a->y = towns[t].y + 5;
                    for (int s = 0; s < PACK; s++)
                        if (a->stolen[s]) {
                            a->pack[s] = (Stack){0, 0};
                            a->stolen[s] = 0;
                        }
                    log_msg("%s taken into custody at %s.", a->name, towns[t].name);
                    break;
                }
            }
        }
        int sheltered = actor_inside(a) || a->taskKind == 3 || town_near(a->x, a->y, 8) >= 0;
        if ((g.weather == 3 || g.weather == 4) && !sheltered)
            a->exposure = fminf(100, a->exposure + dt * fmaxf(0, .20f - gear_bonus(a, 4) * .002f));
        else
            a->exposure = fmaxf(0, a->exposure - dt * .45f);
        if (a->exposure > 65) {
            a->health[CHEST] -= dt * .12f;
            if (a->health[CHEST] <= 0)
                a->unconscious = 30;
        }
        worker_production(a, dt);
    }
    for (int i = 0; i < g.buildingCount; i++)
        if (g.buildings[i].type == FARM && g.weather == 2)
            g.buildings[i].stock = 1;
    for (int i = 0; i < CONTRACTS; i++) {
        Contract *c = &g.contracts[i];
        if (c->active != 1)
            continue;
        if (c->type == 2)
            c->progress = g.kills - c->baseline;
        if (c->type == 0 && town_near(g.actors[g.selected].x, g.actors[g.selected].y, 12) == c->target)
            c->progress = 1;
    }
    int surveyed = 0;
    for (int t = 0; t < TOWNS; t++)
        surveyed += g.discovered[t];
    int stage = (g.contractsDone >= 3) + (g.tech >= 2) + (surveyed >= 8) +
                (g.contractsDone >= 10 && g.tech == 3 && surveyed >= 12);
    if (stage > g.storyStage) {
        g.storyStage = stage;
        g.money += 150;
        log_msg("Expedition legacy advanced: %d/4. Patronage +150.", stage);
    }
}
