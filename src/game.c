#include "game.h"
#include "legacy.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Game g;
UI ui;
volatile uint32_t display_buffer = 0x04000000;
volatile uint32_t rendered_frames = 0;
static char saveRoot[256] = ".";
const char *skillNames[SKILLS] = {
    "Athletics",    "Strength", "Dexterity",   "Melee attack", "Melee defence",  "Toughness", "Dodge",
    "Martial arts", "Stealth",  "Thievery",    "Lockpicking",  "Field medicine", "Labouring", "Farming",
    "Cooking",      "Smithing", "Engineering", "Science",      "Crossbows",      "Perception"};
const char *limbNames[LIMBS] = {"Head", "Chest", "Left arm", "Right arm", "Left leg", "Right leg"};
const char *jobNames[] = {"Follow squad", "Hold position", "Mine nearest ore", "Tend camp farm",
                          "Field medic",  "Smelt iron",    "Cook rations",     "Haul to depot"};
const char *buildNames[] = {"Campfire",       "Bedroll & shelter", "Farm plot",     "Iron forge",
                            "Research bench", "Palisade",          "Storage depot", "Wind pump"};
const char *recipeNames[] = {"Iron bar",      "Cooked ration",  "Field dressing", "Iron sabre",
                             "Plate cuirass", "Mechanical arm", "Mechanical leg", "Crossbow bolts"};
const int recipeCount = 8, buildCount = 8;
const ItemDef baseDefs[BASE_ITEMS] = {
    {"Empty", "", 0, 0, -1, 0, 0, 0},
    {"Trail ration", "Restores 48 hunger. Eaten automatically below 40.", 35, 1, -1, 0, 0, 0xc39e65},
    {"Field dressing", "Stops bleeding; restores injured limbs. X to treat.", 65, 1, -1, 0, 0, 0xd8d2ad},
    {"Iron ore", "Mine, sell or smelt at a forge.", 18, 4, -1, 0, 0, 0x77817a},
    {"Copper ore", "Valuable trade metal. Heavy to carry.", 30, 3, -1, 0, 0, 0xb87f55},
    {"Timber", "Construction material for camp structures.", 12, 2, -1, 0, 0, 0x97795c},
    {"Cloth", "Bandages, shelters and armor components.", 20, 1, -1, 0, 0, 0xd8c69c},
    {"Water", "Use to restore 12 hunger; needed by crops.", 10, 1, -1, 0, 0, 0x79aaaa},
    {"Grain", "Harvested from irrigated farms. Cook into food.", 12, 1, -1, 0, 0, 0xc3b379},
    {"Raw meat", "Butchered from wild animals. Cook before eating.", 20, 2, -1, 0, 0, 0xad6963},
    {"Iron bar", "Refined metal for weapons and armor.", 50, 3, -1, 0, 0, 0xa7a99a},
    {"Rusted blade", "Cut 10. An old sword with a jagged edge.", 65, 4, 0, 10, 0, 0x8f8269},
    {"Iron sabre", "Cut 18. Fast, balanced and reliable.", 260, 5, 0, 18, 0, 0xc9c6a8},
    {"Desert cleaver", "Cut 27. Heavy; benefits from strength.", 420, 9, 0, 27, 0, 0x8f9a9c},
    {"Ashwood staff", "Blunt 12. Light; avoids most bleeding.", 95, 3, 0, 12, 0, 0xae8760},
    {"Scout crossbow", "Pierce 22. Uses bolts; range 7 tiles.", 360, 5, 0, 22, 0, 0x796e53},
    {"Crossbow bolt", "Ammunition. One consumed per ranged attack.", 6, 0, -1, 0, 0, 0xc6bd9b},
    {"Desert rags", "Armor 2. Torn wraps and a faded sash.", 30, 1, 1, 0, 2, 0xa49270},
    {"Leather vest", "Armor 9. Light protection, free movement.", 180, 3, 1, 0, 9, 0x976247},
    {"Plate cuirass", "Armor 20. Strong protection; heavy.", 480, 10, 1, 0, 20, 0x879899},
    {"Traveler duster", "Armor 6. Long weathered desert coat.", 155, 2, 1, 0, 6, 0x647b75},
    {"Straw hat", "Armor 2. A wide woven brim.", 40, 1, 2, 0, 2, 0xc9b37b},
    {"Wrapped hood", "Armor 5. Cloth and a dark face wrap.", 85, 1, 2, 0, 5, 0xb3a995},
    {"Iron helmet", "Armor 15. A plated face guard.", 220, 4, 2, 0, 15, 0x90a0a0},
    {"Travel boots", "Armor 5. Leather boots protect your legs.", 70, 2, 3, 0, 5, 0x725345},
    {"Sandals", "Armor 1. Light desert footwear.", 15, 0, 3, 0, 1, 0xa28863},
    {"Mechanical arm", "Fits a lost arm. Restores its function.", 700, 3, -1, 0, 0, 0x9fa8a1},
    {"Mechanical leg", "Fits a lost leg. Restores walking speed.", 700, 3, -1, 0, 0, 0x9fa8a1},
    {"Lock tools", "Reusable. Needed to pick sealed salvage.", 90, 1, -1, 0, 0, 0xb5ada0},
    {"Old-world relic", "Sell for silver or spend on research.", 300, 2, -1, 0, 0, 0x80c8be},
    {"Cartridge", "Ammunition for frontier guns.", 10, 0, -1, 0, 0, 0xac925b},
    {"Cured hide", "Leatherworking and trade material.", 24, 1, -1, 0, 0, 0x78553b},
    {"Coal", "High-grade forging fuel.", 18, 2, -1, 0, 0, 0x34393a},
    {"Machine parts", "Salvage for advanced equipment.", 90, 2, -1, 0, 0, 0x698078},
    {"Blood tonic", "Restores blood and reduces exposure.", 110, 1, -1, 0, 0, 0x9c5141},
    {"Survey chart", "Reveals settlement positions and terrain.", 180, 0, -1, 0, 0, 0xbea777}};
const Town towns[TOWNS] = {{"DUSK HAVEN", 26, 27, UNION},       {"COPPER CROSS", 76, 32, SCAVENGERS},
                           {"SALTWATCH", 32, 83, UNION},        {"THE DRY MARKET", 91, 88, SCAVENGERS},
                           {"BROKEN SPIRE", 61, 62, HOSTILE},   {"RED GULCH", 155, 42, UNION},
                           {"IRON GATE", 235, 71, SHOGUN},      {"ASH MONASTERY", 322, 36, MONKS},
                           {"BLACK MESA", 165, 150, OUTLAWS},   {"REED CROSSING", 65, 215, SCAVENGERS},
                           {"CINDER PORT", 138, 294, UNION},    {"SHOGUN'S REST", 278, 198, SHOGUN},
                           {"PALE PASS", 307, 322, MONKS},      {"DUSK FERRY", 228, 281, SCAVENGERS},
                           {"VULTURE NEST", 338, 135, HOSTILE}, {"PINE REFUGE", 48, 333, MONKS}};

static uint32_t rnd(void) {
    uint32_t x = g.rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g.rng = x ? x : 17;
    return g.rng;
}
static float rf(void) { return (rnd() & 65535) / 65535.0f; }
static float clamp(float f, float a, float b) { return f < a ? a : (f > b ? b : f); }
float distance(float x, float y, float u, float v) {
    float a = x - u, b = y - v;
    return sqrtf(a * a + b * b);
}
int conscious(const Actor *a) {
    return a->active && !a->dead && a->unconscious <= 0 && a->carriedBy < 0 && a->stun <= 0;
}
int level(const Actor *a, int s) { return (int)clamp(1 + sqrtf(a->xp[s] / 12), 1, 100); }
void gain(Actor *a, int s, float n) {
    if (s < 0 || s >= SKILLS || !a->squad)
        return;
    int before = level(a, s);
    a->xp[s] = fminf(117612, a->xp[s] + n);
    if (level(a, s) > before && g.actors + g.selected == a)
        log_msg("%s: %s %d", a->name, skillNames[s], level(a, s));
}
void log_msg(const char *fmt, ...) {
    char b[100];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(b, sizeof(b), fmt, ap);
    va_end(ap);
    for (int i = LOGS - 1; i > 0; i--)
        memcpy(ui.logs[i], ui.logs[i - 1], 80);
    snprintf(ui.logs[0], 80, "%.79s", b);
    snprintf(ui.notice, 100, "%s", b);
    ui.noticeTime = 4;
    ui.logCount++;
}
int inventory_count(const Actor *a, int item) {
    int n = 0;
    for (int i = 0; i < PACK; i++)
        if (a->pack[i].item == item)
            n += a->pack[i].qty;
    return n;
}
int inventory_add(Actor *a, int item, int qty) {
    if (item <= 0 || item >= ITEMS || qty <= 0)
        return 0;
    for (int i = 0; i < PACK; i++)
        if (a->pack[i].item == item && a->pack[i].qty + qty <= 999) {
            a->pack[i].qty += qty;
            return 1;
        }
    for (int i = 0; i < PACK; i++)
        if (!a->pack[i].item) {
            a->pack[i] = (Stack){item, qty};
            return 1;
        }
    return 0;
}
int inventory_remove(Actor *a, int item, int qty) {
    if (qty <= 0 || inventory_count(a, item) < qty)
        return 0;
    for (int i = 0; i < PACK && qty; i++)
        if (a->pack[i].item == item) {
            int n = a->pack[i].qty < qty ? a->pack[i].qty : qty;
            a->pack[i].qty -= n;
            qty -= n;
            if (!a->pack[i].qty) {
                a->pack[i].item = 0;
                a->stolen[i] = 0;
            }
        }
    return 1;
}
float encumbrance(const Actor *a) {
    float w = 0;
    for (int i = 0; i < PACK; i++)
        w += itemDefs[a->pack[i].item].weight * a->pack[i].qty;
    for (int i = 0; i < GEARS; i++)
        w += itemDefs[a->gear[i]].weight;
    if (a->carrying >= 0)
        w += 40;
    return w;
}
float movement_speed(const Actor *a) {
    float cap = 30 + level(a, STRENGTH) * 2 + gear_bonus(a, 3);
    float f = 1.0f / (1 + fmaxf(0, encumbrance(a) - cap) / cap);
    if (a->health[LLEG] < 20 || a->health[RLEG] < 20)
        f *= .55f;
    if ((a->lostMask & 48) & ~a->prostheticMask)
        f *= .22f;
    if (a->hunger < 20)
        f *= .7f;
    if (a->fatigue > 85)
        f *= .75f;
    return (1.9f + level(a, ATHLETICS) * .027f) * f * (a->sneaking ? .55f : 1) *
           (1 + gear_bonus(a, 1) * .005f);
}
int town_near(float x, float y, float r) {
    for (int i = 0; i < TOWNS; i++)
        if (distance(x, y, towns[i].x, towns[i].y) < r)
            return i;
    return -1;
}
int can_walk(float x, float y) {
    if (x < 2 || y < 2 || x > WORLD - 3 || y > WORLD - 3)
        return 0;
    if (terrain_water(x, y))
        return 0;
    for (int i = 0; i < g.buildingCount; i++) {
        Building *b = &g.buildings[i];
        if (b->type == HOUSE || b->type == SHOP || b->type == BAR || b->type == TOWER || b->type == WALL) {
            float r = b->type == TOWER ? .8f : b->type == WALL ? .55f : 1.25f;
            if (fabsf(b->x - x) < r && fabsf(b->y - y) < r)
                return 0;
        }
    }
    return 1;
}
void move_actor(Actor *a, float dx, float dy, float dt) {
    float len = sqrtf(dx * dx + dy * dy);
    if (len < .01f || !conscious(a))
        return;
    if (len > 1) {
        dx /= len;
        dy /= len;
    }
    if (g.hitstop > 0 || a->prisonTime > 0 || a->attackPhase == 1 || a->attackPhase == 2)
        return;
    a->facing = dx - dy > 0 ? 1 : 0;
    float sp = movement_speed(a) * dt;
    float ox = a->x, oy = a->y;
    if (can_walk(a->x + dx * sp, a->y))
        a->x += dx * sp;
    if (can_walk(a->x, a->y + dy * sp))
        a->y += dy * sp;
    float moved = distance(ox, oy, a->x, a->y);
    if (moved > .001f) {
        a->anim += dt * 8;
        a->movedTimer = .12f;
        gain(a, ATHLETICS, moved * 2);
        gain(a, STRENGTH, moved * encumbrance(a) * .035f);
        if (a->sneaking)
            gain(a, STEALTH, moved * 1.5f);
        a->fatigue = clamp(a->fatigue + dt * .016f, 0, 100);
    }
}
int add_actor(const char *name, int kind, int faction, float x, float y, int skill) {
    if (g.actorCount >= MAX_ACTORS)
        return -1;
    int n = g.actorCount++;
    Actor *a = &g.actors[n];
    memset(a, 0, sizeof(*a));
    snprintf(a->name, 16, "%s", name);
    a->active = 1;
    a->homeTown = town_near(x, y, 12);
    a->prisonTown = -1;
    a->skin = n % 8;
    a->hair = n % 7;
    a->hairColor = n % 6;
    a->body = n % 3;
    a->beard = n % 4;
    a->scar = n % 5;
    a->speechCooldown = 6 + n % 13;
    a->kind = kind;
    a->faction = faction;
    a->x = a->homeX = a->tx = x;
    a->y = a->homeY = a->ty = y;
    a->blood = a->hunger = 100;
    a->target = a->carrying = a->carriedBy = a->attackTarget = -1;
    for (int i = 0; i < LIMBS; i++)
        a->health[i] = 100;
    for (int i = 0; i < SKILLS; i++)
        a->xp[i] = 12 * (skill - 1) * (skill - 1);
    a->gear[0] = kind == BEAST ? NONE : kind == GUARD ? SABRE : kind == RAIDER ? RUSTSWORD : STAFF;
    a->gear[1] = kind == BEAST ? NONE : kind == GUARD ? PLATE : kind == RAIDER ? LEATHER : DUSTER;
    a->gear[2] = kind == GUARD ? HELM : kind == RAIDER ? HAT : STRAW;
    a->gear[3] = BOOTS;
    inventory_add(a, RATION, 2);
    if (kind == RAIDER)
        inventory_add(a, COPPER, 2);
    if (kind == BEAST) {
        memset(a->gear, 0, sizeof(a->gear));
        memset(a->pack, 0, sizeof(a->pack));
        inventory_add(a, MEAT, 3);
    }
    return n;
}
int add_build(float x, float y, int type, int faction, int owner) {
    if (g.buildingCount >= MAX_BUILDINGS)
        return -1;
    int n = g.buildingCount++;
    g.buildings[n] = (Building){x, y, type, faction, owner, 1, 0, 0};
    return n;
}
void add_node(float x, float y, int type, int owner, int item, int qty) {
    if (g.nodeCount >= MAX_NODES)
        return;
    g.nodes[g.nodeCount++] =
        (Node){x,    y,   type, type <= NODE_WOOD ? 20 : 1, type == NODE_LOCKED || type == NODE_RUIN, owner,
               item, qty, 0};
}
void restock(void) {
    for (int t = 0; t < TOWNS; t++) {
        memset(g.stock[t], 0, sizeof(g.stock[t]));
        for (int j = 1; j < BASE_ITEMS; j++)
            g.stock[t][j] = j <= 10 ? 16 : (j == BOLTS || j == CARTRIDGE ? 60 : j == RELIC ? 1 : 3);
        for (int j = 0; j < 36; j++) {
            int f = (t * 11 + j * 7 + g.day * 3) % FAMILIES;
            int m = (t + j / 6 + g.day) % MATERIALS, q = (j + t + g.day) % QUALITIES;
            g.stock[t][gear_id(f, m, q)] = 1 + (j % 3);
        }
    }
    g.lastRestock = g.day;
}
void game_new(uint32_t seed, int scenario) {
    content_init();
    memset(&g, 0, sizeof(g));
    memset(&ui, 0, sizeof(ui));
    g.seed = g.rng = seed ? seed : 73819;
    g.day = 1;
    g.dayTime = 8;
    g.money = scenario == 1 ? 1400 : 650;
    g.selected = 0;
    g.raidTimer = 500;
    ui.speed = 1;
    ui.slot = 1;
    ui.mapX = 26;
    ui.mapY = 29;
    int p = add_actor("ASH", PLAYER, FREE, 26, 29.5f, 3);
    Actor *a = &g.actors[p];
    a->squad = 1;
    a->autoattack = 1;
    a->gear[0] = RUSTSWORD;
    a->gear[1] = RAGS;
    a->gear[2] = STRAW;
    a->gear[3] = SANDALS;
    inventory_add(a, MEDKIT, 4);
    inventory_add(a, WOOD, 8);
    inventory_add(a, CLOTH, 5);
    inventory_add(a, TOOLS, 1);
    g.squadCount = 1;
    if (scenario == 1) {
        int n = add_actor("REED", PLAYER, FREE, 25, 30.5f, 5);
        g.actors[n].squad = 1;
        g.actors[n].autoattack = 1;
        inventory_add(&g.actors[n], MEDKIT, 3);
        n = add_actor("VALE", PLAYER, FREE, 27, 30.5f, 5);
        g.actors[n].squad = 1;
        g.actors[n].autoattack = 1;
        g.actors[n].gear[0] = CROSSBOW;
        inventory_add(&g.actors[n], BOLTS, 40);
        g.squadCount = 3;
    }
    for (int t = 0; t < 5; t++) {
        float x = towns[t].x, y = towns[t].y;
        int f = towns[t].faction;
        add_build(x - 4, y - 3, HOUSE, f, -1);
        add_build(x, y - 5, TOWER, f, -1);
        add_build(x + 4, y - 3, SHOP, f, -1);
        add_build(x - 4, y + 3, BAR, f, -1);
        add_build(x + 4, y + 3, HOUSE, f, -1);
        add_build(x + 7, y, WINDMILL, f, -1);
        add_build(x, y + 1, WELL, f, -1);
        if (t == 4) {
            add_actor("Spire reaver", RAIDER, HOSTILE, x + 1, y + 2, 16);
            add_actor("Spire reaver", RAIDER, HOSTILE, x - 2, y + 1, 16);
            add_node(x - 1, y - 2, NODE_RUIN, 0, RELIC, 2);
            continue;
        }
        int k = add_actor("Watch keeper", GUARD, f, x - 1, y + 4, 16);
        g.actors[k].homeX = x - 1;
        add_actor("Gate guard", GUARD, f, x + 2, y - 1, 16);
        add_actor(t == 0 ? "MARA - TRADER" : "Outpost trader", TRADER, f, x + 2, y - 1.5f, 6);
        static const char *names[] = {"IVO", "SABLE", "FLINT", "JUNO"};
        add_actor(names[t], RECRUIT, f, x - 2, y + 3.5f, 6 + t * 2);
        add_actor("Wayfarer", SETTLER, f, x + 1, y + 4, 4);
        add_actor("Caravan scout", CARAVAN, f, x + 3, y + 5, 13);
        add_node(x + 3, y - 4.5f, NODE_LOCKED, f, RELIC, 1);
    }
    for (int i = 0; i < 110; i++) {
        float x = 5 + rf() * 116, y = 5 + rf() * 116;
        if (town_near(x, y, 9) >= 0)
            continue;
        int t = i % 9 == 0 ? NODE_RUIN : i % 3;
        add_node(x, y, t, 0, t == NODE_RUIN ? RELIC : NONE, t == NODE_RUIN ? 1 : 0);
    }
    add_node(24, 34, NODE_ORE, 0, 0, 0);
    add_node(29, 34, NODE_WOOD, 0, 0, 0);
    add_node(32, 29, NODE_LOOT, 0, LEATHER, 1);
    add_node(32, 32, NODE_LOOT, 0, MEDKIT, 2);
    for (int i = 0; i < 18; i++) {
        float x = 9 + rf() * 108, y = 9 + rf() * 108;
        if (town_near(x, y, 12) >= 0) {
            x += 12;
            y += 12;
        }
        add_actor(i % 3 == 0 ? "Dust hound" : "Dune scavenger", i % 3 == 0 ? BEAST : RAIDER, HOSTILE,
                  clamp(x, 3, WORLD - 4), clamp(y, 3, WORLD - 4), i % 3 == 0 ? 6 : 5 + i % 7);
    }
    add_actor("Starving drifter", RAIDER, HOSTILE, 36, 35, 3);
    restock();
    g.discovered[0] = 1;
    ui.camX = a->x;
    ui.camY = a->y;
    log_msg("Dusk Haven. Find supplies, allies, and a way out.");
    init_expansion();
}

int hostile(const Actor *a, const Actor *b) {
    if (a->faction == b->faction)
        return 0;
    if (a->faction == HOSTILE || b->faction == HOSTILE || a->faction == WILDLIFE || b->faction == WILDLIFE)
        return 1;
    if (a->squad && b->kind == GUARD && g.rep[b->faction] < -25)
        return 1;
    if (b->squad && a->kind == GUARD && g.rep[a->faction] < -25)
        return 1;
    return 0;
}
static int armor_at(const Actor *a, int limb) {
    return limb >= LLEG ? itemDefs[a->gear[3]].armor / 2 + itemDefs[a->gear[4]].armor
                        : itemDefs[a->gear[limb == HEAD ? 2 : 1]].armor;
}
void strike(int ai, int bi) {
    if (ai < 0 || bi < 0 || ai >= g.actorCount || bi >= g.actorCount || ai == bi)
        return;
    Actor *a = &g.actors[ai], *b = &g.actors[bi];
    if (!conscious(a) || !conscious(b) || a->cooldown > 0 || a->attackPhase || a->prisonTime > 0)
        return;
    int w = a->gear[0], ranged = weapon_ranged(w);
    if (distance(a->x, a->y, b->x, b->y) > (w ? itemDefs[w].range : 1.55f))
        return;
    if (ranged && !inventory_count(a, gear_family(w) >= 29 ? CARTRIDGE : BOLTS)) {
        if (inventory_add(a, w, 1)) {
            a->gear[0] = NONE;
            a->gearStolen[0] = 0;
        }
        if (a->squad)
            log_msg("%s needs ammunition.", a->name);
        return;
    }
    if ((a->lostMask & (1 << RARM)) && !(a->prostheticMask & (1 << RARM)))
        w = NONE;
    float cadence = fmaxf(.55f, (w ? itemDefs[w].cooldown : 1.0f) - level(a, DEXTERITY) * .003f);
    a->attackPhase = 1;
    a->attackTarget = bi;
    a->attackWeapon = w;
    a->attackDuration = a->attackClock = fmaxf(.16f, cadence * (ranged ? .46f : .28f));
    a->cooldown = cadence;
    a->taskKind = 0;
    a->attackAngle = atan2f((b->x + b->y - a->x - a->y) * 11, (b->x - b->y - a->x + a->y) * 22);
    a->facing = cosf(a->attackAngle) >= 0;
    if (distance(a->x, a->y, g.actors[g.selected].x, g.actors[g.selected].y) < 12)
        audio_event(ranged ? (gear_family(w) >= 29 ? 4 : 1) : 1);
}
static void resolve_strike(int ai, int bi) {
    if (ai < 0 || bi < 0 || ai >= g.actorCount || bi >= g.actorCount)
        return;
    Actor *a = &g.actors[ai], *b = &g.actors[bi];
    if (!conscious(a) || b->dead || b->unconscious > 0)
        return;
    int w = a->attackWeapon, ranged = weapon_ranged(w);
    if (distance(a->x, a->y, b->x, b->y) > (w ? itemDefs[w].range : 1.55f) + .3f)
        return;
    if (ranged && !inventory_remove(a, gear_family(w) >= 29 ? CARTRIDGE : BOLTS, 1))
        return;
    int sk = ranged ? CROSSBOWS : w ? MELEE : MARTIAL;
    float atk = level(a, sk) + gear_bonus(a, 0), def = level(b, b->gear[0] ? DEFENCE : DODGE);
    for (int e = 0; e < GEARS; e++)
        if (itemDefs[b->gear[e]].effect == 4)
            def += itemDefs[b->gear[e]].potency;
    if (ranged && g.weather)
        atk -= 7;
    if (a->health[LARM] < 30)
        atk -= 15;
    if ((a->lostMask & (1 << LARM)) && !(a->prostheticMask & (1 << LARM)))
        atk -= 15;
    gain(a, sk, 8);
    gain(a, DEXTERITY, 3);
    if (rf() > clamp(.71f + (atk - def) * .008f, .22f, .95f)) {
        gain(b, b->gear[0] ? DEFENCE : DODGE, 9);
        if (distance(b->x, b->y, g.actors[g.selected].x, g.actors[g.selected].y) < 12 && b->gear[0])
            audio_event(3);
        b->impactKind = b->gear[0] ? 2 : 3;
        b->hitFlash = .24f;
        b->impactDamage = 0;
        return;
    }
    int limb = rnd() % LIMBS;
    if (rf() < .35f)
        limb = CHEST;
    float damage =
        (w ? itemDefs[w].power : 7 + level(a, MARTIAL) * .4f) + level(a, STRENGTH) * .16f + atk * .18f;
    damage *= .8f + rf() * .4f;
    damage -= fmaxf(0, armor_at(b, limb) - itemDefs[w].penetration) * .65f;
    if (itemDefs[w].effect == 3)
        b->stun = .3f + itemDefs[w].potency * .03f;
    damage = fmaxf(2, damage);
    if (a->health[RARM] < 20)
        damage *= .65f;
    if (a->hunger < 15)
        damage *= .7f;
    if (distance(b->x, b->y, g.actors[g.selected].x, g.actors[g.selected].y) < 12)
        audio_event(2);
    b->health[limb] -= damage;
    b->impactLimb = limb;
    b->impactDamage = (int)damage;
    b->impactKind = 1;
    b->hitFlash = .38f;
    b->stun = fmaxf(b->stun, .10f + fminf(.35f, damage * .006f));
    b->attackPhase = 0;
    float dx = b->x - a->x, dy = b->y - a->y, d = fmaxf(.1f, sqrtf(dx * dx + dy * dy));
    b->recoilX = dx / d;
    b->recoilY = dy / d;
    float knock = fminf(.28f, damage * .006f);
    if (can_walk(b->x + dx / d * knock, b->y + dy / d * knock)) {
        b->x += dx / d * knock;
        b->y += dy / d * knock;
    }
    if (a->squad || b->squad) {
        g.hitstop = fminf(.075f, .025f + damage * .001f);
        g.shake = fminf(.22f, damage * .006f);
    }

    gain(b, TOUGHNESS, damage * 1.3f);
    gain(b, DEFENCE, 4);
    if (w && w != STAFF && !(gear_family(w) >= 14 && gear_family(w) <= 16))
        b->bleed += damage * (itemDefs[w].effect == 0 ? .04f : .025f);
    b->target = ai;
    if (a->squad || b->squad)
        log_msg("%s hits %s: %s -%d", a->name, b->name, limbNames[limb], (int)damage);
    if (limb >= LARM && b->health[limb] < -55 && !(b->lostMask & (1 << limb))) {
        b->lostMask |= 1 << limb;
        b->bleed += 2;
        if (b->squad)
            log_msg("%s lost %s. Stop the bleeding!", b->name, limbNames[limb]);
    }
    if (b->health[HEAD] <= 0 || b->health[CHEST] <= 0 || b->blood < 20) {
        b->unconscious = 25 + damage;
        b->autoattack = 0;
        if (a->squad && !b->defeatCredited) {
            b->defeatCredited = 1;
            g.kills++;
            if (b->bountyReward)
                log_msg("Bounty target down. Loot for proof and reward.");
        }
        if (a->squad || b->squad)
            log_msg("%s is unconscious.", b->name);
    }
}
int heal_actor(int mi, int pi) {
    if (mi < 0 || pi < 0 || mi >= g.actorCount || pi >= g.actorCount)
        return 0;
    Actor *m = &g.actors[mi], *p = &g.actors[pi];
    if (!conscious(m) || p->dead || distance(m->x, m->y, p->x, p->y) > 3)
        return 0;
    int hurt = p->bleed > .01f;
    for (int j = 0; j < LIMBS; j++)
        if (p->health[j] < 98 && !(p->lostMask & (1 << j)))
            hurt = 1;
    if (!hurt)
        return 0;
    if (!inventory_remove(m, MEDKIT, 1)) {
        if (mi == g.selected)
            log_msg("No field dressings.");
        return 0;
    }
    p->bleed = 0;
    for (int j = 0; j < LIMBS; j++)
        if (!(p->lostMask & (1 << j)))
            p->health[j] = fminf(100, p->health[j] + 18 + level(m, MEDICINE) * .65f + gear_bonus(m, 5));
    p->blood = fminf(100, p->blood + 10);
    gain(m, MEDICINE, 24);
    log_msg("%s bandaged %s. Bleeding stopped.", m->name, p->name);
    return 1;
}
void use_item(int s) {
    Actor *a = &g.actors[g.selected];
    if (s < 0 || s >= PACK || !a->pack[s].item)
        return;
    int it = a->pack[s].item;
    if (itemDefs[it].slot >= 0) {
        int e = itemDefs[it].slot, old = a->gear[e], owner = a->stolen[s], oldOwner = a->gearStolen[e];
        Actor backup = *a;
        inventory_remove(a, it, 1);
        if (old && !inventory_add(a, old, 1)) {
            *a = backup;
            log_msg("Pack full. Cannot swap equipment.");
            return;
        }
        a->gear[e] = it;
        a->gearStolen[e] = owner;
        if (old && oldOwner)
            for (int j = 0; j < PACK; j++)
                if (a->pack[j].item == old)
                    a->stolen[j] = oldOwner;
        log_msg("Equipped %s.", itemDefs[it].name);
    } else if (it == RATION || it == WATER) {
        inventory_remove(a, it, 1);
        a->hunger = fminf(100, a->hunger + (it == RATION ? 48 : 12));
        log_msg("Used %s.", itemDefs[it].name);
    } else if (it == TONIC) {
        inventory_remove(a, TONIC, 1);
        a->blood = fminf(100, a->blood + 35);
        a->exposure = fmaxf(0, a->exposure - 60);
        log_msg("Tonic restores blood and warmth.");
    } else if (it == MAPSCROLL) {
        inventory_remove(a, MAPSCROLL, 1);
        memset(g.seen, 255, sizeof(g.seen));
        for (int t = 0; t < TOWNS; t++)
            g.discovered[t] = 1;
        log_msg("Survey chart reveals the frontier.");
    } else if (it == MEDKIT)
        heal_actor(g.selected, g.selected);
    else if (it == ARM || it == LEG) {
        int first = it == ARM ? LARM : LLEG, done = 0;
        for (int i = first; i < first + 2; i++)
            if ((a->lostMask & (1 << i)) && !(a->prostheticMask & (1 << i))) {
                inventory_remove(a, it, 1);
                a->prostheticMask |= 1 << i;
                a->health[i] = 100;
                done = 1;
                log_msg("Fitted %s.", itemDefs[it].name);
                break;
            }
        if (!done)
            log_msg("No missing limb needs this prosthetic.");
    } else
        log_msg("%s", itemDefs[it].desc);
}
int buy_item(int town, int it) {
    Actor *a = &g.actors[g.selected];
    if (town < 0 || town >= TOWNS || it <= 0 || it >= ITEMS)
        return 0;
    int price = item_price(town, it, 0);
    if (g.rep[g.townOwner[town]] < -25) {
        log_msg("This faction refuses to trade with you.");
        return 0;
    }
    if (g.money < price) {
        log_msg("Not enough silver.");
        return 0;
    }
    if (g.stock[town][it] <= 0) {
        log_msg("Out of stock until tomorrow.");
        return 0;
    }
    if (!inventory_add(a, it, 1)) {
        log_msg("Pack is full.");
        return 0;
    }
    g.money -= price;
    g.stock[town][it]--;
    log_msg("Bought %s for %d silver.", itemDefs[it].name, price);
    return 1;
}
int sell_item(int town, int s) {
    Actor *a = &g.actors[g.selected];
    if (town < 0 || town >= TOWNS || s < 0 || s >= PACK || !a->pack[s].item)
        return 0;
    int it = a->pack[s].item, price = item_price(town, it, 1);
    if (a->stolen[s] && a->stolen[s] == g.townOwner[town]) {
        g.rep[a->stolen[s]] -= 8;
        a->bounty += 40;
        log_msg("Recognized stolen goods. The trader alerts guards.");
        return 0;
    }
    inventory_remove(a, it, 1);
    g.money += price;
    g.stock[town][it]++;
    log_msg("Sold %s for %d silver.", itemDefs[it].name, price);
    return 1;
}
int recruit_actor(int n) {
    if (n < 0 || n >= g.actorCount)
        return 0;
    Actor *a = &g.actors[n];
    if (a->kind != RECRUIT || a->squad)
        return 0;
    if (g.squadCount >= SQUAD) {
        log_msg("Squad is full (8 members).");
        return 0;
    }
    if (g.money < 300) {
        log_msg("Recruiting costs 300 silver.");
        return 0;
    }
    g.money -= 300;
    a->squad = 1;
    a->faction = FREE;
    a->kind = PLAYER;
    a->autoattack = 1;
    g.squadCount++;
    inventory_add(a, MEDKIT, 2);
    log_msg("%s joined the squad.", a->name);
    return 1;
}

static const int buildTypes[] = {CAMPFIRE, BED, FARM, FORGE, BENCH, WALL, STORE, WINDMILL};
static const int buildWood[] = {3, 5, 6, 8, 6, 3, 5, 8}, buildIron[] = {0, 0, 0, 3, 1, 0, 0, 3},
                 buildTech[] = {0, 0, 0, 0, 0, 0, 0, 1};
int build_structure(int c) {
    Actor *a = &g.actors[g.selected];
    if (c < 0 || c >= buildCount)
        return 0;
    if (town_near(a->x, a->y, 10) >= 0) {
        log_msg("Build outside settlement boundaries.");
        return 0;
    }
    if (g.buildingCount >= MAX_BUILDINGS) {
        log_msg("Structure limit reached.");
        return 0;
    }
    if (g.tech < buildTech[c]) {
        log_msg("Research level %d required.", buildTech[c]);
        return 0;
    }
    if (inventory_count(a, WOOD) < buildWood[c] || inventory_count(a, IRONBAR) < buildIron[c]) {
        log_msg("Need %d timber and %d iron bars.", buildWood[c], buildIron[c]);
        return 0;
    }
    float x = a->x + 1.9f, y = a->y;
    if (!can_walk(x, y) || !can_walk(x + 1, y)) {
        log_msg("This space is obstructed.");
        return 0;
    }
    for (int i = 0; i < g.buildingCount; i++)
        if (distance(x, y, g.buildings[i].x, g.buildings[i].y) < 3) {
            log_msg("Too close to another structure.");
            return 0;
        }
    inventory_remove(a, WOOD, buildWood[c]);
    if (buildIron[c])
        inventory_remove(a, IRONBAR, buildIron[c]);
    add_build(x, y, buildTypes[c], FREE, g.selected);
    if (!g.baseExists) {
        g.baseExists = 1;
        g.baseX = x;
        g.baseY = y;
        g.raidTimer = 420;
    }
    gain(a, ENGINEERING, 50);
    log_msg("Built %s.", buildNames[c]);
    return 1;
}
typedef struct {
    int output, qty, in1, n1, in2, n2, station, skill, tech;
} Recipe;
static const Recipe recipes[] = {{IRONBAR, 1, ORE, 3, NONE, 0, FORGE, SMITHING, 0},
                                 {RATION, 2, GRAIN, 3, WATER, 1, CAMPFIRE, COOKING, 0},
                                 {MEDKIT, 2, CLOTH, 2, WATER, 1, BENCH, MEDICINE, 0},
                                 {SABRE, 1, IRONBAR, 3, WOOD, 1, FORGE, SMITHING, 1},
                                 {PLATE, 1, IRONBAR, 6, CLOTH, 2, FORGE, SMITHING, 2},
                                 {ARM, 1, IRONBAR, 5, RELIC, 1, FORGE, ENGINEERING, 3},
                                 {LEG, 1, IRONBAR, 5, RELIC, 1, FORGE, ENGINEERING, 3},
                                 {BOLTS, 10, IRONBAR, 1, WOOD, 1, FORGE, SMITHING, 0}};
int nearby_station(int type, float r) {
    Actor *a = &g.actors[g.selected];
    for (int i = 0; i < g.buildingCount; i++)
        if (g.buildings[i].type == type && distance(a->x, a->y, g.buildings[i].x, g.buildings[i].y) < r)
            return i;
    return -1;
}
int craft_item(int c) {
    if (c < 0 || c >= recipeCount)
        return 0;
    Actor *a = &g.actors[g.selected];
    const Recipe *r = &recipes[c];
    if (g.tech < r->tech) {
        log_msg("Research level %d required.", r->tech);
        return 0;
    }
    if (nearby_station(r->station, 5) < 0) {
        log_msg("Stand near a %s.", r->station == FORGE      ? "forge"
                                    : r->station == CAMPFIRE ? "campfire"
                                                             : "research bench");
        return 0;
    }
    int in1 = r->in1, n1 = r->n1;
    if (c == 1 && inventory_count(a, GRAIN) < n1) {
        in1 = MEAT;
        n1 = 2;
    }
    if (inventory_count(a, in1) < n1 || (r->in2 && inventory_count(a, r->in2) < r->n2)) {
        log_msg("Need %d %s + %d %s.", n1, itemDefs[in1].name, r->n2, itemDefs[r->in2].name);
        return 0;
    }
    Stack old[PACK];
    memcpy(old, a->pack, sizeof(old));
    inventory_remove(a, in1, n1);
    if (r->in2)
        inventory_remove(a, r->in2, r->n2);
    if (!inventory_add(a, r->output, r->qty)) {
        memcpy(a->pack, old, sizeof(old));
        log_msg("Pack full.");
        return 0;
    }
    gain(a, r->skill, 35);
    log_msg("Crafted %d %s.", r->qty, itemDefs[r->output].name);
    return 1;
}
int research_tech(void) {
    Actor *a = &g.actors[g.selected];
    if (g.tech >= 3) {
        log_msg("All three research tiers completed.");
        return 0;
    }
    if (nearby_station(BENCH, 5) < 0) {
        log_msg("Stand near a research bench.");
        return 0;
    }
    int need = g.tech + 1;
    if (g.money < 150 * need || inventory_count(a, RELIC) < 1) {
        log_msg("Need 1 relic and %d silver.", 150 * need);
        return 0;
    }
    inventory_remove(a, RELIC, 1);
    g.money -= 150 * need;
    g.tech++;
    gain(a, SCIENCE, 120);
    log_msg("Research tier %d completed.", g.tech);
    return 1;
}

void find_context(void) {
    Actor *a = &g.actors[g.selected];
    float best = 3;
    ui.contextKind = ui.contextIndex = -1;
    for (int i = 0; i < g.actorCount; i++) {
        Actor *b = &g.actors[i];
        if (i == g.selected || !b->active)
            continue;
        /* Healthy followers must not intercept nearby loot or workstations. */
        if (b->squad && !b->dead && b->unconscious <= 0) {
            int hurt = b->bleed > .01f;
            for (int j = 0; j < LIMBS; j++)
                if (b->health[j] < 98 && !(b->lostMask & (1 << j)))
                    hurt = 1;
            if (!hurt)
                continue;
        }
        if (b->kind == TRADER || b->kind == RECRUIT || b->dead || b->unconscious > 0 || b->squad ||
            b->kind == SETTLER || b->kind == GUARD || b->kind == CARAVAN) {
            float d = distance(a->x, a->y, b->x, b->y);
            if (d < best) {
                best = d;
                ui.contextKind = 0;
                ui.contextIndex = i;
            }
        }
    }
    for (int i = 0; i < g.nodeCount; i++) {
        Node *n = &g.nodes[i];
        if (!n->remaining)
            continue;
        float d = distance(a->x, a->y, n->x, n->y);
        if (d < best) {
            best = d;
            ui.contextKind = 1;
            ui.contextIndex = i;
        }
    }
    for (int i = 0; i < g.buildingCount; i++) {
        Building *b = &g.buildings[i];
        if (b->type < WELL || b->type == WALL || b->type == WINDMILL)
            continue;
        float d = distance(a->x, a->y, b->x, b->y);
        if (d < best) {
            best = d;
            ui.contextKind = 2;
            ui.contextIndex = i;
        }
    }
}
static void theft(Actor *a, Node *n) {
    int seen = 0;
    for (int i = 0; i < g.actorCount; i++) {
        Actor *b = &g.actors[i];
        if (b->faction == n->owner && conscious(b) &&
            distance(a->x, a->y, b->x, b->y) < (a->sneaking ? 3 : 6))
            seen = 1;
    }
    gain(a, THEFT, 20);
    if (seen) {
        g.rep[n->owner] -= 18;
        a->bounty += 80;
        log_msg("Witnessed theft! Faction -18; bounty +80.");
    } else
        log_msg("Stolen without witnesses.");
}
void interact(void) {
    Actor *a = &g.actors[g.selected];
    if (!conscious(a)) {
        log_msg("Choose a conscious squad member with L.");
        return;
    }
    find_context();
    int n = ui.contextIndex;
    if (n < 0) {
        log_msg("Nothing nearby. Move closer to a person or object.");
        return;
    }
    if (ui.contextKind == 0) {
        Actor *b = &g.actors[n];
        if (b->squad) {
            if (b->unconscious > 0) {
                if (!heal_actor(g.selected, n)) {
                    a->carrying = n;
                    b->carriedBy = g.selected;
                    log_msg("Carrying %s. Interact again to put down.", b->name);
                }
                return;
            }
            if (a->carrying == n) {
                a->carrying = -1;
                b->carriedBy = -1;
                return;
            }
            heal_actor(g.selected, n);
            return;
        }
        if (b->unconscious > 0 || b->dead) {
            if (b->bountyReward) {
                g.money += b->bountyReward;
                log_msg("Bounty proof recovered: %d silver.", b->bountyReward);
                b->bountyReward = 0;
            }
            ui.lootActor = n;
            ui.selection = 0;
            ui.screen = SCREEN_LOOT;
        } else if (a->sneaking)
            pickpocket(n);
        else
            dialogue_open(n);
        return;
    }
    if (ui.contextKind == 1) {
        Node *b = &g.nodes[n];
        if (b->locked) {
            if (!inventory_count(a, TOOLS)) {
                log_msg("Lock tools required.");
                return;
            }
            a->taskKind = 2;
            a->taskIndex = n;
            a->action = 4.5f;
            log_msg("Picking lock... stay close.");
            return;
        }
        if (b->type <= NODE_WOOD) {
            a->taskKind = 1;
            a->taskIndex = n;
            a->action = 2.8f;
            log_msg("Gathering... movement cancels the job.");
            return;
        }
        if (inventory_add(a, b->item, b->qty)) {
            if (b->owner)
                theft(a, b);
            if (b->owner)
                for (int s = 0; s < PACK; s++)
                    if (a->pack[s].item == b->item)
                        a->stolen[s] = b->owner;
            b->remaining = 0;
            log_msg("Found %d %s.", b->qty, itemDefs[b->item].name);
        } else
            log_msg("Pack full.");
        return;
    }
    Building *b = &g.buildings[n];
    if (b->type == WELL) {
        if (inventory_add(a, WATER, 2)) {
            gain(a, LABOUR, 6);
            log_msg("Drew 2 water.");
        }
    } else if (b->type == BED || b->type == CAMPFIRE) {
        if (b->type == BED && b->owner < 0) {
            start_sleep(town_near(a->x, a->y, 12));
            return;
        }
        a->taskKind = 3;
        a->taskIndex = n;
        a->action = 180;
        log_msg("Resting. Move to wake; wounds recover slowly.");
    } else if (b->type == FARM) {
        if (b->progress >= 100) {
            if (inventory_add(a, GRAIN, 5)) {
                b->progress = 0;
                b->stock = 0;
                gain(a, FARMING, 40);
                log_msg("Harvested 5 grain. Water to replant.");
            }
        } else if (inventory_remove(a, WATER, 1)) {
            b->stock = 1;
            gain(a, FARMING, 10);
            log_msg("Farm watered. Crop grows while time passes.");
        } else
            log_msg("Need water to plant this crop.");
    } else if (b->type == FORGE) {
        ui.screen = SCREEN_CRAFT;
        ui.selection = 0;
    } else if (b->type == BENCH) {
        ui.screen = SCREEN_RESEARCH;
        ui.selection = 0;
    } else if (b->type == STORE) {
        ui.storageIndex = n;
        ui.storageSide = 0;
        ui.selection = 0;
        ui.screen = SCREEN_STORAGE;
    }
}

static void complete_task(Actor *a) {
    int n = a->taskIndex;
    if (a->taskKind == 1 && n >= 0 && n < g.nodeCount) {
        Node *b = &g.nodes[n];
        if (distance(a->x, a->y, b->x, b->y) > 3 || !b->remaining) {
            a->taskKind = 0;
            return;
        }
        int it = b->type == NODE_ORE ? ORE : b->type == NODE_COPPER ? COPPER : WOOD;
        if (inventory_add(a, it, 1)) {
            b->remaining--;
            gain(a, LABOUR, 12);
            gain(a, STRENGTH, 5);
            if (g.actors + g.selected == a)
                log_msg("Gathered %s (%d left).", itemDefs[it].name, b->remaining);
        } else {
            a->taskKind = 0;
            return;
        }
        if (b->remaining > 0) {
            a->action = fmaxf(.8f, 2.8f - level(a, LABOUR) * .035f);
            return;
        }
    } else if (a->taskKind == 2 && n >= 0 && n < g.nodeCount) {
        Node *b = &g.nodes[n];
        if (distance(a->x, a->y, b->x, b->y) > 3) {
            a->taskKind = 0;
            return;
        }
        gain(a, LOCKPICK, 28);
        if (rf() < .62f + level(a, LOCKPICK) * .008f) {
            b->locked = 0;
            log_msg("Lock opened. Interact to take the contents.");
        } else
            log_msg("Lock resisted. Your lockpicking improved.");
    }
    a->taskKind = 0;
}
static void die(Actor *a) {
    a->dead = 1;
    a->unconscious = 9999;
    a->bleed = 0;
    a->taskKind = 0;
    if (a->squad)
        log_msg("%s has died. Their death is permanent.", a->name);
}
static void reveal(Actor *a) {
    int x = (int)a->x, y = (int)a->y;
    for (int yy = y - 6; yy <= y + 6; yy++)
        for (int xx = x - 6; xx <= x + 6; xx++)
            if (xx >= 0 && yy >= 0 && xx < WORLD && yy < WORLD) {
                int k = yy * WORLD + xx;
                g.seen[k >> 3] |= 1 << (k & 7);
            }
    for (int t = 0; t < TOWNS; t++)
        if (!g.discovered[t] && distance(a->x, a->y, towns[t].x, towns[t].y) < 10) {
            g.discovered[t] = 1;
            log_msg("Discovered %s.", towns[t].name);
            gain(a, PERCEPTION, 80);
        }
}
static void squad_job(Actor *a, float dt) {
    if (a->taskKind)
        return;
    if (a->job == JOB_FOLLOW) {
        Actor *l = &g.actors[g.selected];
        if (a != l && distance(a->x, a->y, l->x, l->y) > 2.2f)
            route_toward(a, l->x + (a - g.actors) % 2, l->y + 1.4f, dt);
    } else if (a->job == JOB_MINE) {
        int n = -1;
        float d = 12;
        for (int j = 0; j < g.nodeCount; j++)
            if (g.nodes[j].type <= NODE_COPPER && g.nodes[j].remaining) {
                float dd = distance(a->x, a->y, g.nodes[j].x, g.nodes[j].y);
                if (dd < d) {
                    d = dd;
                    n = j;
                }
            }
        if (n >= 0) {
            if (d > 2)
                route_toward(a, g.nodes[n].x, g.nodes[n].y, dt);
            else {
                a->taskKind = 1;
                a->taskIndex = n;
                a->action = 3;
            }
        }
    } else if (a->job == JOB_MEDIC) {
        for (int j = 0; j < g.actorCount; j++)
            if (g.actors[j].squad && !g.actors[j].dead &&
                (g.actors[j].bleed > .1f || g.actors[j].health[CHEST] < 60)) {
                if (distance(a->x, a->y, g.actors[j].x, g.actors[j].y) > 2)
                    route_toward(a, g.actors[j].x, g.actors[j].y, dt);
                else
                    heal_actor((int)(a - g.actors), j);
                break;
            }
    } else if (a->job == JOB_FARM) {
        for (int j = 0; j < g.buildingCount; j++) {
            Building *b = &g.buildings[j];
            if (b->type != FARM || b->owner < 0 || distance(a->x, a->y, b->x, b->y) > 15)
                continue;
            if (distance(a->x, a->y, b->x, b->y) > 2)
                route_toward(a, b->x, b->y, dt);
            else if (b->progress >= 100 && inventory_add(a, GRAIN, 5)) {
                b->progress = 0;
                b->stock = 0;
                gain(a, FARMING, 40);
            } else if (!b->stock && inventory_remove(a, WATER, 1))
                b->stock = 1;
            break;
        }
    }
}
void simulate(float dt) {
    g.shake = fmaxf(0, g.shake - dt);
    if (g.hitstop > 0) {
        g.hitstop = fmaxf(0, g.hitstop - dt);
        return;
    }
    tick_expansion(dt);
    g.time += dt;
    g.dayTime += dt / 75;
    if (g.dayTime >= 24) {
        g.dayTime -= 24;
        g.day++;
        restock();
    }
    for (int i = 0; i < g.buildingCount; i++) {
        Building *b = &g.buildings[i];
        if (b->type == FARM && b->stock)
            b->progress = fminf(100, b->progress + dt * .35f);
        if (b->type == WINDMILL && b->owner >= 0) {
            for (int j = 0; j < g.buildingCount; j++)
                if (g.buildings[j].type == FARM &&
                    distance(b->x, b->y, g.buildings[j].x, g.buildings[j].y) < 12)
                    g.buildings[j].stock = 1;
        }
    }
    for (int i = 0; i < g.nodeCount; i++)
        if (g.nodes[i].type <= NODE_WOOD && g.nodes[i].remaining == 0) {
            g.nodes[i].regen += dt;
            if (g.nodes[i].regen > 300) {
                g.nodes[i].remaining = 20;
                g.nodes[i].regen = 0;
            }
        }
    for (int i = 0; i < g.actorCount; i++) {
        Actor *a = &g.actors[i];
        if (!a->active || a->dead)
            continue;
        a->cooldown = fmaxf(0, a->cooldown - dt);
        a->stun = fmaxf(0, a->stun - dt);
        a->hitFlash = fmaxf(0, a->hitFlash - dt);
        a->movedTimer = fmaxf(0, a->movedTimer - dt);
        if (a->attackPhase) {
            if (!conscious(a))
                a->attackPhase = 0;
            else {
                a->attackClock -= dt;
                if (a->attackClock <= 0) {
                    if (a->attackPhase == 1) {
                        a->attackPhase = 2;
                        a->attackDuration = a->attackClock = .16f;
                        resolve_strike(i, a->attackTarget);
                    } else if (a->attackPhase == 2) {
                        a->attackPhase = 3;
                        a->attackDuration = a->attackClock = fmaxf(.12f, a->cooldown);
                    } else
                        a->attackPhase = 0;
                }
            }
        }
        if (!a->squad && distance(a->x, a->y, g.actors[g.selected].x, g.actors[g.selected].y) > 45)
            continue;
        if (a->carriedBy >= 0) {
            Actor *c = &g.actors[a->carriedBy];
            if (c->dead || c->unconscious > 0) {
                a->carriedBy = -1;
                c->carrying = -1;
            } else {
                a->x = c->x;
                a->y = c->y;
            }
        }
        if (a->bleed > 0) {
            a->blood -= a->bleed * dt * .35f;
            a->bleed = fmaxf(0, a->bleed - dt * .0015f);
        } else
            a->blood = fminf(100, a->blood + dt * .07f);
        if (a->blood <= 0 || a->health[HEAD] < -70 || a->health[CHEST] < -70) {
            die(a);
            continue;
        }
        if (a->blood < 20 && a->unconscious <= 0)
            a->unconscious = 30;
        if (a->unconscious > 0) {
            a->unconscious = fmaxf(0, a->unconscious - dt);
            if (a->bleed <= .01f) {
                if (a->health[HEAD] < 35)
                    a->health[HEAD] = fminf(35, a->health[HEAD] + dt * .35f);
                if (a->health[CHEST] < 35)
                    a->health[CHEST] = fminf(35, a->health[CHEST] + dt * .35f);
            }
            if (a->unconscious <= 0 && (a->health[HEAD] <= 0 || a->health[CHEST] <= 0 || a->blood < 25))
                a->unconscious = 10;
            continue;
        }
        if (a->squad) {
            a->hunger = fmaxf(0, a->hunger - dt * .045f);
            a->fatigue = fminf(100, a->fatigue + dt * .008f);
            if (a->hunger < 40 && inventory_remove(a, RATION, 1)) {
                a->hunger = fminf(100, a->hunger + 48);
                if (i == g.selected)
                    log_msg("%s ate a trail ration.", a->name);
            }
            if (a->hunger <= 0) {
                a->health[CHEST] -= dt * .12f;
                if (a->health[CHEST] <= 0)
                    a->unconscious = 30;
            }
            reveal(a);
        }
        if (a->taskKind) {
            if (a->taskKind == 3) {
                a->fatigue = fmaxf(0, a->fatigue - dt * 1.8f);
                if (a->bleed <= 0)
                    for (int l = 0; l < LIMBS; l++)
                        if (!(a->lostMask & (1 << l)) || (a->prostheticMask & (1 << l)))
                            a->health[l] = fminf(100, a->health[l] + dt * (.6f + gear_bonus(a, 6) * .015f));
            }
            a->action -= dt;
            if (a->action <= 0)
                complete_task(a);
        }
        if (!conscious(a))
            continue;
        int target = -1;
        float best = weapon_ranged(a->gear[0]) ? itemDefs[a->gear[0]].range + 1 : 6.0f;
        if (a->kind == BEAST)
            best = 4.5f;
        if (g.weather == 1 || g.weather == 3)
            best *= .75f;
        for (int j = 0; j < g.actorCount; j++) {
            Actor *b = &g.actors[j];
            if (i == j || !conscious(b) || !hostile(a, b))
                continue;
            float d = distance(a->x, a->y, b->x, b->y);
            float detect =
                b->sneaking ? best * fmaxf(.12f, .5f - level(b, STEALTH) * .003f - gear_bonus(b, 2) * .003f)
                            : best;
            if (d < best && d < detect) {
                target = j;
                best = d;
            }
        }
        if (target >= 0 && (a->kind == RAIDER || a->kind == BEAST || a->kind == GUARD || a->kind == CARAVAN ||
                            (a->squad && a->autoattack))) {
            a->target = target;
            Actor *b = &g.actors[target];
            float range = (a->gear[0] ? itemDefs[a->gear[0]].range : 1.55f) - .2f;
            if (best > range) {
                if (i != g.selected)
                    route_toward(a, b->x, b->y, dt);
            } else
                strike(i, target);
            continue;
        }
        a->target = -1;
        if (a->squad) {
            if (i != g.selected || a->job != JOB_FOLLOW)
                squad_job(a, dt);
        } else if (a->kind == RAIDER || a->kind == BEAST || a->kind == SETTLER || a->kind == CARAVAN) {
            if (distance(a->x, a->y, a->tx, a->ty) < .8f || !can_walk(a->tx, a->ty)) {
                if (a->kind == CARAVAN) {
                    int t = (i + g.day + (int)(g.time / 65)) % 4;
                    a->tx = towns[t].x;
                    a->ty = towns[t].y + 6;
                } else {
                    a->tx = clamp(a->homeX + (rf() - .5f) * 9, 3, WORLD - 4);
                    a->ty = clamp(a->homeY + (rf() - .5f) * 9, 3, WORLD - 4);
                }
            }
            route_toward(a, a->tx, a->ty, dt * .45f);
        }
    }
    if (g.baseExists) {
        g.raidTimer -= dt;
        if (g.raidTimer <= 0) {
            g.raidTimer = 480;
            int spawned = 0;
            for (int i = 0; i < MAX_ACTORS && spawned < 2; i++) {
                int n = -1;
                if (i < g.actorCount && g.actors[i].dead && !g.actors[i].squad)
                    n = i;
                else if (g.actorCount < MAX_ACTORS) {
                    n = add_actor("Camp raider", RAIDER, HOSTILE, clamp(g.baseX + 10, 3, WORLD - 4), g.baseY,
                                  9);
                    i = g.actorCount;
                }
                if (n >= 0) {
                    Actor *a = &g.actors[n];
                    if (a->dead) {
                        a->dead = 0;
                        a->defeatCredited = 0;
                        a->lostMask = a->prostheticMask = 0;
                        a->attackPhase = 0;
                        a->unconscious = 0;
                        a->blood = 100;
                        a->bleed = 0;
                        for (int l = 0; l < LIMBS; l++)
                            a->health[l] = 100;
                        a->x = clamp(g.baseX + 10, 3, WORLD - 4);
                        a->y = g.baseY;
                        a->kind = RAIDER;
                        a->faction = HOSTILE;
                    }
                    a->tx = a->homeX = g.baseX;
                    a->ty = a->homeY = g.baseY;
                    spawned++;
                }
            }
            if (spawned)
                log_msg("Raiders have found your camp.");
        }
    }
}

void set_save_root(const char *p) { snprintf(saveRoot, sizeof(saveRoot), "%s", p); }
static uint32_t checksum(const void *p, size_t n) {
    const uint8_t *b = p;
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; i++)
        h = (h ^ b[i]) * 16777619u;
    return h;
}
typedef struct {
    char magic[8];
    uint32_t version, size, hash;
} SaveHeader;
int save_game(const char *path) {
    char tmp[512], bak[512];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    snprintf(bak, sizeof(bak), "%s.bak", path);
    FILE *f = fopen(tmp, "wb");
    if (!f)
        return 0;
    SaveHeader h = {{'D', 'U', 'S', 'T', 'S', 'A', 'V', 'E'}, 2, sizeof(g), checksum(&g, sizeof(g))};
    int ok = fwrite(&h, sizeof(h), 1, f) == 1 && fwrite(&g, sizeof(g), 1, f) == 1;
    if (fclose(f) != 0)
        ok = 0;
    if (!ok) {
        remove(tmp);
        return 0;
    }
    remove(bak);
    rename(path, bak);
    if (rename(tmp, path) != 0) {
        rename(bak, path);
        return 0;
    }
    return 1;
}
static int validate_game(const Game *t) {
    if (t->actorCount < 1 || t->actorCount > MAX_ACTORS || t->buildingCount < 0 ||
        t->buildingCount > MAX_BUILDINGS || t->nodeCount < 0 || t->nodeCount > MAX_NODES || t->selected < 0 ||
        t->selected >= t->actorCount || !t->actors[t->selected].squad || t->tech < 0 || t->tech > 3 ||
        t->squadCount < 1 || t->squadCount > SQUAD)
        return 0;
    if (!isfinite(t->time) || !isfinite(t->dayTime) || t->weather < 0 || t->weather > 4 || t->day < 1 ||
        t->money < 0)
        return 0;
    for (int i = 0; i < TOWNS; i++) {
        if (t->townOwner[i] < 0 || t->townOwner[i] >= FACTIONS)
            return 0;
        for (int j = 0; j < ITEMS; j++)
            if (t->stock[i][j] < 0)
                return 0;
    }
    for (int i = 0; i < CONTRACTS; i++) {
        const Contract *c = &t->contracts[i];
        if (c->active && (c->town < 0 || c->town >= TOWNS || c->target < 0 || c->target >= TOWNS ||
                          c->item < 0 || c->item >= ITEMS || c->type < 0 || c->type > 3))
            return 0;
    }
    for (int i = 0; i < t->buildingCount; i++) {
        const Building *b = &t->buildings[i];
        if (!isfinite(b->x) || !isfinite(b->y) || b->type < 0 || b->type > STORE || b->faction < 0 ||
            b->faction >= FACTIONS)
            return 0;
        for (int j = 0; j < 16; j++)
            if (b->storage[j].item < 0 || b->storage[j].item >= ITEMS || b->storage[j].qty < 0 ||
                b->storage[j].qty > 999)
                return 0;
    }
    for (int i = 0; i < t->nodeCount; i++) {
        const Node *n = &t->nodes[i];
        if (!isfinite(n->x) || !isfinite(n->y) || n->owner < 0 || n->owner >= FACTIONS || n->item < 0 ||
            n->item >= ITEMS || n->type < 0 || n->type > NODE_RUIN)
            return 0;
    }
    for (int i = 0; i < t->actorCount; i++) {
        const Actor *a = &t->actors[i];
        if (!isfinite(a->x) || !isfinite(a->y) || a->x < 0 || a->x > WORLD || a->y < 0 || a->y > WORLD)
            return 0;
        if (!memchr(a->name, 0, sizeof(a->name)) || a->faction < 0 || a->faction >= FACTIONS || a->kind < 0 ||
            a->kind > CARAVAN || a->job < 0 || a->job > JOB_HAUL || a->skin < 0 || a->skin > 7 ||
            a->hairColor < 0 || a->hairColor > 5 || a->routeLen < 0 || a->routeLen > 24 || a->routePos < 0 ||
            a->routePos > 24 || a->attackPhase < 0 || a->attackPhase > 3 || a->attackWeapon < 0 ||
            a->attackWeapon >= ITEMS || a->attackTarget >= t->actorCount || a->prisonTown >= TOWNS ||
            a->homeTown >= TOWNS || !isfinite(a->blood) || !isfinite(a->bleed) || !isfinite(a->hunger) ||
            !isfinite(a->fatigue))
            return 0;
        for (int j = 0; j < LIMBS; j++)
            if (!isfinite(a->health[j]))
                return 0;
        for (int j = 0; j < SKILLS; j++)
            if (!isfinite(a->xp[j]) || a->xp[j] < 0)
                return 0;
        for (int j = 0; j < PACK; j++)
            if (a->pack[j].item < 0 || a->pack[j].item >= ITEMS || a->pack[j].qty < 0 || a->pack[j].qty > 999)
                return 0;
        for (int j = 0; j < GEARS; j++)
            if (a->gear[j] < 0 || a->gear[j] >= ITEMS)
                return 0;
        if (a->carrying >= t->actorCount || a->carriedBy >= t->actorCount)
            return 0;
    }
    return 1;
}
static int migrate_legacy(const LegacyGame *old) {
    if (old->actorCount < 1 || old->actorCount > 80 || old->buildingCount < 0 || old->buildingCount > 100 ||
        old->nodeCount < 0 || old->nodeCount > 200 || old->selected < 0 || old->selected >= old->actorCount)
        return 0;
    for (int i = 0; i < old->actorCount; i++)
        if (!isfinite(old->actors[i].x) || !isfinite(old->actors[i].y))
            return 0;
    Game *expanded = malloc(sizeof(Game));
    if (!expanded)
        return 0;
    game_new(old->seed, 0);
    *expanded = g;
    g.seed = old->seed;
    g.rng = old->rng;
    g.time = old->time;
    g.dayTime = old->dayTime;
    g.day = old->day;
    g.money = old->money;
    g.selected = old->selected;
    g.squadCount = old->squadCount;
    g.tech = old->tech;
    g.kills = old->kills;
    g.baseExists = old->baseExists;
    g.baseX = old->baseX;
    g.baseY = old->baseY;
    g.raidTimer = old->raidTimer;
    g.actorCount = old->actorCount;
    g.buildingCount = old->buildingCount;
    g.nodeCount = old->nodeCount;
    for (int i = 0; i < 4; i++)
        g.rep[i] = old->rep[i];
    for (int i = 0; i < 5; i++) {
        g.discovered[i] = old->discovered[i];
        for (int j = 0; j < 30; j++)
            g.stock[i][j] = old->stock[i][j];
    }
    memset(g.seen, 0, sizeof(g.seen));
    for (int y = 0; y < 128; y++)
        for (int x = 0; x < 128; x++)
            if (old->seen[(y * 128 + x) >> 3] & (1 << ((y * 128 + x) & 7))) {
                int z = y * WORLD + x;
                g.seen[z >> 3] |= 1 << (z & 7);
            }
    for (int i = 0; i < old->actorCount; i++) {
        Actor *a = &g.actors[i];
        const LegacyActor *o = &old->actors[i];
        memset(a, 0, sizeof(*a));
        memcpy(a, o, (char *)&o->pack - (char *)o);
        memcpy(a->pack, o->pack, sizeof(o->pack));
        memcpy(a->gear, o->gear, sizeof(o->gear));
        memcpy(&a->active, &o->active, sizeof(*o) - ((char *)&o->active - (char *)o));
        a->prisonTown = -1;
        a->homeTown = town_near(a->x, a->y, 12);
        a->skin = i % 8;
        a->hair = i % 7;
        a->speechCooldown = 15;
        a->gear[4] = gear_id(69, 0, 0);
        if (a->squad)
            a->gear[5] = gear_id(76, 0, 0);
    }
    for (int i = 46; i < expanded->actorCount && g.actorCount < MAX_ACTORS; i++)
        g.actors[g.actorCount++] = expanded->actors[i];
    for (int i = 0; i < old->buildingCount; i++) {
        memset(&g.buildings[i], 0, sizeof(Building));
        memcpy(&g.buildings[i], &old->buildings[i], sizeof(LegacyBuilding));
    }
    for (int i = 35; i < expanded->buildingCount && g.buildingCount < MAX_BUILDINGS; i++)
        g.buildings[g.buildingCount++] = expanded->buildings[i];
    memcpy(g.nodes, old->nodes, sizeof(LegacyNode) * old->nodeCount);
    for (int i = 0; i < expanded->nodeCount && g.nodeCount < MAX_NODES; i++)
        if (expanded->nodes[i].x >= 128 || expanded->nodes[i].y >= 128)
            g.nodes[g.nodeCount++] = expanded->nodes[i];
    free(expanded);
    return validate_game(&g);
}
int load_game(const char *path) {
    content_init();
    FILE *f = fopen(path, "rb");
    if (!f)
        return 0;
    SaveHeader h;
    if (fread(&h, sizeof(h), 1, f) != 1 || memcmp(h.magic, "DUSTSAVE", 8)) {
        fclose(f);
        return 0;
    }
    int ok = 0;
    if (h.version == 2 && h.size == sizeof(Game)) {
        Game *t = malloc(sizeof(Game));
        if (t) {
            ok = fread(t, sizeof(*t), 1, f) == 1 && h.hash == checksum(t, sizeof(*t)) && validate_game(t);
            if (ok)
                g = *t;
            free(t);
        }
    } else if (h.version == 1 && h.size == sizeof(LegacyGame)) {
        LegacyGame *t = malloc(sizeof(LegacyGame));
        if (t) {
            ok = fread(t, sizeof(*t), 1, f) == 1 && h.hash == checksum(t, sizeof(*t));
            if (ok) {
                Game *before = malloc(sizeof(Game));
                UI oldui = ui;
                if (before) {
                    *before = g;
                    ok = migrate_legacy(t);
                    if (!ok) {
                        g = *before;
                        ui = oldui;
                    }
                    free(before);
                } else
                    ok = 0;
            }
            free(t);
        }
    }
    fclose(f);
    if (ok) {
        memset(&ui, 0, sizeof(ui));
        ui.speed = 1;
        ui.slot = 1;
        ui.camX = g.actors[g.selected].x;
        ui.camY = g.actors[g.selected].y;
        log_msg(h.version == 1 ? "Old expedition migrated. Save to a new slot first." : "Expedition loaded.");
    }
    return ok;
}
int save_inspect(int slot, SaveInfo *info) {
    memset(info, 0, sizeof(*info));
    char path[400];
    snprintf(path, sizeof(path), "%s/DUST_%d.sav", saveRoot, slot);
    FILE *f = fopen(path, "rb");
    if (!f)
        return 0;
    info->exists = 1;
    SaveHeader h;
    if (fread(&h, sizeof(h), 1, f) != 1 || memcmp(h.magic, "DUSTSAVE", 8)) {
        fclose(f);
        return 0;
    }
    info->version = h.version;
    info->valid =
        (h.version == 2 && h.size == sizeof(Game)) || (h.version == 1 && h.size == sizeof(LegacyGame));
    if (info->valid) {
        unsigned char *buf = malloc(h.size);
        if (buf && fread(buf, h.size, 1, f) == 1 && checksum(buf, h.size) == h.hash) {
            if (h.version == 2) {
                Game *v = (Game *)buf;
                if (validate_game(v)) {
                    info->day = v->day;
                    info->money = v->money;
                    info->squad = v->squadCount;
                    snprintf(info->name, 16, "%.15s", v->actors[v->selected].name);
                } else
                    info->valid = 0;
            } else {
                LegacyGame *v = (LegacyGame *)buf;
                if (v->selected >= 0 && v->selected < v->actorCount && v->actorCount <= 80) {
                    info->day = v->day;
                    info->money = v->money;
                    info->squad = v->squadCount;
                    snprintf(info->name, 16, "%.15s", v->actors[v->selected].name);
                } else
                    info->valid = 0;
            }
        } else
            info->valid = 0;
        free(buf);
    }
    fclose(f);
    return info->valid;
}
void save_refresh(void) {
    for (int i = 0; i < SAVE_SLOTS; i++)
        save_inspect(i, &ui.saves[i]);
}
int save_slot(int n) {
    char p[400];
    snprintf(p, sizeof(p), "%s/DUST_%d.sav", saveRoot, n);
    int ok = save_game(p);
    log_msg(ok ? "Saved to slot %d." : "Save failed in slot %d.", n);
    return ok;
}
int load_slot(int n) {
    char p[400];
    snprintf(p, sizeof(p), "%s/DUST_%d.sav", saveRoot, n);
    int ok = load_game(p);
    if (!ok)
        log_msg("Slot %d is empty or damaged.", n);
    return ok;
}
static void open_screen(int s) {
    ui.screen = s;
    ui.selection = ui.scroll = 0;
}
static void cycle_squad(int direction) {
    int n = g.selected;
    for (int i = 0; i < g.actorCount; i++) {
        n = (n + direction + g.actorCount) % g.actorCount;
        if (g.actors[n].squad && !g.actors[n].dead) {
            g.selected = n;
            ui.waypoint = 0;
            return;
        }
    }
}
void game_tick(float dt, Input in) {
    dt = clamp(dt, 0, .1f);
    ui.noticeTime = fmaxf(0, ui.noticeTime - dt);
    int p = in.pressed;
    if (expansion_input(dt, in))
        return;
    Actor *a = &g.actors[g.selected];
    if (ui.screen == SCREEN_TITLE) {
        if (p & KEY_UP)
            ui.selection = (ui.selection + 3) % 4;
        if (p & KEY_DOWN)
            ui.selection = (ui.selection + 1) % 4;
        if (p & KEY_CROSS) {
            if (ui.selection == 0 || ui.selection == 1) {
                character_begin(ui.selection == 1 ? 4 : 0);
            } else if (ui.selection == 2) {
                ui.saveMode = 0;
                save_refresh();
                open_screen(SCREEN_SAVES);
            } else
                open_help(0);
        }
        g.time += dt;
        return;
    }
    if (ui.screen == SCREEN_CONFIRM) {
        if (p & KEY_CIRCLE)
            open_screen(SCREEN_MENU);
        if (p & KEY_CROSS) {
            if (ui.confirmAction == 1) {
                character_begin(0);
            } else if (ui.confirmAction == 2) {
                load_slot(ui.slot);
            } else if (ui.confirmAction == 3) {
                save_slot(ui.slot);
                open_screen(SCREEN_MENU);
            }
        }
        return;
    }
    if (ui.screen != SCREEN_WORLD) {
        if ((p & KEY_CIRCLE) || (p & KEY_START)) {
            open_screen(SCREEN_WORLD);
            return;
        }
        int count = 1;
        switch (ui.screen) {
        case SCREEN_PACK:
            count = PACK;
            break;
        case SCREEN_SKILLS:
            count = SKILLS;
            break;
        case SCREEN_HEALTH:
            count = LIMBS;
            break;
        case SCREEN_SQUAD:
            count = 8;
            break;
        case SCREEN_TRADE:
            count = ui.sell ? PACK : shop_count();
            if (count < 1)
                count = 1;
            break;
        case SCREEN_BUILD:
            count = buildCount;
            break;
        case SCREEN_CRAFT:
            count = recipeCount;
            break;
        case SCREEN_MENU:
            count = 16;
            break;
        case SCREEN_HELP:
            count = GUIDE_PAGES;
            break;
        default:
            break;
        }
        if (ui.screen != SCREEN_RECRUIT) {
            if (p & KEY_UP)
                ui.selection = (ui.selection + count - 1) % count;
            if (p & KEY_DOWN)
                ui.selection = (ui.selection + 1) % count;
        }
        if (ui.screen >= SCREEN_PACK && ui.screen <= SCREEN_SQUAD) {
            if (p & KEY_R)
                open_screen(ui.screen == SCREEN_SQUAD ? SCREEN_PACK : ui.screen + 1);
            if (p & KEY_L)
                open_screen(ui.screen == SCREEN_PACK ? SCREEN_SQUAD : ui.screen - 1);
        }
        if (ui.screen == SCREEN_PACK) {
            if (p & KEY_CROSS)
                use_item(ui.selection);
            if (p & KEY_SQUARE) {
                Stack *s = &a->pack[ui.selection];
                if (s->item) {
                    int recipient = -1;
                    for (int j = 1; j <= g.actorCount; j++) {
                        int n = (g.selected + j) % g.actorCount;
                        if (n != g.selected && g.actors[n].squad && !g.actors[n].dead &&
                            distance(a->x, a->y, g.actors[n].x, g.actors[n].y) < 4) {
                            recipient = n;
                            break;
                        }
                    }
                    if (recipient >= 0 && inventory_add(&g.actors[recipient], s->item, 1)) {
                        log_msg("Passed %s to %s.", itemDefs[s->item].name, g.actors[recipient].name);
                        inventory_remove(a, s->item, 1);
                    } else
                        log_msg("No nearby ally can take this item.");
                }
            }
        } else if (ui.screen == SCREEN_HEALTH && (p & KEY_CROSS))
            heal_actor(g.selected, g.selected);
        else if (ui.screen == SCREEN_SQUAD) {
            if (p & KEY_LEFT)
                cycle_squad(-1);
            if (p & KEY_RIGHT)
                cycle_squad(1);
            if (p & KEY_CROSS) {
                a = &g.actors[g.selected];
                a->job = ui.selection;
                a->taskKind = 0;
                log_msg("%s: %s.", a->name, jobNames[a->job]);
            }
        } else if (ui.screen == SCREEN_MAP) {
            float sp = 65 * dt;
            if (in.held & KEY_LEFT)
                ui.mapX -= sp;
            if (in.held & KEY_RIGHT)
                ui.mapX += sp;
            if (in.held & KEY_UP)
                ui.mapY -= sp;
            if (in.held & KEY_DOWN)
                ui.mapY += sp;
            ui.mapX = clamp(ui.mapX, 3, WORLD - 4);
            ui.mapY = clamp(ui.mapY, 3, WORLD - 4);
            if (p & KEY_CROSS) {
                ui.waypoint = 1;
                open_screen(SCREEN_WORLD);
                log_msg("Route set. Move manually to cancel.");
            }
        } else if (ui.screen == SCREEN_TRADE) {
            if (p & (KEY_LEFT | KEY_RIGHT)) {
                ui.sell = !ui.sell;
                ui.selection = 0;
            }
            if (p & KEY_CROSS) {
                if (ui.sell)
                    sell_item(ui.tradeTown, ui.selection);
                else
                    buy_item(ui.tradeTown, shop_item(ui.selection));
            }
        } else if (ui.screen == SCREEN_RECRUIT && (p & KEY_CROSS)) {
            if (recruit_actor(ui.selection))
                open_screen(SCREEN_WORLD);
        } else if (ui.screen == SCREEN_BUILD && (p & KEY_CROSS))
            build_structure(ui.selection);
        else if (ui.screen == SCREEN_CRAFT && (p & KEY_CROSS))
            craft_item(ui.selection);
        else if (ui.screen == SCREEN_RESEARCH && (p & KEY_CROSS))
            research_tech();
        else if (ui.screen == SCREEN_MENU) {
            if (p & (KEY_LEFT | KEY_RIGHT))
                ui.slot = (ui.slot + 1) % SAVE_SLOTS;
            if (p & KEY_CROSS) {
                switch (ui.selection) {
                case 0:
                    open_screen(SCREEN_WORLD);
                    break;
                case 1:
                    open_screen(SCREEN_BUILD);
                    break;
                case 2:
                    open_screen(SCREEN_CRAFT);
                    break;
                case 3:
                    open_screen(SCREEN_RESEARCH);
                    break;
                case 4:
                    ui.saveMode = 1;
                    save_refresh();
                    open_screen(SCREEN_SAVES);
                    break;
                case 5:
                    ui.saveMode = 0;
                    save_refresh();
                    open_screen(SCREEN_SAVES);
                    break;
                case 6:
                    ui.speed = ui.speed == 1 ? 2 : 1;
                    break;
                case 7:
                    open_help(0);
                    break;
                case 8:
                    ui.confirmAction = 1;
                    open_screen(SCREEN_CONFIRM);
                    break;
                case 9:
                    if (save_slot(ui.slot ? ui.slot : 1))
                        open_screen(SCREEN_TITLE);
                    break;
                case 11:
                    open_screen(SCREEN_FORGE);
                    break;
                case 12:
                    open_screen(SCREEN_JOURNAL);
                    break;
                case 13:
                    open_screen(SCREEN_FACTIONS);
                    break;
                case 14:
                    if (town_near(a->x, a->y, 12) >= 0) {
                        character_begin(-1);
                    } else
                        log_msg("A town barber is required.");
                    break;
                case 15:
                    start_sleep(town_near(a->x, a->y, 10));
                    break;
                case 10:
                    if (town_near(a->x, a->y, 10) < 0) {
                        log_msg("Visit a town official to pay a bounty.");
                        break;
                    }
                    if (a->bounty && g.money >= a->bounty) {
                        g.money -= a->bounty;
                        a->bounty = 0;
                        g.rep[UNION] = g.rep[SCAVENGERS] = 0;
                        log_msg("Bounty paid. Reputation restored.");
                    } else
                        log_msg("No bounty, or insufficient silver.");
                    break;
                }
            }
        }
        return;
    }
    if (p & KEY_TRIANGLE) {
        open_screen(SCREEN_PACK);
        return;
    }
    if (p & KEY_START) {
        open_screen(SCREEN_MENU);
        return;
    }
    if (p & KEY_SELECT) {
        ui.mapX = a->x;
        ui.mapY = a->y;
        open_screen(SCREEN_MAP);
        return;
    }
    if (p & KEY_L) {
        cycle_squad(1);
        a = &g.actors[g.selected];
    }
    if (p & KEY_R)
        ui.paused = !ui.paused;
    if (p & KEY_CIRCLE) {
        a->sneaking = !a->sneaking;
        log_msg(a->sneaking ? "Sneaking. Detection range reduced." : "Standing. Normal movement.");
    }
    if (p & KEY_SQUARE) {
        a->autoattack = !a->autoattack;
        log_msg(a->autoattack ? "Combat stance: attack nearby hostiles."
                              : "Passive stance: flee or move freely.");
    }
    if (p & KEY_CROSS) {
        if (a->carrying >= 0) {
            g.actors[a->carrying].carriedBy = -1;
            a->carrying = -1;
            log_msg("Put down carried squad member.");
        } else
            interact();
    }
    if (ui.screen != SCREEN_WORLD || ui.paused)
        return;
    float dx = in.ax, dy = in.ay;
    if (in.held & KEY_LEFT)
        dx = -1;
    if (in.held & KEY_RIGHT)
        dx = 1;
    if (in.held & KEY_UP)
        dy = -1;
    if (in.held & KEY_DOWN)
        dy = 1;
    float step = dt * ui.speed;
    if (fabsf(dx) + fabsf(dy) > .15f) {
        ui.waypoint = 0;
        a->taskKind = 0;
        move_actor(a, (dx + dy) * .7071f, (dy - dx) * .7071f, step);
    } else if (ui.waypoint) {
        if (distance(a->x, a->y, ui.mapX, ui.mapY) < .7f) {
            ui.waypoint = 0;
            log_msg("Waypoint reached.");
        } else
            route_toward(a, ui.mapX, ui.mapY, step);
    }
    simulate(step);
    g.autosave += step;
    if (g.autosave > 90 && a->target < 0) {
        g.autosave = 0;
        save_slot(0);
    }
    find_context();
}
void debug_snapshot(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f)
        return;
    Actor *a = &g.actors[g.selected];
    fprintf(f, "{\"version\":\"%s\",\"screen\":%d,\"frames\":%u", VERSION, ui.screen,
            (unsigned)rendered_frames);
#define PTR(n, p) fprintf(f, ",\"%s_pointer\":%u", n, (unsigned)(uintptr_t)(p))
    PTR("display", &display_buffer);
    PTR("screen", &ui.screen);
    PTR("money", &g.money);
    PTR("squad", &g.squadCount);
    PTR("position", &a->x);
    PTR("gear", a->gear);
    PTR("pack", a->pack);
    PTR("health", a->health);
    PTR("attack", &a->attackPhase);
    PTR("time", &g.time);
    PTR("framecount", &rendered_frames);
    PTR("selected", &g.selected);
    PTR("selection", &ui.selection);
    PTR("weather", &g.weather);
    PTR("bounty", &a->bounty);
    PTR("contracts", &g.contractsDone);
    PTR("game", &g);
#undef PTR
    fprintf(f, "}\n");
    fclose(f);
}
