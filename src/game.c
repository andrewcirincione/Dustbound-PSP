#include "game.h"
#include <stddef.h>
#include "combat_pose.h"
#include "legacy.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Game g;
/* A running attack keeps its chosen motion and timings until it finishes.
   This side table leaves the binary save layout unchanged. */
typedef struct {
    float stance;
    CombatTiming timing;
    uint32_t entropy;
    unsigned played;
    int variant;
    float walkX, walkY;
} CombatVisual;
static CombatVisual combatVisuals[MAX_ACTORS];
typedef struct { float x, y, time, stuck; } Retreat;
static Retreat retreats[MAX_ACTORS];
void combat_visual_reset(void) {
    memset(combatVisuals, 0, sizeof(combatVisuals));
    memset(retreats, 0, sizeof(retreats));
}
static const CombatVisual *combat_visual(const Actor *a) {
    uintptr_t address = (uintptr_t)a, start = (uintptr_t)g.actors;
    return address >= start && address < start + sizeof(g.actors)
        ? &combatVisuals[(address - start) / sizeof(Actor)] : NULL;
}
float combat_stance(const Actor *a) {
    const CombatVisual *visual = combat_visual(a);
    return visual ? visual->stance : a->attackPhase != 0;
}
int combat_attack_variant(const Actor *a) {
    const CombatVisual *visual = combat_visual(a);
    return visual && a->attackPhase ? visual->variant : 0;
}
static int next_attack_variant(int actor) {
    CombatVisual *visual = &combatVisuals[actor];
    const unsigned all = (1u << ATTACK_VARIANTS) - 1;
    unsigned choices = all & ~visual->played;
    if (!choices) {
        visual->played = 0;
        choices = all & ~(1u << visual->variant);
    }
    /* Shuffle locally so choosing a pose never consumes world or damage RNG. */
    uint32_t roll = visual->entropy;
    if (!roll)
        roll = (g.seed ^ ((uint32_t)(actor + 1) * 0x9e3779b9u)) | 1u;
    roll ^= roll << 13;
    roll ^= roll >> 17;
    roll ^= roll << 5;
    visual->entropy = roll;
    unsigned pick = roll % __builtin_popcount(choices);
    for (int i = 0; i < ATTACK_VARIANTS; i++) {
        if (!(choices & (1u << i)))
            continue;
        if (pick == 0) {
            visual->played |= 1u << i;
            return i;
        }
        pick--;
    }
    return 0;
}
UI ui;
volatile uint32_t display_buffer = 0x04000000;
volatile uint32_t rendered_frames = 0;
static char saveRoot[256] = ".";
const char *skillNames[SKILLS] = {
    "Agility",      "Strength", "Dexterity",   "Melee attack", "Melee defence",  "Toughness", "Dodge",
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
    {"Field dressing", "Stops bleeding; restores injured limbs. \001 to treat.", 65, 1, -1, 0, 0, 0xd8d2ad},
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
    {"Hand tools", "Reusable for engineering. Improvised picks are enough for any lock.", 90, 1, -1, 0, 0, 0xb5ada0},
    {"Old-world relic", "Sell for silver or spend on research.", 300, 2, -1, 0, 0, 0x80c8be},
    {"Cartridge", "Ammunition for frontier guns.", 10, 0, -1, 0, 0, 0xac925b},
    {"Cured hide", "Leatherworking and trade material.", 24, 1, -1, 0, 0, 0x78553b},
    {"Coal", "High-grade forging fuel.", 18, 2, -1, 0, 0, 0x34393a},
    {"Machine parts", "Salvage for advanced equipment.", 90, 2, -1, 0, 0, 0x698078},
    {"Blood tonic", "Restores blood and warmth over time.", 110, 1, -1, 0, 0, 0x9c5141},
    {"Survey chart", "Reveals settlement positions and terrain.", 180, 0, -1, 0, 0, 0xbea777}};
Town towns[TOWNS] = {{"DUSK HAVEN", 26, 27, UNION},       {"COPPER CROSS", 76, 32, SCAVENGERS},
                     {"SALTWATCH", 32, 83, UNION},        {"THE DRY MARKET", 91, 88, SCAVENGERS},
                     {"BROKEN SPIRE", 61, 62, HOSTILE},   {"RED GULCH", 155, 42, UNION},
                     {"IRON GATE", 235, 71, SHOGUN},      {"ASH MONASTERY", 322, 36, MONKS},
                     {"BLACK MESA", 165, 150, OUTLAWS},   {"REED CROSSING", 65, 215, SCAVENGERS},
                     {"CINDER PORT", 138, 294, UNION},    {"SHOGUN'S REST", 278, 198, SHOGUN},
                     {"PALE PASS", 307, 322, MONKS},      {"DUSK FERRY", 228, 281, SCAVENGERS},
                     {"VULTURE NEST", 338, 135, HOSTILE}, {"PINE REFUGE", 48, 333, MONKS},
                     {"BRASS JUNCTION", 244, 250, UNION},  {"VERMILION KEEP", 350, 245, SHOGUN},
                     {"CINDER ABBEY", 277, 359, MONKS}};

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
const char *travel_hold_reason(const Actor *a) {
    if (a->carriedBy >= 0)
        return "BEING CARRIED";
    if (a->unconscious > 0)
        return "UNCONSCIOUS";
    if (a->prisonTime > 0)
        return "IN CUSTODY";
    if (a->stun > 0)
        return "STUNNED";
    if (g.hitstop > 0 || a->attackPhase == 1 || a->attackPhase == 2)
        return "COMBAT";
    return NULL;
}
int level(const Actor *a, int s) { return (int)clamp(1 + sqrtf(a->xp[s] / 12), 1, 100); }
void gain(Actor *a, int s, float n) {
    if (s < 0 || s >= SKILLS || !a->squad)
        return;
    int before = level(a, s);
    a->xp[s] = fminf(117612, a->xp[s] + n);
    if (level(a, s) > before)
        log_level("%s reached %s %d.", a->name, skillNames[s], level(a, s));
}
static void log_write(int priority, const char *fmt, va_list ap) {
    char b[100]; vsnprintf(b,sizeof(b),fmt,ap);
    if (!ui.logCount || strncmp(ui.logs[0], b, sizeof(ui.logs[0]) - 1)) {
        for(int i=LOGS-1;i>0;i--) memcpy(ui.logs[i],ui.logs[i-1],80);
        snprintf(ui.logs[0],80,"%.79s",b);
        if (ui.logCount < LOGS) ui.logCount++;
    }
    if(priority<0){if(ui.feedCount)return;priority=0;}
    snprintf(ui.notice,sizeof(ui.notice),"%s",b);ui.noticeTime=4;
    for(int i=0;i<ui.feedCount;i++) if(!strcmp(ui.feed[i],b)) return;
    int at=ui.feedCount;
    if (priority) {
        at = 0;
        while (at < ui.feedCount && ui.feedPriority[at] >= priority) at++;
    }
    if(at>=12)return;
    if(ui.feedCount<12)ui.feedCount++;
    for(int i=ui.feedCount-1;i>at;i--){memcpy(ui.feed[i],ui.feed[i-1],100);ui.feedPriority[i]=ui.feedPriority[i-1];}
    snprintf(ui.feed[at],100,"%s",b);ui.feedPriority[at]=priority;
    if (!at) ui.feedClock = ui.feedScroll = 0;
}
void log_msg(const char *fmt, ...) { va_list ap;va_start(ap,fmt);log_write(0,fmt,ap);va_end(ap); }
void log_progress(const char *fmt, ...) { va_list ap;va_start(ap,fmt);log_write(-1,fmt,ap);va_end(ap); }
void log_level(const char *fmt, ...) { va_list ap;va_start(ap,fmt);log_write(1,fmt,ap);va_end(ap); }
void log_warn(const char *fmt, ...) { va_list ap;va_start(ap,fmt);log_write(2,fmt,ap);va_end(ap); }
enum { TICKER_GAP = 20 };
static int ticker_width(const char *s) {
    int width = 0;
    for (; *s; s++)
        width += *s == '\007' ? 24 : *s == '\010' ? 28 : (unsigned char)*s < 16 ? 10 : 6;
    return width;
}
static void danger_news(void) {
    unsigned char danger[MAX_ACTORS] = {0};
    for (int i = 0; i < g.actorCount; i++) {
        Actor *foe = &g.actors[i];
        int n = foe->target;
        if (n >= 0 && n < g.actorCount && !foe->squad && conscious(foe) &&
            g.actors[n].squad && !g.actors[n].dead && depth_distance(foe, &g.actors[n]) < 12 &&
            hostile(foe, &g.actors[n]))
            danger[n] |= 4;
    }
    for (int n = 0; n < g.actorCount; n++) {
        Actor *a = &g.actors[n];
        if (!a->squad || a->dead)
            continue;
        if (a->bleed > .01f) danger[n] |= 1;
        if (a->unconscious > 0) danger[n] |= 2;
        if (recovery_coma(a)) danger[n] |= 16;
        if (a->blood < 35 || a->health[HEAD] < 15 || a->health[CHEST] < 15) danger[n] |= 8;
        int added = danger[n] & ~ui.danger[n];
        if (added & 16)
            log_warn("%s is in a recovery coma.", a->name);
        else if (added & 2)
            log_warn("%s is unconscious.", a->name);
        if (added & 1)
            log_warn("%s is bleeding. Nearby Actions has first aid.", a->name);
        if ((added & 8) && !(danger[n] & 2))
            log_warn("%s is critically wounded.", a->name);
        else if (added == 4 && !(danger[n] & 3))
            log_warn("%s is under attack.", a->name);
    }
    memcpy(ui.danger, danger, sizeof(danger));
}
void log_tick(float dt) {
    ui.statusClock += dt;
    if (ui.feedCount) {
        ui.feedClock += dt;
        float dwell = ui.feedCount == 1 ? 1.05f : ui.feedCount < 4 ? .40f : .16f;
        float speed = fminf(390, 96 + (ui.feedCount - 1) * 39);
        if (ui.feedClock > dwell)
            ui.feedScroll += dt * speed;
        if (ui.feedScroll >= ticker_width(ui.feed[0]) + TICKER_GAP) {
            for (int i = 1; i < ui.feedCount; i++) {
                memcpy(ui.feed[i - 1], ui.feed[i], 100);
                ui.feedPriority[i - 1] = ui.feedPriority[i];
            }
            ui.feedCount--;
            ui.feedClock = ui.feedScroll = 0;
        }
    }
    if (ui.screen == SCREEN_TITLE || ui.screen == SCREEN_SCENARIOS || !g.actorCount)
        return;
    if (ui.screen == SCREEN_WORLD)
        danger_news();
    Actor*a=&g.actors[g.selected];int t=town_near(a->x,a->y,24),v=t>=0?wanted_amount(a,g.townOwner[t]):0;
    if(v>0&&(t!=ui.wantedTown||v!=ui.wantedValue))log_warn("%s: wanted by %s, %d silver. Guards may arrest you.",towns[t].name,factionNames[g.townOwner[t]],v);
    ui.wantedTown=t;ui.wantedValue=v;
}
const char *log_current(int *priority,int *offset) {
    static char status[100];*offset=0;
    if(ui.wantedValue>0&&ui.wantedTown>=0&&!ui.feedCount){
        snprintf(status,sizeof(status),"WANTED HERE: %s / %d silver. See Start > World > Factions.",factionNames[g.townOwner[ui.wantedTown]],ui.wantedValue);*priority=2;return status;}
    if(!ui.feedCount)return NULL;
    *priority = ui.feedPriority[0];
    *offset = (int)ui.feedScroll;
    return ui.feed[0];
}
int inventory_count(const Actor *a, int item) {
    int n = 0;
    for (int i = 0; i < PACK; i++)
        if (a->pack[i].item == item)
            n += a->pack[i].qty;
    return n;
}
int inventory_add_owned(Actor *a, int item, int qty, int owner) {
    if (item <= 0 || item >= ITEMS || qty <= 0 || qty > 999 || owner < 0 || owner >= FACTIONS)
        return 0;
    for (int i = 0; i < PACK; i++)
        if (a->pack[i].item == item && a->stolen[i] == owner && a->pack[i].qty + qty <= 999) {
            a->pack[i].qty += qty;
            return 1;
        }
    for (int i = 0; i < PACK; i++)
        if (!a->pack[i].item) {
            a->pack[i] = (Stack){item, qty};
            a->stolen[i] = owner;
            return 1;
        }
    return 0;
}
int inventory_add(Actor *a, int item, int qty) { return inventory_add_owned(a, item, qty, 0); }
int inventory_remove_slot(Actor *a, int slot, int qty) {
    if (slot < 0 || slot >= PACK || qty <= 0 || !a->pack[slot].item || a->pack[slot].qty < qty)
        return 0;
    a->pack[slot].qty -= qty;
    if (!a->pack[slot].qty) {
        a->pack[slot].item = 0;
        a->stolen[slot] = 0;
    }
    return 1;
}
int inventory_transfer(Actor *from, Actor *to, int slot, int qty) {
    if (from == to || slot < 0 || slot >= PACK || qty <= 0 || from->pack[slot].qty < qty)
        return 0;
    if (!inventory_add_owned(to, from->pack[slot].item, qty, from->stolen[slot]))
        return 0;
    return inventory_remove_slot(from, slot, qty);
}
int pass_recipient(int row) {
    for (int n = 0; n < g.actorCount; n++)
        if (n != g.selected && g.actors[n].active && g.actors[n].squad && row-- == 0)
            return n;
    return -1;
}
const char *pass_blocked(int n) {
    if (n < 0)
        return "No other squad members";
    const Actor *a = &g.actors[g.selected], *b = &g.actors[n];
    if (!conscious(a))
        return "Sender unavailable";
    if (b->dead)
        return "Fallen";
    if (b->carriedBy >= 0 || g.depth.actors[n].captive || b->prisonTime > 0)
        return "Unavailable";
    if (depth_room(a) != depth_room(b))
        return "Different room";
    if (depth_distance(a, b) > 3)
        return "Too far away";
    const Stack *s = &a->pack[ui.passSlot];
    Actor test = *b;
    if (!inventory_add_owned(&test, s->item, 1, a->stolen[ui.passSlot]))
        return "Pack full";
    return NULL;
}
int inventory_input_owner(const Actor *a, int item, int qty) {
    for (int i = 0; i < PACK && qty > 0; i++)
        if (a->pack[i].item == item) {
            int taken = a->pack[i].qty < qty ? a->pack[i].qty : qty;
            if (taken && a->stolen[i])
                return a->stolen[i];
            qty -= taken;
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
    w *= backpack_load_percent(a->gear[5]) * .01f;
    for (int i = 0; i < GEARS; i++)
        w += itemDefs[a->gear[i]].weight;
    if((uintptr_t)a>=(uintptr_t)g.actors && (uintptr_t)a<(uintptr_t)(g.actors+MAX_ACTORS))
        w += itemDefs[g.depth.actors[a-g.actors].layer].weight;
    for(int l=LARM;l<=RLEG;l++) w+=itemDefs[limb_installed(a,l)].weight;
    if (a->carrying >= 0)
        w += 40;
    return w;
}
float backpack_load_preview(const Actor *a, int item) {
    if (item == a->gear[5])
        return encumbrance(a);
    float cargo = 0, worn = itemDefs[item].weight + (a->carrying >= 0 ? 40 : 0);
    for (int i = 0; i < PACK; i++)
        cargo += itemDefs[a->pack[i].item].weight * a->pack[i].qty;
    if (inventory_count(a, item))
        cargo -= itemDefs[item].weight;
    cargo += itemDefs[a->gear[5]].weight;
    for (int i = 0; i < GEARS - 1; i++)
        worn += itemDefs[a->gear[i]].weight;
    if((uintptr_t)a>=(uintptr_t)g.actors && (uintptr_t)a<(uintptr_t)(g.actors+MAX_ACTORS))
        worn += itemDefs[g.depth.actors[a-g.actors].layer].weight;
    for(int l=LARM;l<=RLEG;l++)worn+=itemDefs[limb_installed(a,l)].weight;
    return worn + cargo * backpack_load_percent(item) * .01f;
}
float movement_speed(const Actor *a) {
    float cap = actor_capacity(a);
    float f = 1.0f / (1 + fmaxf(0, encumbrance(a) - cap) / cap);
    if (limb_crawling(a))
        f *= !limb_missing(a, LLEG) && !limb_missing(a, RLEG) ? .14f :
             limb_missing(a, LLEG) && limb_missing(a, RLEG) ? .12f : .21f;
    else if (limb_limping(a))
        f *= a->health[LLEG] <= 0 || a->health[RLEG] <= 0 ? .37f : .53f;
    if (a->hunger < 20)
        f *= .7f;
    if (a->fatigue > 85)
        f *= .75f;
    if (a->carrying >= 0)
        f *= .75f;
    return (1.92f + (level(a, ATHLETICS) - 1) * .037f) * f * (a->sneaking ? .55f : 1) *
           (1 + gear_bonus(a, 1) * .005f) * speciesDefs[species_index(a)].move * limb_move_bonus(a);
}
int town_near(float x, float y, float r) {
    for (int i = 0; i < TOWNS; i++) {
        if (fabsf(x - towns[i].x) >= r || fabsf(y - towns[i].y) >= r)
            continue;
        if (distance(x, y, towns[i].x, towns[i].y) < r)
            return i;
    }
    return -1;
}
int can_walk(float x, float y) {
    if (x < 2 || y < 2 || x > WORLD - 3 || y > WORLD - 3)
        return 0;
    if (trail_blocked(x,y) || terrain_water(x, y))
        return 0;
    for (int i = 0; i < g.buildingCount; i++) {
        Building *b = &g.buildings[i];
        if (b->type == HOUSE || b->type == SHOP || b->type == BAR || b->type == TOWER || b->type == WALL) {
            float r = b->type == TOWER ? .8f : b->type == WALL ? 1.01f : 1.25f;
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
    if (fabsf(dx - dy) > .08f)
        a->facing = dx > dy;
    float sp = movement_speed(a) * dt;
    float ox = a->x, oy = a->y;
    int captive=(uintptr_t)a>=(uintptr_t)g.actors&&(uintptr_t)a<(uintptr_t)(g.actors+g.actorCount)?g.depth.actors[a-g.actors].captive:0;
    if (!captive && can_walk(a->x + dx * sp, a->y) && society_gate(a,a->x+dx*sp,a->y))
        a->x += dx * sp;
    if (!captive && can_walk(a->x, a->y + dy * sp) && society_gate(a,a->x,a->y+dy*sp))
        a->y += dy * sp;
    float moved = distance(ox, oy, a->x, a->y);
    if (moved > .001f) {
        walk_motion(a, a->x - ox, a->y - oy);
        gain(a, STRENGTH, moved * encumbrance(a) * .035f);
        if (a->sneaking)
            gain(a, STEALTH, moved * 1.5f);
        a->fatigue = clamp(a->fatigue + dt * .016f, 0, 100);
    }
}
void walk_motion(Actor *a, float dx, float dy) {
    float moved = sqrtf(dx * dx + dy * dy);
    if (moved <= .001f)
        return;
    float x = view_horizontal(dx, dy) / moved, y = view_vertical(dx, dy) / moved;
    if ((uintptr_t)a >= (uintptr_t)g.actors && (uintptr_t)a < (uintptr_t)(g.actors + MAX_ACTORS)) {
        CombatVisual *v = &combatVisuals[a - g.actors];
        v->walkX = x;
        v->walkY = y;
    }
    if (fabsf(x) > .10f)
        a->facing = x > 0;
    float rate = a->kind == BEAST || limb_crawling(a) ? 2.2f : limb_limping(a) ? 4.5f : a->sneaking ? 7.4f : 5.2f;
    a->anim = fmodf(a->anim + moved * rate, 6.2831853f);
    a->movedTimer = .12f;
    gain(a, ATHLETICS, moved * 2);
}
void walk_heading(const Actor *a, float *x, float *y) {
    const CombatVisual *v = combat_visual(a);
    if (v && (v->walkX || v->walkY)) {
        *x = v->walkX;
        *y = v->walkY;
    } else {
        *x = a->facing ? 1 : -1;
        *y = 0;
    }
}
void init_actor(int n, const char *name, int kind, int faction, float x, float y, int skill) {
    combatVisuals[n] = (CombatVisual){0};
    retreats[n] = (Retreat){0};
    g.townLife.squadHitAt[n] = 0;
    ui.danger[n] = ui.jobGather[n] = ui.priorityJob[n] = 0;
    society_actor_reset(n);
    memset(&g.depth.actors[n], 0, sizeof(g.depth.actors[n]));
    g.depth.actors[n].upkeep = 100;
    for (int j = 0; j < 4; j++) g.depth.actors[n].queue[j] = JOB_HOLD;
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
        a->health[i] = species_health(a);
    for (int i = 0; i < SKILLS; i++)
        a->xp[i] = 12 * (skill - 1) * (skill - 1);
    a->gear[0] = kind == BEAST ? NONE : kind == GUARD ? SABRE : kind == RAIDER ? RUSTSWORD : STAFF;
    a->gear[1] = kind == BEAST ? NONE : kind == GUARD ? PLATE : kind == RAIDER ? LEATHER : DUSTER;
    a->gear[2] = kind == GUARD ? HELM : kind == RAIDER ? HAT : STRAW;
    a->gear[3] = BOOTS;
    inventory_add(a, RATION, 2);
    if (kind == RAIDER)
        inventory_add(a, COPPER, 2);
    else if ((kind == SETTLER || kind == TRADER || kind == RECRUIT || kind == CARAVAN) &&
             (g.seed + (unsigned)n * 11) % 5 == 0)
        inventory_add(a, MEDKIT, 1);
    if (kind == BEAST) {
        memset(a->gear, 0, sizeof(a->gear));
        memset(a->pack, 0, sizeof(a->pack));
        inventory_add(a, MEAT, 3);
    }
    return;
}
int add_actor(const char *name, int kind, int faction, float x, float y, int skill) {
    if (g.actorCount >= MAX_ACTORS) return -1;
    int n = g.actorCount++;
    init_actor(n, name, kind, faction, x, y, skill);
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
void restock_town(int t) {
        memset(g.stock[t], 0, sizeof(g.stock[t]));
        for (int j = 1; j < BASE_ITEMS; j++)
            g.stock[t][j] = j <= 10 ? 16 : (j == BOLTS || j == CARTRIDGE ? 60 : j == RELIC ? 1 : 3);
        for (int j = 0; j < 36; j++) {
            int f = (t * 11 + j * 7 + g.day * 3) % FAMILIES;
            int m = (t + j / 6 + g.day) % MATERIALS, q = (j + t + g.day) % QUALITIES;
            g.stock[t][gear_id(f, m, q)] = 1 + (j % 3);
        }
    frontier_restock(t);regional_restock(t);society_stock(t);civil_stock(t);limbs_stock(t);
}
void restock(void) {
    for (int t = 0; t < TOWNS; t++) restock_town(t);
    g.lastRestock = g.day;
}
void game_new(uint32_t seed, int scenario) {
    content_init();
    memset(&g, 0, sizeof(g));
    combat_visual_reset();
    memset(&ui, 0, sizeof(ui));
    ui.attackFocus=-1;
    world_layout(0);
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
        float x = 24 + rf() * 112, y = 25 + rf() * 112;
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
        float x = 24 + rf() * 110, y = 25 + rf() * 110;
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
    spread_frontier(&g);
    place_wilderness_encounters();
    frontier_identity();
    depth_init();
    regional_init();
    living_init();
    society_init();
    civil_init();
    limbs_init();
    population_init();
    life_init();
    trail_init();
    war_init();
    town_life_init();
    ui.camX = a->x;
    ui.camY = a->y;
    ui.mapX = a->x;
    ui.mapY = a->y;
    g.discovered[1] = g.discovered[2] = 1;
    find_context();
}

int hostile(const Actor *a, const Actor *b) {
    if (depth_room(a) != depth_room(b))
        return 0;
    if (a->squad && b->squad) return 0;
    if (jail_authority(a, b) || jail_authority(b, a))
        return 0;
    if (g.war.version && war_rivals(a->faction,b->faction)) {
        uintptr_t lo=(uintptr_t)g.actors, hi=(uintptr_t)(g.actors+g.actorCount);
        if (((uintptr_t)a>=lo&&(uintptr_t)a<hi&&g.war.prisonTown[a-g.actors]) ||
            ((uintptr_t)b>=lo&&(uintptr_t)b<hi&&g.war.prisonTown[b-g.actors])) return 0;
        if (a->kind == GUARD && b->kind == GUARD) return 1;
    }
    if(living_hostile(a,b)) return 1;
    int civic=society_hostile(a,b);if(civic>=0)return civic;
    if(regional_hostile(a,b)) return 1;
    /* Escapees remain recognizable to their former captors for one day. */
    if ((uintptr_t)b >= (uintptr_t)g.actors && (uintptr_t)b < (uintptr_t)(g.actors+g.actorCount) &&
        g.depth.actors[b-g.actors].recognized > 0 && (a->kind == GUARD || a->kind == RAIDER) &&
        a->faction == OUTLAWS) return 1;
    if ((uintptr_t)a >= (uintptr_t)g.actors && (uintptr_t)a < (uintptr_t)(g.actors+g.actorCount) &&
        g.depth.actors[a-g.actors].recognized > 0 && (b->kind == GUARD || b->kind == RAIDER) &&
        b->faction == OUTLAWS) return 1;
    if (a->faction == b->faction)
        return 0;
    if (a->faction == HOSTILE || b->faction == HOSTILE || a->faction == WILDLIFE || b->faction == WILDLIFE)
        return 1;
    if ((a->squad || merc_slot(a) >= 0) && b->kind == GUARD && g.rep[b->faction] < -25)
        return 1;
    if ((b->squad || merc_slot(b) >= 0) && a->kind == GUARD && g.rep[a->faction] < -25)
        return 1;
    return 0;
}
static int attackable(const Actor *a) {
    return a->active && !a->dead && a->unconscious <= 0 && a->carriedBy < 0;
}
int combat_slot_available(int ai,int bi){
 int active=0;for(int i=0;i<g.actorCount;i++)if(i!=ai&&g.actors[i].active&&conscious(&g.actors[i])&&g.actors[i].attackTarget==bi&&g.actors[i].attackPhase>=1&&g.actors[i].attackPhase<=2&&!weapon_ranged(g.actors[i].attackWeapon))active++;
 return active<2;
}
void strike(int ai, int bi) {
    if (ai < 0 || bi < 0 || ai >= g.actorCount || bi >= g.actorCount || ai == bi)
        return;
    Actor *a = &g.actors[ai], *b = &g.actors[bi];
    if (depth_room(a) != depth_room(b) || !conscious(a) || !attackable(b) || g.depth.actors[ai].captive || a->cooldown > 0 ||
        a->attackPhase || a->prisonTime > 0)
        return;
    int w = a->gear[0], ranged = weapon_ranged(w);
    if(ai!=g.selected&&!ranged&&!combat_slot_available(ai,bi))return;
    if (!limb_arms(a)) { if(limb_missing(a,LLEG)&&limb_missing(a,RLEG))return; w=NONE; ranged=0; }
    if (depth_distance(a, b) > (w ? itemDefs[w].range : 1.55f))
        return;
    if (ranged && !inventory_count(a, gear_family(w) >= 29 ? CARTRIDGE : BOLTS)) {
        if (inventory_add_owned(a, w, 1, a->gearStolen[0])) {
            a->gear[0] = NONE;
            a->gearStolen[0] = 0;
        }
        if (a->squad)
            log_msg("%s needs ammunition.", a->name);
        return;
    }
    CombatVisual *visual = &combatVisuals[ai];
    visual->variant = a->kind == BEAST ? 0 : next_attack_variant(ai);
    CombatTiming timing = combat_timing(a, w, visual->variant);
    visual->timing = timing;
    a->attackPhase = 1;
    a->attackTarget = bi;
    a->attackWeapon = w;
    a->attackDuration = a->attackClock = timing.windup;
    a->cooldown = timing.windup + timing.stroke + timing.recovery;
    a->taskKind = 0;
    a->attackAngle = atan2f((depth_x(b)+depth_y(b)-depth_x(a)-depth_y(a))*11, (depth_x(b)-depth_y(b)-depth_x(a)+depth_y(a))*22);
    a->facing = cosf(a->attackAngle) >= 0;
    if(a->kind==GUARD&&b->kind==GUARD&&war_rivals(a->faction,b->faction))war_report_clash(a->faction,b->faction,b->x,b->y);
    if (distance(a->x, a->y, g.actors[g.selected].x, g.actors[g.selected].y) < 12)
        audio_event(ranged ? (gear_family(w) >= 29 ? 4 : 1) : 1);
}
static void resolve_strike(int ai, int bi) {
    if (ai < 0 || bi < 0 || ai >= g.actorCount || bi >= g.actorCount)
        return;
    Actor *a = &g.actors[ai], *b = &g.actors[bi];
    if (depth_room(a) != depth_room(b) || !conscious(a) || b->dead || b->unconscious > 0)
        return;
    int w = a->attackWeapon, ranged = weapon_ranged(w);
    if (depth_distance(a, b) > (w ? itemDefs[w].range : 1.55f) + .3f)
        return;
    if (ranged && !inventory_remove(a, gear_family(w) >= 29 ? CARTRIDGE : BOLTS, 1))
        return;
    if (ranged) {
        if (!depth_fire(ai, bi, w)) inventory_add(a, gear_family(w)>=29?CARTRIDGE:BOLTS, 1);
        return;
    }
    combat_impact(ai, bi, w);
}
static void credit_squad_defeat(Actor *a) {
    int n = (int)(a - g.actors);
    if (a->squad || a->defeatCredited || g.townLife.squadHitAt[n] <= g.time)
        return;
    a->defeatCredited = 1;
    g.townLife.squadHitAt[n] = 0;
    g.kills++;
    contract_defeat(a);
    if (a->bountyReward)
        log_msg("Bounty target down. Carry them to claim a named bounty.");
}
void combat_impact(int ai, int bi, int w) {
    if(ai<0||bi<0||ai>=g.actorCount||bi>=g.actorCount)return;
    Actor *a=&g.actors[ai], *b=&g.actors[bi];
    if (b->dead || b->unconscious > 0 || jail_authority(a, b)) return;
    int ranged=weapon_ranged(w);
    int sk = ranged ? CROSSBOWS : w ? MELEE : MARTIAL;
    int evading=combat_evasion(b),defSkill=evading?DODGE:DEFENCE;
    float atk = level(a, sk) + gear_bonus(a, 0), def = level(b,defSkill);
    if(evading)def*=dodge_mobility(b);
    if (!ranged)
        atk -= backpack_combat_penalty(a->gear[5]);
    def -= backpack_combat_penalty(b->gear[5]);
    if(g.depth.actors[bi].order==ORDER_BLOCK) def+=18;
    for (int e = 0; e < GEARS; e++)
        if (itemDefs[b->gear[e]].effect == 4)
            def += itemDefs[b->gear[e]].potency;
    if (ranged)
        atk += level(a, PERCEPTION) * .15f;
    if (ranged && g.weather)
        atk -= 7;
    if (limb_arm_condition(a,w)<.3f && limb_arms(a)>0) atk-=15;
    atk-=limb_attack_penalty(a,w);
    if(limb_crawling(b))def*=.65f;
    int avoided=rf()>clamp(.71f+(atk-def)*.008f,.05f,.95f);
    float opposingDef=level(b,b->gear[0]?DEFENCE:MARTIAL),learn=ranged?1:combat_learning(atk,opposingDef);
    float defendLearn=ranged?1:combat_learning(def,level(a,w?MELEE:MARTIAL));
    float cut=weapon_cut_ratio(w),strengthXP=ranged?.15f:w?fminf(8,itemDefs[w].weight*.35f):1.5f;
    gain(a,sk,(avoided?4:8)*learn);
    gain(a,DEXTERITY,(ranged?.3f:3*cut)*learn*(avoided?.5f:1));
    gain(a,STRENGTH,strengthXP);
    weapon_class_gain(a,w,(avoided?3.2f:8)*combat_learning(weapon_class_level(a,weapon_class(w)),opposingDef));
    gain(b,defSkill,(evading?(avoided?4:8):(avoided?2:16))*defendLearn*(evading?dodge_mobility(b):1));
    gain(b,STRENGTH,strengthXP*(avoided&&!evading?.4f:avoided?0:.1f));
    if(avoided){
        if(!evading)weapon_class_gain(b,b->gear[0],.8f*combat_learning(weapon_class_level(b,weapon_class(b->gear[0])),opposingDef));
        if(distance(b->x,b->y,g.actors[g.selected].x,g.actors[g.selected].y)<12&&!evading)audio_event(3);
        b->impactKind=evading?3:2;
        b->hitFlash = .24f;
        b->impactDamage = 0;
        return;
    }
    int limb = rnd() % LIMBS;
    if (rf() < .35f || limb_missing(b,limb))
        limb = CHEST;
    float damage =
        (w ? itemDefs[w].power : 7 + level(a, MARTIAL) * .4f) + (w?(effective_level(a,STRENGTH)*(1-cut)+effective_level(a,DEXTERITY)*cut):effective_level(a,STRENGTH))*.16f + atk*.18f + (!w?level(a,TOUGHNESS)*.08f:0);
    damage*=1+(weapon_class_level(a,weapon_class(w))-1)*.0025f;
    damage *= .8f + rf() * .4f;
    float rawDamage=damage;
    damage -= fmaxf(0, depth_armor(b, limb, w, rnd()%100) - itemDefs[w].penetration) * .65f;
    if (itemDefs[w].effect == 3)
        b->stun = .3f + itemDefs[w].potency * .03f;
    damage = fmaxf(2, damage * (1 - level(b, TOUGHNESS) * .002f));
    if (limb_arm_condition(a,w)<.2f && limb_arms(a)>0) damage *= .65f;
    damage *= limb_damage_factor(a,w);
    if (a->hunger < 15)
        damage *= .7f;
    if (distance(b->x, b->y, g.actors[g.selected].x, g.actors[g.selected].y) < 12)
        audio_event(2);
    b->health[limb] -= damage;
    depth_wound(b, limb, damage);
    b->taskKind = 0; /* A hit interrupts resting and gathering even in passive stance. */
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
    if (!actor_inside(b) && can_walk(b->x + dx / d * knock, b->y + dy / d * knock)) {
        b->x += dx / d * knock;
        b->y += dy / d * knock;
    }
    if (a->squad || b->squad) {
        g.hitstop = fminf(.075f, .025f + damage * .001f);
        g.shake = fminf(.22f, damage * .006f);
    }

    gain(b, TOUGHNESS, rawDamage * .65f * combat_learning(level(b,TOUGHNESS),rawDamage));
    if (depth_organic(b, limb) && w && (cut >= .25f || ranged)) {
        b->woundFlags |= 1u << limb;
        b->bleed += damage * (ranged ? .028f : cut * .043f);
    }
    if (a->squad && hostile(a, b))
        g.townLife.squadHitAt[bi] = g.time + 60;
    b->target = ai;
    if (a->squad || b->squad)
        log_progress("%s hits %s: %s -%d", a->name, b->name, limbNames[limb], (int)damage);
    limb_hit(b,limb,cut);
    if (b->health[HEAD] <= 0 || b->health[CHEST] <= 0 || b->blood < 20) {
        injury_knockout(b, damage);
        credit_squad_defeat(b);
        if (a->squad || b->squad)
            log_warn("%s %s.", b->name, recovery_coma(b) ? "is in a recovery coma" : "is unconscious");
    }
}
float coma_threshold(const Actor *a) {
    return -species_health(a) * (.10f + .0055f * (level(a, TOUGHNESS) - 1));
}
int recovery_coma(const Actor *a) {
    return !a->dead && a->unconscious > 0 && (a->woundFlags & WOUND_COMA);
}
int fatal_injury(const Actor *a) {
    return (a->race != 2 && a->blood <= 0) || a->health[HEAD] <= -.85f * species_health(a);
}
void injury_knockout(Actor *a, float impact) {
    if (fminf(a->health[HEAD], a->health[CHEST]) <= coma_threshold(a))
        a->woundFlags |= WOUND_COMA;
    if (a->unconscious <= 0)
        a->unconscious = clamp(9 + impact * .12f - level(a, TOUGHNESS) * .075f, 5, 18);
    a->autoattack = a->attackPhase = 0;
    if (a->taskKind != TASK_JAIL_CARE && a->taskKind != 3)
        a->taskKind = 0;
    a->target = -1;
}
int wound_needs_dressing(const Actor *a) {
    if (a->dead || a->race == 2 || fatal_injury(a))
        return 0;
    if (a->bleed > .01f || (a->woundFlags & WOUND_CUTS))
        return 1;
    const DepthActor *z = &g.depth.actors[a - g.actors];
    for (int l = 0; l < LIMBS; l++)
        if (depth_can_heal(a, l) && !(a->treatedMask & (1 << l)) &&
            (a->health[l] < species_health(a) - z->trauma[l] - 2 || z->trauma[l] > .5f))
            return 1;
    return 0;
}
void injury_dress(Actor *a, const DepthActor *z) {
    for (int l = 0; l < LIMBS; l++)
        if (depth_can_heal(a, l) && (a->health[l] < species_health(a) || z->trauma[l] > 0))
            a->treatedMask |= 1 << l;
    a->woundFlags &= ~WOUND_CUTS;
    a->bleed = 0;
}
void dress_wounds(Actor *a) { injury_dress(a, &g.depth.actors[a - g.actors]); }
void injury_repair(Actor *a, const DepthActor *z) {
    for (int l = 0; l < LIMBS; l++)
        if (!depth_organic(a, l) && !limb_missing(a, l) &&
            (a->health[l] < limb_max_health(a, l) - 1 || z->trauma[l] > 0))
            a->woundFlags |= 1u << (WOUND_REPAIR_SHIFT + l);
}

/* Both live and stored travelers follow the same injury and wake-up rules.
   A severe torso wound bottoms out in coma territory. Only head trauma or
   exhausted blood can turn a combat injury into death. */
int injury_tick(Actor *a, DepthActor *z, float dt, int resting) {
    if (fatal_injury(a))
        return 1;
    float max = species_health(a);
    int uprightVitals = a->health[HEAD] > 0 && a->health[CHEST] > 0;
    a->health[CHEST] = fmaxf(a->health[CHEST], -.70f * max);
    if (a->race != 2) {
        if (a->bleed > .01f) {
            if (a->taskKind != TASK_JAIL_CARE)
                a->blood -= a->bleed * dt * .35f;
            float clotting = .028f + .00028f * level(a, TOUGHNESS) + (a->unconscious > 0 ? .016f : 0);
            a->bleed = fmaxf(0, a->bleed - dt * clotting);
            /* Earlier saves and authored casualties may have bleeding without a cut mask. */
            if (!(a->woundFlags & WOUND_CUTS))
                for (int l = 0; l < LIMBS; l++)
                    if (depth_can_heal(a, l) && a->health[l] < max - 2 && !(a->treatedMask & (1 << l)))
                        a->woundFlags |= 1u << l;
        } else {
            a->bleed = 0;
            float rate = resting || (a->woundFlags & WOUND_CLINIC) ? .8f : a->unconscious > 0 ? .5f : .25f;
            if (a->woundFlags & WOUND_TONIC)
                rate += .65f;
            a->blood = fminf(100, a->blood + dt * rate);
        }
    }
    if (fatal_injury(a))
        return 1;
    if (a->bleed <= .01f && (a->woundFlags & WOUND_CUTS)) {
        a->treatedMask |= a->woundFlags & WOUND_CUTS;
        a->woundFlags &= ~WOUND_CUTS;
    }

    if (fminf(a->health[HEAD], a->health[CHEST]) <= coma_threshold(a) || (a->race != 2 && a->blood < 20))
        injury_knockout(a, 0);
    if (a->woundFlags & (WOUND_TONIC | WOUND_CLINIC))
        a->exposure = fmaxf(0, a->exposure - dt * 2.0f);
    if (a->blood >= 100 && a->exposure <= 0)
        a->woundFlags &= ~WOUND_TONIC;
    float nutrition = a->hunger < 20 ? .65f : 1;
    int wounds = 0;
    for (int l = 0; l < LIMBS; l++) {
        unsigned repair = 1u << (WOUND_REPAIR_SHIFT + l);
        if (a->woundFlags & repair) {
            float full = limb_max_health(a, l);
            z->trauma[l] = fmaxf(0, z->trauma[l] - dt * (resting ? .8f : .45f));
            float rate = resting || (a->unconscious > 0 && l <= CHEST) ? 2.0f : 1.05f;
            a->health[l] = fminf(full - z->trauma[l], a->health[l] + dt * rate);
            if (a->health[l] >= full || limb_missing(a, l))
                a->woundFlags &= ~repair;
            continue;
        }
        if (!depth_can_heal(a, l)) {
            /* A Clockwork can restart its core. Full repair still needs parts. */
            if (a->race == 2 && a->unconscious > 0 && l <= CHEST && a->health[l] < 3)
                a->health[l] = fminf(3, a->health[l] + dt * 1.1f);
            continue;
        }
        if (a->woundFlags & (1u << l)) {
            float floor = l == HEAD && a->health[l] < -.45f * max ? -max : -.34f * max;
            if (a->health[l] > floor)
                a->health[l] = fmaxf(floor, a->health[l] - dt * .16f * (1 - level(a, TOUGHNESS) * .006f));
            continue;
        }
        if (a->health[l] >= max && z->trauma[l] <= 0)
            continue;
        int dressed = a->treatedMask & (1 << l);
        int vital = l == HEAD || l == CHEST;
        int cared = resting || (a->woundFlags & WOUND_CLINIC);
        float rate = cared ? 1.85f + gear_bonus(a, 6) * .01f :
            a->unconscious > 0 && vital ? 1.05f : dressed ? .44f : .38f;
        z->trauma[l] = fmaxf(0, z->trauma[l] - dt * (cared ? .8f : dressed ? .35f : .2f) * nutrition);
        float cap = max - z->trauma[l];
        a->health[l] = fminf(cap, a->health[l] + dt * rate * nutrition);
        if (a->health[l] >= max)
            a->treatedMask &= ~(1 << l);
        else
            wounds = 1;
    }
    if (!wounds && a->blood >= 100 && a->exposure <= 0)
        a->woundFlags &= ~WOUND_CLINIC;
    if (fatal_injury(a))
        return 1;
    if (uprightVitals && (a->health[HEAD] <= 0 || a->health[CHEST] <= 0))
        injury_knockout(a, 0);

    if (a->unconscious > 0) {
        float left = a->unconscious - dt;
        int bloodReady = a->race == 2 || a->blood >= 25;
        int stabilized = a->health[HEAD] > 0 && a->health[CHEST] > 0 && bloodReady;
        if (left <= 0 && (!bloodReady || (recovery_coma(a) && !stabilized)))
            left = .25f;
        a->unconscious = fmaxf(0, left);
        if (!a->unconscious)
            a->woundFlags &= ~WOUND_COMA;
    }
    if (a->unconscious <= 0 && a->bleed <= .01f && a->blood >= 35)
        a->woundFlags &= ~WOUND_AID_CHECKED;
    return 0;
}
static int treatment_ready(const Actor *a, int type) {
    if (type == TREAT_PRESSURE)
        return a->bleed > .01f && a->race != 2;
    if (type == TREAT_REPAIR)
        return depth_needs_repair(a);
    if (type == TREAT_CLINIC) {
        if (a->race == 2 || (a->woundFlags & WOUND_CLINIC))
            return 0;
        if (a->bleed > .01f || a->blood < 98 || a->exposure > 1)
            return 1;
        for (int l = 0; l < LIMBS; l++)
            if (depth_can_heal(a, l) && a->health[l] < species_health(a) - 1)
                return 1;
        return 0;
    }
    return wound_needs_dressing(a);
}
static int treatment_supplies(Actor *m, int type, int slot, int consume) {
    if (type == TREAT_CLINIC) {
        if (g.money < 80 || town_near(m->x, m->y, 14) < 0)
            return 0;
        if (consume) g.money -= 80;
        return 1;
    }
    if (type == TREAT_PRESSURE) {
        if (inventory_count(m, CLOTH) >= 2)
            return !consume || inventory_remove(m, CLOTH, 2);
        if (m->gear[1] != RAGS)
            return 0;
        if (consume) m->gear[1] = m->gearStolen[1] = 0;
        return 1;
    }
    int item = type == TREAT_REPAIR ? PARTS : MEDKIT;
    if (slot >= 0)
        return slot < PACK && m->pack[slot].item == item &&
            (!consume || inventory_remove_slot(m, slot, 1));
    return consume ? inventory_remove(m, item, 1) : inventory_count(m, item) > 0;
}
float treatment_burden(const Actor *a) {
    float damage = 0;
    for (int l = 0; l < LIMBS; l++) {
        if (!depth_can_heal(a, l) || ((a->treatedMask & (1 << l)) && !(a->woundFlags & (1u << l))))
            continue;
        damage += fmaxf(0, 1 - a->health[l] / limb_max_health(a, l));
        if (a->woundFlags & (1u << l))
            damage += .18f;
    }
    return fminf(8, 1.7f * damage + a->bleed * .52f);
}
int begin_treatment(int mi, int pi, int type, int slot) {
    if (mi < 0 || pi < 0 || mi >= g.actorCount || pi >= g.actorCount)
        return 0;
    Actor *m = &g.actors[mi], *a = &g.actors[pi];
    if (!conscious(m) || !a->active || a->dead || fatal_injury(a) || depth_distance(m, a) > TALK_REACH ||
        m->taskKind == TASK_TREAT || m->taskKind == TASK_JAIL_CARE || m->taskKind == TASK_CELL_LOCK)
        return 0;
    if (!treatment_ready(a, type) || !treatment_supplies(m, type, slot, 0)) {
        if (mi == g.selected)
            log_msg(type == TREAT_REPAIR ? "Repairs need damaged mechanical parts and a spare part." :
                    type == TREAT_CLINIC ? "Clinic care needs an injury and 80 silver." :
                    type == TREAT_PRESSURE ? "Pressure dressing needs bleeding and cloth or rags." :
                    "First aid needs a wound and a field dressing.");
        return 0;
    }
    m->taskKind = TASK_TREAT;
    m->taskIndex = (type * MAX_ACTORS + pi) * (PACK + 1) + slot + 1;
    float burden = treatment_burden(a);
    float skill = level(m, type == TREAT_REPAIR ? ENGINEERING : MEDICINE) * .035f;
    if (type == TREAT_PRESSURE)
        m->action = 2.1f + fminf(2.3f, burden * .32f);
    else if (type == TREAT_CLINIC)
        m->action = 3.5f + burden * .75f;
    else if (type == TREAT_REPAIR)
        m->action = fmaxf(1.75f, 5.5f - skill);
    else
        m->action = fmaxf(1.75f, 3.0f - skill - gear_bonus(m, 5) * .05f) + burden;
    if (mi == g.selected) {
        ui.screen = SCREEN_WORLD;
        ui.selection = ui.menuReturn = 0;
    }
    if (m->squad || a->squad)
        log_msg("%s starts %s. Stay within reach.", m->name, type == TREAT_REPAIR ? "repairs" :
                type == TREAT_CLINIC ? "clinic care" : "first aid");
    return 1;
}
static void treatment_tick(float dt) {
    for (int mi = 0; mi < g.actorCount; mi++) {
        Actor *m = &g.actors[mi];
        if (m->taskKind != TASK_TREAT)
            continue;
        int type = m->taskIndex / ((PACK + 1) * MAX_ACTORS);
        int pi = m->taskIndex / (PACK + 1) % MAX_ACTORS, slot = m->taskIndex % (PACK + 1) - 1;
        if (m->taskIndex < 0 || type > TREAT_CLINIC || pi >= g.actorCount || !conscious(m)) {
            m->taskKind = 0;
            continue;
        }
        Actor *a = &g.actors[pi];
        if (!a->active || a->dead || fatal_injury(a) || depth_distance(m, a) > TALK_REACH) {
            m->taskKind = 0;
            if (mi == g.selected) log_msg("Treatment interrupted.");
            continue;
        }
        m->action -= dt;
        if (m->action > 0)
            continue;
        m->taskKind = 0;
        if (!treatment_ready(a, type) || !treatment_supplies(m, type, slot, 1))
            continue;
        if (type == TREAT_REPAIR) {
            injury_repair(a, &g.depth.actors[pi]);
            gain(m, ENGINEERING, 25);
            log_msg("%s repaired %s. Parts are recovering.", m->name, a->name);
        } else {
            dress_wounds(a);
            if (type == TREAT_CLINIC)
                a->woundFlags |= WOUND_CLINIC;
            if (type == TREAT_DRESS)
                gain(m, MEDICINE, 24);
            if (type == TREAT_CLINIC)
                log_msg("Clinic care begins. Rest here while you recover.");
            else if (a->squad && !m->squad)
                log_warn("%s bandaged %s. The bleeding has stopped.", m->name, a->name);
            else
                log_msg("%s dressed %s's wounds. Recovery takes time.", m->name, a->name);

        }
        if (!m->squad && a->squad && mi != pi)
            roadside_aid_finished(mi, pi);
    }
}
int heal_actor(int mi, int pi) { return begin_treatment(mi, pi, TREAT_DRESS, -1); }
void use_item(int s) {
    Actor *a = &g.actors[g.selected];
    if (s < 0 || s >= PACK || !a->pack[s].item)
        return;
    int it = a->pack[s].item;
    if (itemDefs[it].slot >= 0) {
        int e = itemDefs[it].slot, old = a->gear[e], owner = a->stolen[s], oldOwner = a->gearStolen[e];
        Actor backup = *a;
        inventory_remove_slot(a, s, 1);
        if (old && !inventory_add_owned(a, old, 1, oldOwner)) {
            *a = backup;
            log_msg("Pack full. Cannot swap equipment.");
            return;
        }
        a->gear[e] = it;
        a->gearStolen[e] = owner;
        log_msg("Equipped %s.", itemDefs[it].name);
    } else if (it == RATION || it == WATER) {
        inventory_remove_slot(a, s, 1);
        a->hunger = fminf(100, a->hunger + (it == RATION ? 48 : 12));
        log_msg("Used %s.", itemDefs[it].name);
    } else if (it == TONIC) {
        if (conscious(a) && a->race != 2 && !(a->woundFlags & WOUND_TONIC) &&
            (a->blood < 100 || a->exposure > 0)) {
            inventory_remove_slot(a, s, 1);
            a->woundFlags |= WOUND_TONIC;
            log_msg("Tonic helps restore blood and warmth over time.");
        } else
            log_msg("A tonic cannot help right now.");
    } else if (it == MAPSCROLL) {
        inventory_remove_slot(a, s, 1);
        memset(g.seen, 255, sizeof(g.seen));
        for (int t = 0; t < TOWNS; t++)
            g.discovered[t] = 1;
        log_msg("Survey chart reveals the frontier.");
    } else if (it == MEDKIT)
        begin_treatment(g.selected, g.selected, TREAT_DRESS, s);
    else if (it == PARTS)
        begin_treatment(g.selected, g.selected, TREAT_REPAIR, s);
    else if (limb_item_grade(it)>=0) {
        ui.limbReturn=SCREEN_PACK;ui.limbTarget=limb_item_leg(it)?LLEG:LARM;
        if(!limb_missing(a,ui.limbTarget)&&limb_missing(a,ui.limbTarget+1))ui.limbTarget++;
        limb_open(SCREEN_LIMBS);ui.selection=ui.limbTarget-LARM;
        log_msg("Choose left or right limb, then choose the replacement.");
    } else
        log_msg("%s", itemDefs[it].desc);
}
int buy_item(int town, int it) {
    Actor *a = &g.actors[g.selected];
    if (town < 0 || town >= TOWNS || it <= 0 || it >= ITEMS)
        return 0;
    int price = item_price(town, it, 0);
    if (g.rep[g.townOwner[town]] < -25 && (disguise_score(a,g.townOwner[town])<40||g.society.recognized[g.selected][g.townOwner[town]]>0)) {
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
    if (g.rep[g.townOwner[town]] < -25 && (disguise_score(a,g.townOwner[town])<40||g.society.recognized[g.selected][g.townOwner[town]]>0)) {
        log_msg("This faction refuses to trade with you.");
        return 0;
    }
    if (a->stolen[s] && a->stolen[s] == g.townOwner[town]) {
        g.rep[a->stolen[s]] -= 8;
        crime_add(a,g.townOwner[town],40);
        log_warn("Recognized stolen goods. The trader alerts guards.");
        return 0;
    }
    inventory_remove_slot(a, s, 1);
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
int build_wood_cost(int c) {
    return c >= 0 && c < buildCount
               ? (int)fmaxf(1, buildWood[c] - level(&g.actors[g.selected], ENGINEERING) / 30)
               : 0;
}
int harvest_yield(const Actor *a) { return 5 + level(a, FARMING) / 30; }
int research_cost(void) {
    return 150 * (g.tech + 1) * (100 - level(&g.actors[g.selected], SCIENCE) / 5) / 100;
}
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
    if (inventory_count(a, WOOD) < build_wood_cost(c) || inventory_count(a, IRONBAR) < buildIron[c]) {
        log_msg("Need %d timber and %d iron bars.", build_wood_cost(c), buildIron[c]);
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
    inventory_remove(a, WOOD, build_wood_cost(c));
    if (buildIron[c])
        inventory_remove(a, IRONBAR, buildIron[c]);
    add_build(x, y, buildTypes[c], FREE, g.selected);
    if (!g.baseExists) {
        g.baseExists = 1;
        g.baseX = x;
        g.baseY = y;
        g.raidTimer = 420;
    }
    g.depth.buildings[g.buildingCount-1].labor = buildTypes[c]==CAMPFIRE?12:buildTypes[c]==BED?16:35;
    g.depth.buildings[g.buildingCount-1].fuel=0;
    g.depth.buildings[g.buildingCount-1].condition=100;
    a->taskKind=4;a->taskIndex=g.buildingCount-1;a->action=g.depth.buildings[a->taskIndex].labor+1;
    gain(a, ENGINEERING, 10);
    log_msg("%s placed. Stay nearby to finish construction.", buildNames[c]);
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
int recipe_requirements(int c,CraftRequirements*out){
    if(c<0||c>=recipeCount)return 0;
    const Recipe*r=&recipes[c];memset(out,0,sizeof(*out));
    out->output=r->output;out->qty=r->qty+(c==1?level(&g.actors[g.selected],COOKING)/35:0);
    out->item[0]=r->in1;out->need[0]=r->n1;out->count=1;
    if(r->in2){out->item[1]=r->in2;out->need[1]=r->n2;out->count++;}
    if(c==1){out->alternate=MEAT;out->alternateNeed=2;}
    out->station=r->station;out->tech=r->tech;return 1;
}
int nearby_station(int type, float r) {
    if (g.interiorBuilding && g.selected == g.interiorActor) {
        int id = g.interiorBuilding - 1;
        int c = interior_context();
        InteriorProp props[8];
        interior_props(id, props);
        int action = type == FORGE   ? ROOM_FORGE
                     : type == BENCH ? ROOM_RESEARCH
                     : type == BED   ? ROOM_REST
                     : type == STORE ? ROOM_STORAGE
                                     : ROOM_NONE;
        return c >= 0 && action && props[c].action == action ? id : -1;
    }
    Actor *a = &g.actors[g.selected];
    for (int i = 0; i < g.buildingCount; i++)
        if (g.buildings[i].type == type && depth_build_ready(i) && distance(a->x, a->y, g.buildings[i].x, g.buildings[i].y) < r)
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
    if (nearby_station(r->station, 1.7f) < 0) {
        log_msg("Stand near a %s.", r->station == FORGE      ? "forge"
                                    : r->station == CAMPFIRE ? "campfire"
                                                             : "research bench");
        return 0;
    }
    int in1 = r->in1, n1 = r->n1, qty = r->qty + (c == 1 ? level(a, COOKING) / 35 : 0);
    if (c == 1 && inventory_count(a, GRAIN) < n1) {
        in1 = MEAT;
        n1 = 2;
    }
    if (inventory_count(a, in1) < n1 || (r->in2 && inventory_count(a, r->in2) < r->n2)) {
        log_msg("Need %d %s + %d %s.", n1, itemDefs[in1].name, r->n2, itemDefs[r->in2].name);
        return 0;
    }
    Actor old = *a;
    int owner = inventory_input_owner(a, in1, n1);
    if (!owner && r->in2)
        owner = inventory_input_owner(a, r->in2, r->n2);
    inventory_remove(a, in1, n1);
    if (r->in2)
        inventory_remove(a, r->in2, r->n2);
    if (!inventory_add_owned(a, r->output, qty, owner)) {
        *a = old;
        log_msg("Pack full.");
        return 0;
    }
    if ((r->station==FORGE||r->station==CAMPFIRE)&&!depth_use_fuel(nearby_station(r->station,1.7f),1)){*a=old;return 0;}
    gain(a, r->skill, 35);
    log_msg("Crafted %d %s.", qty, itemDefs[r->output].name);
    return 1;
}
void recipe_status(int c, char *out, int size) {
    const Recipe *r = &recipes[c];
    Actor *a = &g.actors[g.selected];
    int in = r->in1, need = r->n1;
    if (c == 1 && inventory_count(a, GRAIN) < need) {
        in = MEAT;
        need = 2;
    }
    if (c == 1) {
        snprintf(out, size, "MAKES %d / GRAIN %d/3 OR MEAT %d/2 + WATER %d/1 / %s",
                 r->qty + level(a, COOKING) / 35, inventory_count(a, GRAIN), inventory_count(a, MEAT),
                 inventory_count(a, WATER),
                 nearby_station(CAMPFIRE, 1.7f) >= 0 ? "CAMPFIRE READY" : "NEED NEARBY CAMPFIRE");
        return;
    }
    char second[64] = "";
    if (r->in2)
        snprintf(second, sizeof(second), " + %s %d/%d", itemDefs[r->in2].name, inventory_count(a, r->in2),
                 r->n2);
    snprintf(out, size, "MAKES %d / %s %d/%d%s / TIER %d/%d / %s",
             r->qty + (c == 1 ? level(a, COOKING) / 35 : 0), itemDefs[in].name, inventory_count(a, in), need,
             second, g.tech, r->tech,
             nearby_station(r->station, 1.7f) >= 0 ? "STATION READY"
             : r->station == FORGE                 ? "NEED NEARBY FORGE"
             : r->station == CAMPFIRE              ? "NEED NEARBY CAMPFIRE"
                                                   : "NEED NEARBY BENCH");
}
int research_tech(void) {
    Actor *a = &g.actors[g.selected];
    if (g.tech >= 3) {
        log_msg("All three research tiers completed.");
        return 0;
    }
    if (nearby_station(BENCH, 1.7f) < 0) {
        log_msg("Stand near a research bench.");
        return 0;
    }
    int cost = research_cost();
    if (g.money < cost || inventory_count(a, RELIC) < 1) {
        log_msg("Need 1 relic and %d silver.", cost);
        return 0;
    }
    inventory_remove(a, RELIC, 1);
    g.money -= cost;
    g.tech++;
    gain(a, SCIENCE, 120);
    log_msg("Research tier %d completed.", g.tech);
    return 1;
}

typedef struct {
    int kind, index;
    float score;
} ContextCandidate;
static ContextCandidate candidates[64];
static void add_context(int kind, int index, float d, float reach) {
    if (d > reach)
        return;
    float score = d;
    int count = ui.contextCount, pos = count < 64 ? count : 63;
    if (count == 64 && score >= candidates[63].score)
        return;
    while (pos > 0 && score < candidates[pos - 1].score) {
        candidates[pos] = candidates[pos - 1];
        pos--;
    }
    candidates[pos] = (ContextCandidate){kind, index, score};
    if (count < 64)
        ui.contextCount++;
}
void find_context(void) {
    Actor *a = &g.actors[g.selected];
    ui.contextCount = 0;
    ui.contextKind = ui.contextIndex = -1;
    for (int i = 0; i < g.actorCount; i++) {
        Actor *b = &g.actors[i];
        if (i == g.selected || !b->active || b->carriedBy >= 0 || population_hidden(i) || actor_inside(a) != actor_inside(b))
            continue;
        if (conscious(b) && hostile(a, b) && (b->target >= 0 || a->target == i))
            continue;
        if (b->squad && !b->dead && b->unconscious <= 0 && !g.depth.actors[i].captive) {
            int hurt = b->bleed > .01f;
            for (int j = 0; j < LIMBS; j++)
                if (b->health[j] < species_health(b)-g.depth.actors[i].trauma[j]-2 && !(b->lostMask & (1 << j)))
                    hurt = 1;
            if (!hurt)
                continue;
        }
        if (b->kind == TRADER || b->kind == RECRUIT || b->dead || b->unconscious > 0 || b->squad ||
            b->kind == SETTLER || b->kind == GUARD || b->kind == CARAVAN || depth_named(i)>=0)
            add_context(0, i, distance(a->x, a->y, b->x, b->y), TALK_REACH);
    }
    for (int i = 0; i < g.nodeCount; i++)
        if (g.nodes[i].remaining)
            add_context(1, i, distance(a->x, a->y, g.nodes[i].x, g.nodes[i].y), WORK_REACH);
    for (int i = 0; i < g.buildingCount; i++) {
        Building *b = &g.buildings[i];
        if (building_has_interior(b->type))
            add_context(3, i, distance(a->x, a->y, b->x + 1.45f, b->y + .65f), 1.7f);
        if (b->type >= WELL && b->type != WALL && b->type != WINDMILL)
            add_context(2, i, distance(a->x, a->y, b->x, b->y), 1.7f);
    }
    if (ui.contextCount) {
        ui.contextChoice %= ui.contextCount;
        ui.contextKind = candidates[ui.contextChoice].kind;
        ui.contextIndex = candidates[ui.contextChoice].index;
    } else
        ui.contextChoice = 0;
}
void witnessed_theft(Actor *a, int owner, int range, int penalty) {
    if (owner <= FREE || owner >= FACTIONS)
        return;
    if (a->squad)
        town_theft(town_near(a->x, a->y, 17));
    int witness = -1;
    float nearest = a->sneaking ? range * .5f : range;
    for (int i = 0; i < g.actorCount; i++) {
        Actor *b = &g.actors[i];
        if (b->squad || b->faction != owner || !conscious(b))
            continue;
        float d = depth_distance(a, b);
        if (d < nearest) {
            nearest = d;
            witness = i;
        }
    }
    gain(a, THEFT, 20);
    if (witness < 0) {
        log_msg("Stolen from %s without witnesses.", factionNames[owner]);
        return;
    }
    Actor *b = &g.actors[witness];
    unsigned warning = 1u << owner;
    if (!(g.theftWarnings & warning) && !wanted_amount(a, owner) && !hostile(a, b) &&
        (owner == SCAVENGERS || (b->kind != GUARD && owner != SHOGUN))) {
        g.theftWarnings |= warning;
        log_warn("%s: Hands off! Those supplies belong to %s.", b->name, factionNames[owner]);
        return;
    }
    g.rep[owner] -= penalty;
    crime_add(a, owner, 80);
    if (population_owned(witness))
        population_anger(witness);
    log_warn("%s saw you steal from %s. Bounty +80.", b->name, factionNames[owner]);
}
void interact(void) {find_context();interact_context(ui.contextKind,ui.contextIndex);}
void interact_context(int kind,int target) {
    ui.contextKind=kind;ui.contextIndex=target;
    Actor *a = &g.actors[g.selected];
    if (!conscious(a)) {
        log_msg("Choose a conscious squad member with \005.");
        return;
    }
    int n = ui.contextIndex;
    if (n < 0) {
        log_msg("Nothing nearby. Move closer to a person or object.");
        return;
    }
    if (context_distance() > context_reach()) {
        log_msg("Move closer to %s before you %s.", context_name(), context_action());
        return;
    }
    if (ui.contextKind == 4) return;
    if ((ui.contextKind==2||ui.contextKind==3)&&!depth_build_ready(n)) {depth_build_action(n);return;}
    if (ui.contextKind == 3) {
        interior_enter(n);
        return;
    }
    if (ui.contextKind == 0) {
        Actor *b = &g.actors[n];
        if (b->squad) {
            ui.lootActor = n;
            ui.selection = 0;
            ui.screen = SCREEN_ASSIST;
            return;
        }
        if (b->unconscious > 0 || b->dead) {
            if (b->bountyReward && b->dead && !bounty_contract_active(n)) {
                g.money += b->bountyReward;
                log_msg("Bounty proof recovered: %d silver.", b->bountyReward);
                b->bountyReward = 0;
            }
            ui.lootActor = n;
            ui.selection = 0;
            ui.screen = SCREEN_LOOT;
        } else if (a->sneaking) {
            ui.lootActor = n;
            ui.selection = 0;
            ui.screen = SCREEN_THEFT;
        }
        else
            dialogue_open(n);
        return;
    }
    if (ui.contextKind == 1) {
        Node *b = &g.nodes[n];
        if (b->locked) {
            a->taskKind = 2;
            a->taskIndex = n;
            a->action = fmaxf(1.2f, 4.5f - level(a, LOCKPICK) * .035f);
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
        if (inventory_add_owned(a, b->item, b->qty, b->owner)) {
            b->remaining = 0;
            log_msg("Found %d %s.", b->qty, itemDefs[b->item].name);
            if (b->owner)
                witnessed_theft(a, b->owner, 6, 18);
        } else
            log_msg("Pack full.");
        return;
    }
    Building *b = &g.buildings[n];
    if (b->type == WELL) {
        if(regional_well(a,b)) return;
        if (inventory_add(a, WATER, 2)) {
            gain(a, LABOUR, 6);
            log_msg("Drew 2 water.");
        }
    } else if (b->type == BED || b->type == CAMPFIRE) {
        if (b->type == BED) {
            int town = town_near(b->x, b->y, 14);
            int price = b->owner < 0 && !trail_bed(n) && town >= 0 ? regional_bed_cost(town) : 0;
            rest_at(a, n, b->x, b->y, 0, price);
            return;
        }
        a->autoattack=0;
        a->taskKind = 3;
        a->taskIndex = n;
        a->action = 180;
        log_msg("Resting by the fire. A bed speeds up healing.");
    } else if (b->type == FARM) {
        if (b->progress >= 100) {
            if (inventory_add(a, GRAIN, harvest_yield(a))) {
                b->progress = 0;
                b->stock = 0;
                gain(a, FARMING, 40);
                log_msg("Harvested grain. Water to replant.");
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
        if (distance(a->x, a->y, b->x, b->y) > WORK_REACH || !b->remaining) {
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
        if (distance(a->x, a->y, b->x, b->y) > WORK_REACH) {
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
    a->woundFlags = 0;
    if (a->squad)
        log_warn("%s died from %s.", a->name, a->blood <= 0 ? "blood loss" : "head trauma");
}
void reveal_area(Actor *a) {
    int x = (int)a->x, y = (int)a->y;
    for (int yy = y - 6; yy <= y + 6; yy++)
        for (int xx = x - 6; xx <= x + 6; xx++)
            if (xx >= 0 && yy >= 0 && xx < WORLD && yy < WORLD) {
                int k = (yy / 4) * FOG_SIZE + xx / 4;
                g.seen[k >> 3] |= 1 << (k & 7);
            }
    for (int t = 0; t < TOWNS; t++)
        if (!g.discovered[t] && distance(a->x, a->y, towns[t].x, towns[t].y) < 10) {
            g.discovered[t] = 1;
            log_msg("Discovered %s.", towns[t].name);
            gain(a, PERCEPTION, 80);
        }
}
int automatic_work(const Actor *a) {
    return a != &g.actors[g.selected] ||
           (ui.playerIdle >= 5 && !a->autoattack && !a->attackPhase &&
            a->target < 0 && g.living.command[g.selected] < 0 &&
            !g.society.queueCount[g.selected] && !society_rescue_busy(g.selected));
}
void work_notice(const Actor *a, const char *action, const char *target) {
    if (a != &g.actors[g.selected])
        return;
    char next[80];
    snprintf(next, sizeof(next), "%s is %s%s%s.", a->name, action, target ? " " : "", target ? target : "");
    if (strcmp(ui.workNotice, next)) {
        snprintf(ui.workNotice, sizeof(ui.workNotice), "%s", next);
        log_msg("%s", next);
    }
}
static void interrupt_work(void) {
    ui.playerIdle = 0;
    ui.workNotice[0] = 0;
    if (ui.jobGather[g.selected]) {
        g.actors[g.selected].taskKind = 0;
        ui.jobGather[g.selected] = 0;
    }
}
static void squad_job(Actor *a, float dt) {
    int member = (int)(a - g.actors);
    if (!automatic_work(a) || a->taskKind || g.depth.actors[member].paused)
        return;
    if (depth_group_job(a, dt))
        return;
    int job = assigned_job(a);
    if (job == JOB_MINE) {
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
            Actor test = *a;
            if (!inventory_add(&test, g.nodes[n].type == NODE_ORE ? ORE : COPPER, 1))
                return;
            work_notice(a, "working the", g.nodes[n].type == NODE_ORE ? "iron deposit" : "copper deposit");
            if (d > WORK_REACH * .9f)
                route_toward(a, g.nodes[n].x, g.nodes[n].y, dt);
            else {
                a->taskKind = 1;
                a->taskIndex = n;
                a->action = 3;
                ui.jobGather[member] = 1;
            }
        }
    } else if (job == JOB_MEDIC && inventory_count(a, MEDKIT)) {
        int patient = -1;
        float best = -1;
        for (int j = 0; j < g.actorCount; j++) {
            Actor *b = &g.actors[j];
            if (!b->squad || !needs_aid(b) || !wound_needs_dressing(b))
                continue;
            float d = depth_distance(a, b);
            if (d > 18)
                continue;
            float urgency = (b->bleed > .01f ? 40 : 0) + (b->unconscious > 0 ? 20 : 0) + 18 - d;
            if (urgency > best) {
                best = urgency;
                patient = j;
            }
        }
        if (patient >= 0) {
            Actor *b = &g.actors[patient];
            work_notice(a, "trying to bandage", b->name);
            if (depth_distance(a, b) > TALK_REACH * .9f) {
                if (actor_inside(a))
                    depth_move_room(a, depth_x(b) - depth_x(a), depth_y(b) - depth_y(a), dt);
                else
                    route_toward(a, b->x, b->y, dt);
            } else if (heal_actor(member, patient))
                ui.jobGather[member] = 1;
        }
    } else if (job == JOB_FARM) {
        for (int j = 0; j < g.buildingCount; j++) {
            Building *b = &g.buildings[j];
            if (b->type != FARM || b->owner < 0 || distance(a->x, a->y, b->x, b->y) > 15)
                continue;
            if (b->progress < 100 && (b->stock || !inventory_count(a, WATER)))
                continue;
            work_notice(a, b->progress >= 100 ? "going to harvest the crop" : "going to water the crop", NULL);
            if (distance(a->x, a->y, b->x, b->y) > 2)
                route_toward(a, b->x, b->y, dt);
            else if (b->progress >= 100 && inventory_add(a, GRAIN, harvest_yield(a))) {
                b->progress = 0;
                b->stock = 0;
                gain(a, FARMING, 40);
            } else if (!b->stock && inventory_remove(a, WATER, 1))
                b->stock = 1;
            break;
        }
    }
}
static int take_ration(Actor *a) {
    if (inventory_remove(a, RATION, 1))
        return 1;
    for (int i = 0; i < g.actorCount; i++) {
        Actor *b = &g.actors[i];
        if (b == a || !b->active || !b->squad || b->dead || depth_distance(a, b) > 3)
            continue;
        if (inventory_remove(b, RATION, 1))
            return 1;
    }
    int room = actor_inside(a);
    for (int i = 0; i < g.buildingCount; i++) {
        Building *b = &g.buildings[i];
        if (b->type != STORE || b->owner < 0 || !depth_build_ready(i) ||
            (room ? room != i + 1 : distance(a->x, a->y, b->x, b->y) > 3))
            continue;
        for (int j = 0; j < 16; j++)
            if (b->storage[j].item == RATION && b->storage[j].qty > 0) {
                if (--b->storage[j].qty == 0)
                    b->storage[j].item = NONE;
                return 1;
            }
    }
    return 0;
}
int civilian_flees(const Actor *a) {
    return !a->squad && (a->kind == SETTLER || a->kind == RECRUIT || a->kind == TRADER);
}
static int town_militia(const Actor *a, const Actor *threat, int town) {
    if (!civilian_flees(a) || town < 0 || a->homeTown != town ||
        a->faction != g.townOwner[town] || !a->gear[0] || limb_crawling(a) ||
        (a->kind != RECRUIT && a->kind != TRADER && npc_role(a) != ROLE_CITIZEN) ||
        a->health[CHEST] < species_health(a) * .48f || a->blood < 45 || threat->squad ||
        (threat->kind != BEAST && threat->kind != RAIDER) ||
        distance(threat->x, threat->y, towns[town].x, towns[town].y) > 18)
        return 0;
    int defenders = 0, threats = 0;
    for (int i = 0; i < g.actorCount; i++) {
        const Actor *b = &g.actors[i];
        if (b == a || fabsf(b->x - a->x) > 13 || fabsf(b->y - a->y) > 13 ||
            !conscious(b) || actor_inside(b) || population_hidden(i))
            continue;
        if (!b->squad && b->homeTown == town && b->faction == a->faction &&
            b->gear[0] && (b->kind == GUARD || b->kind == RECRUIT || b->kind == TRADER))
            defenders++;
        if (!b->squad && (b->kind == BEAST || b->kind == RAIDER) && hostile(a, b))
            threats++;
    }
    return defenders >= 1 && defenders + 1 >= threats;
}
static int civilian_retreat(int n, int threat, float dt) {
    Actor *a = &g.actors[n];
    if (!civilian_flees(a))
        return 0;
    Retreat *r = &retreats[n];
    if (threat >= 0 && (r->time <= 0 || r->stuck > .65f || distance(a->x, a->y, r->x, r->y) < 1)) {
        Actor *b = &g.actors[threat];
        float angle = atan2f(a->y - b->y, a->x - b->x);
        const float turns[] = {0, .55f, -.55f, 1.1f, -1.1f, 1.55f, -1.55f};
        r->x = a->x;
        r->y = a->y;
        r->stuck = 0;
        r->time = 14;
        for (unsigned i = 0; i < sizeof(turns) / sizeof(*turns); i++) {
            float dx = cosf(angle + turns[i]), dy = sinf(angle + turns[i]);
            if (!can_walk(a->x + dx, a->y + dy))
                continue;
            for (int d = 12; d >= 4; d -= 2) {
                float x = a->x + dx * d, y = a->y + dy * d;
                if (!can_walk(x, y))
                    continue;
                r->x = x;
                r->y = y;
                return 1;
            }
        }
    }
    if (r->time <= 0)
        return 0;
    r->time = threat >= 0 ? 14 : r->time - dt;
    a->target = -1;
    if (distance(a->x, a->y, r->x, r->y) > 1) {
        float ox = a->x, oy = a->y;
        route_toward(a, r->x, r->y, dt);
        r->stuck = distance(ox, oy, a->x, a->y) < .005f ? r->stuck + dt : 0;
    }
    return 1;
}
void simulate(float dt) {
    memset(g.depth.groupTravel,0,sizeof(g.depth.groupTravel));
    frontier_tick(dt);
    for(int k=0;k<6;k++)if(g.trails.bed[k]){Building*b=&g.buildings[g.trails.bed[k]-1];if(distance(g.actors[g.selected].x,g.actors[g.selected].y,b->x,b->y)<13)g.trails.seen|=1<<k;}

    g.shake = fmaxf(0, g.shake - dt);
    if (g.hitstop > 0) {
        g.hitstop = fmaxf(0, g.hitstop - dt);
        return;
    }
    depth_tick(dt);
    tick_expansion(dt);
    population_tick(dt);
    living_tick(dt);
    society_tick(dt);
    civil_tick(dt);
    politics_tick(dt);
    g.time += dt;
    g.dayTime += dt * (24.f / DAY_SECONDS);
    if (g.dayTime >= 24) {
        g.dayTime -= 24;
        g.day++;
        restock();
    }
    treatment_tick(dt);
    roadside_aid_tick(dt);
    for (int i = 0; i < g.buildingCount; i++) {
        Building *b = &g.buildings[i];
        if (!depth_build_ready(i)) continue;
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
        int ready = conscious(a) && (a->attackPhase || (a->autoattack && a->target >= 0 &&
            a->target < g.actorCount && conscious(&g.actors[a->target]) &&
            depth_distance(a,&g.actors[a->target]) < 10));
        CombatVisual *visual = &combatVisuals[i];
        visual->stance = clamp(visual->stance + (ready ? dt * 5 : -dt * 3), 0, 1);
        a->cooldown = fmaxf(0, a->cooldown - dt);
        a->stun = fmaxf(0, a->stun - dt);
        a->hitFlash = fmaxf(0, a->hitFlash - dt);
        a->movedTimer = fmaxf(0, a->movedTimer - dt);
        if (a->attackPhase) {
            if (!conscious(a))
                a->attackPhase = 0;
            else {
                float before = a->attackClock;
                a->attackClock -= dt;
                if (a->attackPhase == 2 && before > a->attackDuration * (1 - STRIKE_CONTACT) &&
                    a->attackClock <= a->attackDuration * (1 - STRIKE_CONTACT))
                    resolve_strike(i, a->attackTarget);
                if (a->attackClock <= 0 && a->attackPhase) {
                    float overrun = a->attackClock;
                    if (a->attackPhase == 1) {
                        a->attackPhase = 2;
                        /* Older saves may resume mid-attack with no side-table timing. */
                        if (visual->timing.stroke == 0)
                            visual->timing = combat_timing(a, a->attackWeapon, visual->variant);
                        a->attackDuration = visual->timing.stroke;
                        a->attackClock = fmaxf(a->attackDuration * .78f, a->attackDuration + overrun);
                    } else if (a->attackPhase == 2) {
                        a->attackPhase = 3;
                        a->attackDuration = visual->timing.recovery > 0
                            ? visual->timing.recovery : fmaxf(.18f, a->cooldown);
                        a->attackClock = fmaxf(0, a->attackDuration + overrun);
                    } else {
                        a->attackPhase = 0;
                        a->cooldown = 0;
                    }
                }
            }
        }
        if (!a->squad && a->prisonTime <= 0 &&
            distance(a->x, a->y, g.actors[g.selected].x, g.actors[g.selected].y) > 45 && !population_owned(i))
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
        if (a->taskKind == 3 && a->taskIndex <= -1000 && !population_sleep_valid(a))
            a->taskKind = 0;
        if ((a->woundFlags & WOUND_CLINIC) && town_near(a->x, a->y, 14) < 0)
            a->woundFlags &= ~WOUND_CLINIC;
        int bed_rest = actor_in_bed(a);
        int was_up = a->unconscious <= 0;
        if (injury_tick(a, &g.depth.actors[i], dt, bed_rest || a->prisonTime > 0)) {
            credit_squad_defeat(a);
            die(a);
            continue;
        }
        if (a->unconscious > 0) {
            if (was_up)
                credit_squad_defeat(a);
            continue;
        }
        if(a->squad&&a->race==2)reveal_area(a);
        if (a->squad && a->race!=2) {
            a->hunger = fmaxf(0, a->hunger - dt * .045f * species_food(a));
            a->fatigue = fminf(100, a->fatigue + dt * .008f);
            if (a->hunger < 40 && take_ration(a)) {
                a->hunger = fminf(100, a->hunger + 48);
                log_msg("%s ate a trail ration.", a->name);
            }
            if (a->hunger <= 0) {
                a->health[CHEST] -= dt * .12f;
                if (a->health[CHEST] <= 0)
                    injury_knockout(a, 0);
            }
            reveal_area(a);
        }
        if ((a->taskKind == 1 || a->taskKind == 2) &&
            (a->taskIndex < 0 || a->taskIndex >= g.nodeCount ||
             distance(a->x, a->y, g.nodes[a->taskIndex].x, g.nodes[a->taskIndex].y) > WORK_REACH))
            a->taskKind = 0;
        if (a->taskKind > 0 && a->taskKind < TASK_CELL_LOCK) {
            if (a->taskKind == 3)
                a->fatigue = fmaxf(0, a->fatigue - dt * 1.8f);
            a->action -= dt;
            if (a->action <= 0 && a->taskKind != 4)
                complete_task(a);
        }
        if (!a->taskKind)
            ui.jobGather[i] = 0;
        if (!conscious(a) || a->prisonTime>0 || society_rescue_busy(i) || g.depth.actors[i].captive || (a->carrying>=0 && g.depth.actors[a->carrying].captive>=2))
            continue;
        if (actor_inside(a)) {
            int job = assigned_job(a);
            if (a->squad && a->target < 0 && (job == JOB_FOLLOW || job == JOB_MEDIC))
                squad_job(a, dt);
            continue;
        }
        if (a->taskKind == TASK_TREAT || field_aid(i, dt))
            continue;
        if (retreats[i].time <= 0 && population_tactics(i, dt))
            continue;
        int target = -1, aggressor = -1;
        float aggressor_distance = 16;
        int town = !a->squad && (a->kind == GUARD || civilian_flees(a)) && a->homeTown >= 0 &&
                   distance(a->x, a->y, towns[a->homeTown].x, towns[a->homeTown].y) < 16
                   ? a->homeTown : -1;
        float best = weapon_ranged(a->gear[0]) ? itemDefs[a->gear[0]].range + 1 : 6.0f;
        if (a->kind == BEAST)
            best = 4.5f;
        float normal_reach = best;
        if (town >= 0 && a->kind == GUARD && a->faction == g.townOwner[town])
            best = fmaxf(best, 14);
        if (g.weather == 1 || g.weather == 3)
            best *= .75f;
        for (int j = 0; j < g.actorCount; j++) {
            Actor *b = &g.actors[j];
            if (i == j || !attackable(b))
                continue;
            if (a->squad && a->autoattack && !b->squad && !population_hidden(j) &&
                (b->target == i || (b->attackPhase && b->attackTarget == i)) &&
                fabsf(a->x - b->x) < aggressor_distance && fabsf(a->y - b->y) < aggressor_distance &&
                !jail_authority(b, a) && hostile(a, b)) {
                float d = distance(a->x, a->y, b->x, b->y);
                if (d < aggressor_distance) {
                    aggressor = j;
                    aggressor_distance = d;
                }
            }
            if (fabsf(a->x - b->x) >= best || fabsf(a->y - b->y) >= best)
                continue;
            float d = distance(a->x, a->y, b->x, b->y);
            if (d >= best || (d >= normal_reach &&
                (town < 0 || b->squad || (b->kind != BEAST && b->kind != RAIDER) ||
                 distance(b->x, b->y, towns[town].x, towns[town].y) > 18)) || !hostile(a, b))
                continue;
            float detect =
                b->sneaking ? best * fmaxf(.12f, .5f - level(b, STEALTH) * .003f - gear_bonus(b, 2) * .003f)
                            : best;
            if (d < best && d < detect) {
                target = j;
                best = d;
            }
        }
        if (aggressor >= 0) {
            target = aggressor;
            best = aggressor_distance;
        }
        int ordered=g.living.command[i];
        if(a->squad && a->autoattack && ordered>=0 && ordered<g.actorCount) {
            Actor *b=&g.actors[ordered];
            if(attackable(b) && depth_room(a)==depth_room(b) && depth_distance(a,b)<24) {target=ordered;best=depth_distance(a,b);}
            else g.living.command[i]=-1;
        }
        if (target >= 0 && (a->kind == RAIDER || a->kind == BEAST || a->kind == GUARD || a->kind == CARAVAN ||
                            (a->squad && a->autoattack) || town_militia(a, &g.actors[target], town))) {
            retreats[i].time = 0;
            a->target = target;
            a->taskKind = 0;
            Actor *b = &g.actors[target];
            if(depth_tactics(a,target,dt))continue;
            float range = a->gear[0] && limb_arms(a) ? itemDefs[a->gear[0]].range : 1.55f;
            if(i!=g.selected&&!weapon_ranged(a->gear[0])&&!combat_slot_available(i,target)&&!a->attackPhase){
                float angle=i*2.39996f;route_toward(a,b->x+cosf(angle)*(range+1.2f),b->y+sinf(angle)*(range+1.2f),dt*.5f);continue;
            }
            if (best > range) {
                if (i != g.selected)
                    route_toward(a, b->x, b->y, dt);
            } else
                strike(i, target);
            continue;
        }
        if (civilian_retreat(i, target, dt))
            continue;
        a->target = -1;
        int hired = merc_slot(a);
        if (hired >= 0) {
            Actor *leader = &g.actors[g.selected];
            float x = g.mercMode[hired] ? g.baseX : leader->x;
            float y = g.mercMode[hired] ? g.baseY : leader->y;
            if (distance(a->x, a->y, x, y) > 2.5f)
                route_toward(a, x, y, dt);
        } else if (society_npc_job(i,dt)) {
        } else if (civil_npc_job(i,dt)) {
        } else if (living_walk(i,dt)) {
        } else if (a->squad) {
            squad_job(a, dt);
        } else if (npc_role(a) == ROLE_MERCENARY || a->kind == GUARD || (npc_role(a)==ROLE_MEDIC && a->homeTown>=0 && g.life.towns[a->homeTown].stage==2)) {
            if (distance(a->x, a->y, a->homeX, a->homeY) > 1)
                route_toward(a, a->homeX, a->homeY, dt * .6f);
        } else if (a->kind == RAIDER || a->kind == BEAST ||
                   (a->kind == SETTLER && npc_role(a) == ROLE_CITIZEN) || a->kind == CARAVAN) {
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
                if (i < g.actorCount && g.actors[i].dead && !g.actors[i].squad && depth_named(i) < 0 && !regional_named(i) && civil_elite_index(i)<0 && !living_owned(i) && !politics_protected(i) &&
                    g.actors[i].carriedBy < 0 && merc_slot(&g.actors[i]) < 0)
                    n = i;
                else if (g.actorCount < MAX_ACTORS) {
                    n = add_actor("Camp raider", RAIDER, HOSTILE, clamp(g.baseX + 10, 3, WORLD - 4), g.baseY,
                                  9);
                    i = g.actorCount;
                }
                if (n >= 0) {
                    Actor *a = &g.actors[n];
                    if (a->dead)
                        init_actor(n, "Camp raider", RAIDER, HOSTILE, clamp(g.baseX+10,3,WORLD-4), g.baseY,9);
                    a->tx = a->homeX = g.baseX;
                    a->ty = a->homeY = g.baseY;
                    spawned++;
                }
            }
            if (spawned)
                log_msg("Raiders have found your camp.");
        }
    }
    depth_after_tick(dt);
    contract_tick();
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
enum { SAVE_VERSION = 21 };
int save_game(const char *path) {
    char tmp[512], bak[512];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    snprintf(bak, sizeof(bak), "%s.bak", path);
    FILE *f = fopen(tmp, "wb");
    if (!f)
        return 0;
    SaveHeader h = {{'D', 'U', 'S', 'T', 'S', 'A', 'V', 'E'}, SAVE_VERSION, sizeof(g), checksum(&g, sizeof(g))};
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
    if (t->townLife.layout < 0 || t->townLife.layout > 1)
        return 0;
    for (int i = 0; i < t->buildingCount; i++)
        if (t->townLife.chestRefresh[i] < 0)
            return 0;
    for (int i = 0; i < TOWNS; i++)
        if (t->townLife.theftCount[i] > 3 || t->townLife.boardCycle[i] < 0 ||
            !isfinite(t->townLife.theftUntil[i]) || !isfinite(t->townLife.searchAfter[i]) ||
            !isfinite(t->townLife.boardAfter[i]))
            return 0;
    for (int i = 0; i < CONTRACTS; i++)
        if (!isfinite(t->townLife.huntX[i]) || !isfinite(t->townLife.huntY[i]))
            return 0;
    for (int i = 0; i < t->actorCount; i++)
        if (!isfinite(t->townLife.squadHitAt[i]))
            return 0;
    if (t->roadsideAid.actor < 0 || t->roadsideAid.actor > t->actorCount ||
        t->roadsideAid.patient < 0 || t->roadsideAid.patient > t->actorCount ||
        (t->roadsideAid.patient && (!t->roadsideAid.actor || t->roadsideAid.patient == t->roadsideAid.actor)) ||
        t->roadsideAid.bed > t->buildingCount || t->roadsideAid.bed < -1000 - POP_GROUPS * 4 ||
        t->roadsideAid.generated < 0 || t->roadsideAid.generated > 1 ||
        t->roadsideAid.day < 0 || t->roadsideAid.day > t->day || !isfinite(t->roadsideAid.until))
        return 0;
    for (int j = 0; j < 4; j++) {
        if (t->mercActor[j] < 0 || t->mercActor[j] > t->actorCount || !isfinite(t->mercUntil[j]) ||
            t->mercUntil[j] < 0 || t->mercMode[j] < 0 || t->mercMode[j] > 1)
            return 0;
        if (t->mercActor[j] &&
            (t->actors[t->mercActor[j] - 1].voice != ROLE_MERCENARY || t->actors[t->mercActor[j] - 1].squad))
            return 0;
        for (int k = 0; k < j; k++)
            if (t->mercActor[j] && t->mercActor[j] == t->mercActor[k])
                return 0;
    }

    if (t->interiorBuilding < 0 || t->interiorBuilding > t->buildingCount)
        return 0;
    if (t->interiorBuilding && (t->interiorActor != t->selected ||
                                !building_has_interior(t->buildings[t->interiorBuilding - 1].type) ||
                                !isfinite(t->interiorX) || !isfinite(t->interiorY) || t->interiorX < .4f ||
                                t->interiorY < .4f || t->interiorX > 6.6f || t->interiorY > 6.6f))
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
                          c->item < 0 || c->item >= ITEMS || c->type < 0 || c->type > 7))
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
            a->race<0 || a->race>=SPECIES || a->hairColor < 0 || a->hairColor > 5 || a->routeLen < 0 || a->routeLen > 24 || a->routePos < 0 ||
            a->routePos > 24 || a->attackPhase < 0 || a->attackPhase > 3 || a->attackWeapon < 0 ||
            a->attackWeapon >= ITEMS || a->attackTarget >= t->actorCount || a->prisonTown >= TOWNS ||
            a->homeTown >= TOWNS || !isfinite(a->blood) || !isfinite(a->bleed) || !isfinite(a->hunger) ||
            !isfinite(a->fatigue) || !isfinite(a->unconscious) || a->prisonTown < -1 ||
            (a->woundFlags & ~WOUND_FLAGS) || a->taskKind < 0 || a->taskKind > TASK_JAIL_CARE ||
            (a->taskKind >= TASK_CELL_LOCK && (!isfinite(a->action) || a->action < 0)))
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
    if(t->trails.version<0||t->trails.version>1||t->trails.seen<0||t->trails.seen>63||t->trails.omit<0||t->trails.omit>0xffffff)return 0;
    for(int k=0;k<6;k++){int b=t->trails.bed[k];if(b<0||b>t->buildingCount||(b&&t->buildings[b-1].type!=BED)||(!t->trails.version&&b))return 0;}
    if(!t->trails.version&&(t->trails.seen||t->trails.omit))return 0;
    return field_notes_valid(t) && depth_validate(t) && regional_validate(t) && living_validate(t) && society_validate(t) && civil_validate(t) && limbs_validate(t) && population_validate(t) && life_validate(t) && war_validate(t);
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
    /* Recreate v15 landmarks after merging the legacy outpost, so their
       building references and collision exclusions use the migrated world. */
    for(int n=0;n<expanded->actorCount;n++)if(!expanded->actors[n].squad&&npc_role(&expanded->actors[n])==ROLE_MEDIC){inventory_remove(&expanded->actors[n],MEDKIT,3);inventory_remove(&expanded->actors[n],PARTS,2);}
    world_layout(0);
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
                int z = y * FOG_SIZE + x;
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
        a->woundFlags = 0;
        a->homeTown = town_near(a->x, a->y, 12);
        a->skin = i % 8;
        a->hair = i % 7;
        a->speechCooldown = 15;
        a->gear[4] = gear_id(69, 0, 0);
        if (a->squad)
            a->gear[5] = gear_id(76, 0, 0);
    }
    for (int i = 0; i < old->buildingCount; i++) {
        memset(&g.buildings[i], 0, sizeof(Building));
        memcpy(&g.buildings[i], &old->buildings[i], sizeof(LegacyBuilding));
    }
    memcpy(g.nodes, old->nodes, sizeof(LegacyNode) * old->nodeCount);
    spread_frontier(&g);
    for (int i = 46; i < expanded->actorCount && g.actorCount < MAX_ACTORS; i++)
        if (expanded->actors[i].homeTown < OLD_TOWNS)
            g.actors[g.actorCount++] = expanded->actors[i];
    for (int i = 35; i < expanded->buildingCount && g.buildingCount < MAX_BUILDINGS; i++){
        int shelter=0;for(int k=0;k<6;k++)if(expanded->trails.bed[k]==i+1)shelter=1;
        if(!shelter && town_near(expanded->buildings[i].x,expanded->buildings[i].y,24)<OLD_TOWNS)
            g.buildings[g.buildingCount++] = expanded->buildings[i];
    }
    for (int i = 0; i < expanded->nodeCount && g.nodeCount < MAX_NODES; i++){
        int cache=0;for(int k=0;k<6;k++)if(expanded->trails.bed[k]){Building*b=&expanded->buildings[expanded->trails.bed[k]-1];if(distance(b->x,b->y,expanded->nodes[i].x,expanded->nodes[i].y)<2)cache=1;}
        if (!cache&&(expanded->nodes[i].x >= 128 || expanded->nodes[i].y >= 128))
            g.nodes[g.nodeCount++] = expanded->nodes[i];
    }
    free(expanded);
    memset(&g.regions,0,sizeof(g.regions));
    memset(&g.living,0,sizeof(g.living));
    memset(&g.society,0,sizeof(g.society));
    memset(&g.civil,0,sizeof(g.civil));
    memset(&g.limbs,0,sizeof(g.limbs));
    memset(&g.population,0,sizeof(g.population));
    memset(&g.life,0,sizeof(g.life));
    memset(&g.trails,0,sizeof(g.trails));
    memset(&g.war,0,sizeof(g.war));
    return validate_game(&g);
}
static int save_shape(uint32_t version,uint32_t size) {
    return ((version==17 || version==18) && size==offsetof(Game,theftWarnings)) ||
      (version==19 && size==offsetof(Game,roadsideAid)) ||
      (version==20 && size==offsetof(Game,townLife)) ||
      (version==SAVE_VERSION && size==sizeof(Game)) || ((version==15 || version==16) && size==2148384) || (version==14 && size==2148348) || (version==13 && size==2143228) || (version==12 && size==2127704) || (version==11 && size==627508) || (version==10 && size==626860) || (version==9 && size==618188) || (version==8 && size==573688) || (version==7 && size==435592) ||
      (version==6 && size==435556) || (version==5 && size==410656) ||
      ((version==3||version==4)&&size==404460) || (version==2&&size==404444);
}
typedef struct {
    const unsigned char *source;
    unsigned char *dest;
    size_t left;
} SaveReader;
static void read_save_span(SaveReader *reader, void *end) {
    size_t count = (unsigned char *)end - reader->dest;
    if (count > reader->left) count = reader->left;
    memcpy(reader->dest, reader->source, count);
    reader->source += count;
    reader->left -= count;
    reader->dest = end;
}
static void read_save_array(SaveReader *reader, void *array, size_t oldBytes, size_t bytes) {
    read_save_span(reader, array);
    read_save_span(reader, (unsigned char *)array + oldBytes);
    reader->dest = (unsigned char *)array + bytes;
}
static int decode_modern(Game *out,const void *data,uint32_t version,uint32_t size) {
    memset(out,0,sizeof(*out));
    if (version >= 17) memcpy(out, data, size);
    else {
        int oldItems = version>=11 ? ITEMS : version==10 ? V100_ITEMS : version==9 ? V090_ITEMS :
                       version==8 ? OLD_ITEMS : version>=5 ? CATALOG_ITEMS : 2052;
        SaveReader reader = {data, (unsigned char *)out, size};
#define WIDEN(field, count) read_save_array(&reader, out->field, (count) * sizeof(out->field[0]), sizeof(out->field))
        WIDEN(discovered, OLD_TOWNS);
        for (int t = 0; t < OLD_TOWNS; ++t)
            read_save_array(&reader, out->stock[t], oldItems * sizeof(int), sizeof(out->stock[t]));
        reader.dest = (unsigned char *)&out->lastRestock;
        WIDEN(buildings, 320);
        WIDEN(townOwner, OLD_TOWNS);
        WIDEN(prosperity, OLD_TOWNS);
        WIDEN(townState, OLD_TOWNS);
        WIDEN(depth.buildings, 320);
        WIDEN(society.crisis, OLD_TOWNS);
        WIDEN(population.groups, OLD_POP_GROUPS);
        WIDEN(population.state, OLD_POP_GROUPS);
        WIDEN(population.social, OLD_POP_GROUPS);
        WIDEN(life.groups, OLD_POP_GROUPS);
        WIDEN(life.towns, OLD_TOWNS);
        WIDEN(fieldNotes.group, OLD_POP_GROUPS);
#undef WIDEN
        read_save_span(&reader, &out->war);
        if (reader.left) return 0;
    }
    if (version < 20) {
        for (int i = 0; i < MAX_ACTORS; i++)
            out->actors[i].woundFlags = 0;
        for (int k = 0; k < POP_GROUPS; k++)
            for (int i = 0; i < POP_MEMBERS; i++)
                out->population.groups[k].reserve[i].woundFlags = 0;
        for (int k = 0; k < EVENTS; k++)
            for (int i = 0; i < POP_MEMBERS; i++)
                out->living.events[k].reserve[i].woundFlags = 0;
    }
    if (out->theftWarnings >= (1u << FACTIONS) || !validate_game(out))
        return 0;
    if (version < 16)
        weapon_xp_migrate(out);
    return 1;
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
    if (save_shape(h.version, h.size)) {
        Game *t = calloc(1, sizeof(Game));
        void *raw = malloc(h.size);
        if (t && raw) {
            ok = fread(raw, h.size, 1, f) == 1 && h.hash == checksum(raw, h.size) &&
                 decode_modern(t, raw, h.version, h.size);
            if (ok) {
                if (h.version < 4)
                    spread_frontier(t);
                else
                    world_layout(1);
                g = *t;
            }
        }
        free(raw);
        free(t);
    } else if (h.version == 1 && h.size == sizeof(LegacyGame)) {
        LegacyGame *t = malloc(sizeof(LegacyGame));
        if (t) {
            ok = fread(t, h.size, 1, f) == 1 && h.hash == checksum(t, h.size);
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
        combat_visual_reset();
        memset(g.depth.groupTravel,0,sizeof(g.depth.groupTravel));
        frontier_identity();
        depth_init();
    regional_init();
    living_init();
    society_init();
    civil_init();
    limbs_init();
    population_init();
    life_init();
    trail_init();
    war_init();
        town_life_init();
        memset(&ui, 0, sizeof(ui));
        ui.attackFocus=-1;
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
    info->valid = save_shape(h.version, h.size) || (h.version == 1 && h.size == sizeof(LegacyGame));
    if (info->valid) {
        unsigned char *buf = calloc(1, sizeof(Game) > h.size ? sizeof(Game) : h.size);
        if (buf && fread(buf, h.size, 1, f) == 1 && checksum(buf, h.size) == h.hash) {
            if (h.version >= 2) {
                Game *v = malloc(sizeof(Game));
                if (v && decode_modern(v, buf, h.version, h.size)) {
                    info->day = v->day;
                    info->money = v->money;
                    info->squad = v->squadCount;
                    snprintf(info->name, 16, "%.15s", v->actors[v->selected].name);
                } else
                    info->valid = 0;
                free(v);
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
    if(!ok)log_warn("Save failed in slot %d.",n);else if(n)log_msg("Saved to slot %d.",n);
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
    ui.screen = s == SCREEN_ORDERS ? SCREEN_SQUAD : s == SCREEN_CARE ? SCREEN_HEALTH : s;
    ui.selection = ui.scroll = 0;
}
void select_squad_member(int n) {
    if (n == g.selected || n < 0 || n >= g.actorCount || !g.actors[n].squad)
        return;
    interrupt_work();
    if (g.interiorBuilding) {
        DepthActor *next = &g.depth.actors[n];
        if (next->room == g.interiorBuilding) {
            DepthActor *prev = &g.depth.actors[g.interiorActor];
            prev->room = g.interiorBuilding;
            prev->rx = g.interiorX;
            prev->ry = g.interiorY;
            g.interiorX = next->rx;
            g.interiorY = next->ry;
            g.interiorActor = n;
            next->room = 0;
        } else
            interior_leave();
    }
    g.selected = n;
    interrupt_work();
    ui.waypoint = ui.contextChoice = 0;
    find_context();
}
void cycle_squad(int direction) {
    int n=g.selected;
    for(int i=0;i<g.actorCount;i++) {
        n=(n+direction+g.actorCount)%g.actorCount;
        if(g.actors[n].squad && (!g.actors[n].dead || ui.screen!=SCREEN_WORLD)) {
            select_squad_member(n);
            return;
        }
    }
}
static const char *pauseLabels[3][9] = {
    {"Resume", "Save game", "Load game", "Game speed", "Controls & field guide", "Save and quit to title", "New wanderer"},
    {"Inventory & equipment", "Skills", "Health & recovery", "Squad management", "Armor layers", "Recent messages", "Choose opponent"},
    {"Map & bearings", "Journal & contracts", "Factions", "Build a camp", "Camp upkeep", "Crafting recipes", "Research", "World activity"}
};
int pause_menu_count(void) { return ui.menuTab == 0 ? 7 : ui.menuTab == 1 ? 7 : 8; }
const char *pause_menu_label(int row) {
    return row >= 0 && row < pause_menu_count() ? pauseLabels[ui.menuTab][row] : "";
}
static void pause_menu_open(void) {
    Actor *a = &g.actors[g.selected];
    int row = ui.selection;
    if (row < 0 || row >= pause_menu_count()) return;
    ui.menuRow = row;
    if (ui.menuTab == 0) {
        switch (row) {
        case 0: ui.menuReturn = 0; open_screen(SCREEN_WORLD); break;
        case 1: case 2: case 5:
            ui.saveMode = row != 2;
            ui.saveExit = row == 5;
            ui.saveReturn = SCREEN_MENU;
            save_refresh();
            open_screen(SCREEN_SAVES);
            ui.selection = ui.saveMode ? (ui.slot ? ui.slot : 1) : 0;
            break;
        case 3: ui.speed = ui.speed == 1 ? 2 : 1; break;
        case 4: open_help(0); break;
        case 6: ui.confirmAction = 1; open_screen(SCREEN_CONFIRM); break;
        }
        return;
    }
    static const int screens[2][9] = {
        {SCREEN_PACK, SCREEN_SKILLS, SCREEN_HEALTH, SCREEN_SQUAD, SCREEN_LAYERS, SCREEN_LOG, SCREEN_ENGAGE},
        {SCREEN_MAP, SCREEN_JOURNAL, SCREEN_FACTIONS, SCREEN_BUILD, SCREEN_CAMP, SCREEN_CRAFT, SCREEN_RESEARCH, SCREEN_LIFE}
    };
    ui.menuReturn = 1;
    int target = screens[ui.menuTab - 1][row];
    if (target == SCREEN_MAP) {
        ui.mapX = a->x;
        ui.mapY = a->y;
        ui.mapZoom = 0;atlas_sync();
    }
    if (target >= SCREEN_EXPEDITION && target <= SCREEN_LAYERS) depth_open(target);
    else open_screen(target);
}
void game_tick(float dt, Input in) {
    dt = clamp(dt, 0, .1f);
    if (ui.screen != SCREEN_WORLD || ui.paused || in.held || in.pressed ||
        fabsf(in.ax) + fabsf(in.ay) > .15f)
        interrupt_work();
    else
        ui.playerIdle = fminf(5, ui.playerIdle + dt);
    if (g.actorCount && g.actors[g.selected].autoattack && ui.jobGather[g.selected])
        interrupt_work();
    ui.noticeTime = fmaxf(0, ui.noticeTime - dt);
    log_tick(dt);
    int p = in.pressed;
    if (ui.screen != SCREEN_WORLD && ui.screen != SCREEN_TITLE)
        contract_tick();
    if(politics_input(in))return;
    if(limb_input(in))return;
    if(ui.screen==SCREEN_ORDERS)ui.screen=SCREEN_SQUAD;
    if(ui.screen==SCREEN_CARE)ui.screen=SCREEN_HEALTH;
    item_view_normalize();
    if(item_view_enabled()) {
        if(p&KEY_UP)item_view_move(-1);
        if(p&KEY_DOWN)item_view_move(1);
        p &= ~(KEY_UP|KEY_DOWN); in.pressed=p;
    }
    if (ui.screen != SCREEN_WORLD)
        ui.squadCyclePending = 0;
    if (ui.screen == SCREEN_WORLD) ui.menuReturn = 0;
    if (ui.menuReturn && ui.screen != SCREEN_MENU && ui.screen != SCREEN_HELP &&
        ui.screen != SCREEN_CONFIRM && ui.screen != SCREEN_SAVES && ui.screen != SCREEN_PASS &&
        ui.screen != SCREEN_SCENARIOS && ui.screen != SCREEN_CREATE && ui.screen != SCREEN_NAME &&
        (p & (KEY_CIRCLE | KEY_START))) {
        open_screen(p & KEY_START ? SCREEN_WORLD : SCREEN_MENU);
        ui.selection = ui.menuRow;
        if (p & KEY_START) ui.menuReturn = 0;
        return;
    }
    if(ui.screen==SCREEN_SKILLS&&(p&(KEY_LEFT|KEY_RIGHT))){ui.skillPage=!ui.skillPage;ui.selection=ui.skillPage?weapon_class(g.actors[g.selected].gear[0]):0;if(ui.selection<0)ui.selection=0;return;}
    if (ui.screen == SCREEN_MENU && (p & (KEY_L | KEY_R | KEY_LEFT | KEY_RIGHT))) {
        ui.menuTab = (ui.menuTab + (p & (KEY_R | KEY_RIGHT) ? 1 : 2)) % 3;
        ui.selection = 0;
        return;
    }
    /* World D-pad is for information and menus. Handle it before captivity
       input, without treating it as movement or cancelling travel/work. */
    if(ui.screen==SCREEN_WORLD && (in.held&KEY_L) && (p&KEY_SQUARE)) {
        ui.squadCyclePending=0;open_screen(SCREEN_ENGAGE);ui.attackFocus=combat_candidate(0);return;
    }
    if (ui.screen == SCREEN_WORLD) {
        if (p & KEY_UP)
            ui.statusPanel = !ui.statusPanel;
        else if (p & KEY_DOWN) {
            ui.skillPage=0;open_screen(SCREEN_SKILLS);
            return;
        } else if (p & KEY_LEFT) {
            open_screen(SCREEN_EQUIPPED);
            return;
        } else if (p & KEY_RIGHT) {
            depth_open(SCREEN_ORDERS);
            return;
        }
    }
    if(((ui.screen>=SCREEN_PACK && ui.screen<=SCREEN_SQUAD) || ui.screen==SCREEN_EQUIPPED) && (p&(KEY_L|KEY_R))) {
        cycle_squad(p&KEY_L?-1:1);item_view_normalize();return;
    }
    if (depth_input(dt,in) || expansion_input(dt, in))
        return;
    Actor *a = &g.actors[g.selected];
    if (ui.screen == SCREEN_TITLE) {
        if (p & KEY_UP)
            ui.selection = (ui.selection + 3) % 4;
        if (p & KEY_DOWN)
            ui.selection = (ui.selection + 1) % 4;
        if (p & KEY_CROSS) {
            if (ui.selection == 0 || ui.selection == 1) {
                scenario_menu(ui.selection == 1 ? 4 : 0, SCREEN_TITLE);
            } else if (ui.selection == 2) {
                ui.saveMode = 0;
                ui.saveReturn = SCREEN_TITLE;
                ui.saveExit = 0;
                save_refresh();
                open_screen(SCREEN_SAVES);
            } else
                open_help(0);
        }
        g.time += dt;
        return;
    }
    if (ui.screen == SCREEN_CONFIRM) {
        if (p & (KEY_CIRCLE | KEY_START)) {
            open_screen(ui.confirmAction == 1 ? SCREEN_MENU : SCREEN_SAVES);
            ui.selection = ui.confirmAction == 1 ? 6 : ui.slot;
            return;
        }
        if (p & KEY_CROSS) {
            if (ui.confirmAction == 1) {
                scenario_menu(0, SCREEN_MENU);
            } else if (ui.confirmAction == 2) {
                if (!load_slot(ui.slot)) {
                    open_screen(SCREEN_SAVES);
                    ui.selection = ui.slot;
                }
            } else if (ui.confirmAction == 3) {
                if (save_slot(ui.slot)) {
                    open_screen(ui.saveExit ? SCREEN_TITLE : SCREEN_MENU);
                    ui.saveExit = 0;
                } else {
                    open_screen(SCREEN_SAVES);
                    ui.selection = ui.slot;
                }
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
            count = ui.skillPage ? WEAPON_CLASSES + 1 : CORE_SKILL_ROWS;
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
            count = pause_menu_count();
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
        if (ui.screen == SCREEN_PACK) {
            if (p & KEY_CROSS)
                use_item(ui.selection);
            if ((p & KEY_SQUARE) && a->pack[ui.selection].item) {
                ui.passSlot = ui.selection;
                open_screen(SCREEN_PASS);
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
            if (ui.mapZoom) {
                float horizontal = ((in.held & KEY_RIGHT) != 0) - ((in.held & KEY_LEFT) != 0);
                float vertical = ((in.held & KEY_DOWN) != 0) - ((in.held & KEY_UP) != 0);
                float dx, dy;
                view_to_world(horizontal * 80 * dt, vertical * 160 * dt, &dx, &dy);
                ui.mapX += dx;
                ui.mapY += dy;
            }
            ui.mapX = clamp(ui.mapX, 3, WORLD - 4);
            ui.mapY = clamp(ui.mapY, 3, WORLD - 4);
            if (p & KEY_CROSS) {
                if(!ui.mapZoom && ui.mapChoice==0)return;
                ui.waypoint = 1;
                ui.travelX = ui.mapX;
                ui.travelY = ui.mapY;
                open_screen(SCREEN_WORLD);
                log_msg("Map pin placed.");
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
        else if (ui.screen == SCREEN_MENU && (p & KEY_CROSS))
            pause_menu_open();
        return;
    }
    if (p & KEY_TRIANGLE) {
        open_screen(SCREEN_PACK);
        return;
    }
    if (p & KEY_START) {
        ui.menuTab = ui.menuRow = ui.menuReturn = 0;
        open_screen(SCREEN_MENU);
        return;
    }
    if (p & KEY_SELECT) {
        ui.mapX = a->x;
        ui.mapY = a->y;
        ui.mapZoom = 0;atlas_sync();
        open_screen(SCREEN_MAP);
        return;
    }
    if (p & KEY_L)
        ui.squadCyclePending = 1;
    if ((p & (KEY_CROSS | KEY_L)) && (in.held & KEY_L) && (in.held & KEY_CROSS) && !g.interiorBuilding && a->prisonTime <= 0) {
        ui.squadCyclePending = 0;
        action_menu_open();
        return;
    }
    if (!(in.held & KEY_L) && ui.squadCyclePending) {
        ui.squadCyclePending = 0;
        cycle_squad(1);
        ui.contextChoice = 0;
        a = &g.actors[g.selected];
    }
    if (p & KEY_R)
        ui.paused = !ui.paused;
    if (a->prisonTime > 0) {
        if ((p & KEY_CIRCLE) && a->taskKind == TASK_CELL_LOCK) {
            a->taskKind = 0;
            log_msg("Stopped picking the cell lock.");
        }
        if (p & KEY_CROSS) {
            if (conscious(a) && a->taskKind != TASK_CELL_LOCK && a->taskKind != TASK_JAIL_CARE) {
                dialogue_open(g.selected);
                return;
            }
        }
        if (!ui.paused)
            simulate(dt * ui.speed);
        return;
    }
    if (p & KEY_CIRCLE)
        a->sneaking = !a->sneaking;
    if (p & KEY_SQUARE) {
        a->autoattack = !a->autoattack;
        if (a->autoattack && g.depth.actors[g.selected].order == ORDER_HOLD_FIRE)
            g.depth.actors[g.selected].order = ORDER_BALANCED;
        if(!a->autoattack){g.living.command[g.selected]=-1;a->target=-1;a->attackPhase=0;a->cooldown=fmaxf(.6f,a->cooldown);}
        log_msg(a->autoattack ? "Combat stance: attack nearby hostiles."
                              : "Passive stance: flee or move freely.");
    }
    if (p & KEY_CROSS) {
        if (g.interiorBuilding) {
            action_menu_open();
            return;
        }
        if (a->carrying >= 0) {
            g.actors[a->carrying].carriedBy = -1;
            a->carrying = -1;
            log_msg("Put down carried squad member.");
        } else
            action_menu_open();
    }
    if (ui.screen != SCREEN_WORLD || ui.paused)
        return;
    float dx = in.ax, dy = in.ay;
    if(fabsf(dx)+fabsf(dy)>.2f){g.society.queueCount[g.selected]=0;society_rescue_cancel(g.selected);}
    else society_queue_tick();
    if(ui.screen!=SCREEN_WORLD)return;
    float step = dt * ui.speed;
    if (g.interiorBuilding) {
        interior_move(dx, dy, step);
        simulate(step);
        g.autosave += step;
        if (g.autosave > 90) {
            g.autosave = 0;
            save_slot(0);
        }
        return;
    }
    if (fabsf(dx) + fabsf(dy) > .15f) {
        ui.contextChoice = 0;
        a->taskKind = 0;
        float wx, wy;
        view_to_world(dx, dy, &wx, &wy);
        move_actor(a, wx * 1.414214f, wy * 1.414214f, step);
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
    PTR("facing", &a->facing);
    PTR("shop_filter", &ui.shopFilter);
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
    PTR("mercs", g.mercActor);
    PTR("mercuntil", g.mercUntil);
    PTR("identity", &g.identityVersion);
    PTR("interior", &g.interiorBuilding);
    PTR("roompos", &g.interiorX);
    PTR("context", &ui.contextKind);
    PTR("task", &a->taskKind);
    PTR("mapzoom", &ui.mapZoom);
    PTR("waypoint", &ui.waypoint);
    PTR("travel", &ui.travelX);
#undef PTR
    /* Read-only QA values avoid racing PPSSPP's live-memory debugger against
       its headless frame loop. No telemetry file can change simulation state. */
#define VALUE(n, p, bytes)                                                                                   \
    do {                                                                                                     \
        const unsigned char *raw = (const unsigned char *)(p);                                               \
        fprintf(f, ",\"%s_value\":\"", n);                                                                   \
        for (unsigned k = 0; k < (bytes); k++)                                                               \
            fprintf(f, "%02x", raw[k]);                                                                      \
        fputc('"', f);                                                                                       \
    } while (0)
    VALUE("screen", &ui.screen, 4);
    VALUE("skillpage", &ui.skillPage, 4);
    VALUE("menutab", &ui.menuTab, 4);
    VALUE("creator_species", &ui.createRace, 4);
    VALUE("species", &a->race, 4);
    VALUE("elite_actors",g.civil.eliteActor,sizeof(g.civil.eliteActor));
    VALUE("corexp", g.actors[g.selected].xp, sizeof(g.actors[0].xp));
    VALUE("weaponxp", g.society.weaponXP[g.selected], sizeof(g.society.weaponXP[0]));
    VALUE("journalTab", &ui.journalTab, 4);
    VALUE("command", &g.living.command[g.selected], 4);
    VALUE("money", &g.money, 4);
    VALUE("squad", &g.squadCount, 4);
    VALUE("position", &a->x, 8);
    VALUE("gear", a->gear, sizeof(a->gear));
    VALUE("facing", &a->facing, 4);
    VALUE("shop_filter", &ui.shopFilter, 4);
    VALUE("pack", a->pack, sizeof(a->pack));
    VALUE("health", a->health, sizeof(a->health));
    VALUE("lost", &a->lostMask, 4);
    VALUE("prosthetics", &a->prostheticMask, 4);
    VALUE("limbtarget", &ui.limbTarget, 4);
    int fitted[4];for(int l=0;l<4;l++)fitted[l]=limb_installed(a,LARM+l);
    VALUE("fitted", fitted, sizeof(fitted));
    int lootFitted[4]={0};if(ui.lootActor>=0&&ui.lootActor<g.actorCount)for(int l=0;l<4;l++)lootFitted[l]=limb_installed(&g.actors[ui.lootActor],LARM+l);
    VALUE("loot_fitted", lootFitted, sizeof(lootFitted));
    int popNear=-1,popOnline=0,popUnits=0,popWearers=0;float popDistance=100000;
    for(int k=0;k<POP_GROUPS;k++){WorldEvent*e=&g.population.groups[k];if(e->spawned)popOnline++;float d=distance(a->x,a->y,e->x,e->y);if(d<popDistance){popDistance=d;popNear=k;}
        for(int j=0;j<e->count;j++){if(g.population.state[k].detached&(1<<j))continue;Actor*b=e->spawned&&e->members[j]>=0?&g.actors[e->members[j]]:&e->reserve[j];popUnits++;popWearers+=b->prostheticMask!=0;}}
    int popStats[8]={POP_GROUPS,popOnline,popUnits,popWearers,g.population.conflicts,g.population.deaths,g.population.arrivals,g.population.imported};VALUE("population",popStats,sizeof(popStats));
    int popInfo[12]={0};float popPosition[2]={0};
    if(popNear>=0){WorldEvent*e=&g.population.groups[popNear];PopulationState*m=&g.population.state[popNear];int data[12]={popNear,e->type,m->phase,e->serial,m->beds,e->spawned,e->count,m->stock[0],m->stock[1],m->stock[2],m->cacheQty,m->harvests};memcpy(popInfo,data,sizeof(data));popPosition[0]=e->x;popPosition[1]=e->y;}
    VALUE("pop_near",popInfo,sizeof(popInfo));VALUE("pop_position",popPosition,sizeof(popPosition));
    VALUE("fatigue",&a->fatigue,4);
    int lifeStats[8]={g.life.version,g.life.rescues,g.life.hunts,g.life.resupplies,g.life.relocations,g.life.settled,g.life.missions,g.life.succession};VALUE("life",lifeStats,sizeof(lifeStats));
    int warStats[5]={g.war.version,TOWNS,0,0,0};
    for(int i=0;i<g.actorCount;i++){
        Actor *n=&g.actors[i];
        warStats[2]+=g.war.prisonTown[i]>0&&!n->dead;
        if(n->active&&!n->dead&&n->speech>=24&&n->speechTime>0&&distance(a->x,a->y,n->x,n->y)<7)
            warStats[3]=n->speech;
    }
    for(int i=0;i<WAR_REPORTS;i++)warStats[4]+=g.war.report[i].kind==WAR_TAKEOVER;
    VALUE("war_stats",warStats,sizeof(warStats));
    VALUE("town_owners",g.townOwner,sizeof(g.townOwner));
    if(ui.screen==SCREEN_DIALOG){
        const char *line=dialogue_line(ui.dialogActor,ui.dialogPage);
        VALUE("dialogue_text",line,strlen(line));
    }
    int lifeNear[12]={0};if(popNear>=0){GroupLife*z=&g.life.groups[popNear];int v[12]={z->goal,z->camp,z->targetGroup,z->targetTown,z->losses,z->ambushSprung,z->rescues,z->hunts,z->supplies,z->settled,z->missionDone,z->relocations};memcpy(lifeNear,v,sizeof(v));}VALUE("life_near",lifeNear,sizeof(lifeNear));
    int lifeCarry[POP_MEMBERS*2];for(int j=0;j<POP_MEMBERS*2;j++)lifeCarry[j]=-1;if(popNear>=0){WorldEvent*e=&g.population.groups[popNear];for(int j=0;j<e->count;j++)if(e->spawned&&e->members[j]>=0){Actor*b=&g.actors[e->members[j]];lifeCarry[j*2]=b->carrying;lifeCarry[j*2+1]=b->carriedBy;}}VALUE("life_carry",lifeCarry,sizeof(lifeCarry));
    int council[10]={0};int councilTown=ui.tradeTown;if(councilTown>=0&&councilTown<TOWNS){TownPolitics*z=&g.life.towns[councilTown];int v[10]={councilTown,z->stage,z->support[0],z->support[1],z->pledge,z->relief,z->winner,z->leader,z->policy,g.townOwner[councilTown]};memcpy(council,v,sizeof(v));}VALUE("council",council,sizeof(council));

    VALUE("attack", &a->attackPhase, 4);
    VALUE("bleed", &a->bleed, 4);
    VALUE("woundflags", &a->woundFlags, 4);
    VALUE("roadside", &g.roadsideAid, sizeof(g.roadsideAid));
    VALUE("autoattack", &a->autoattack, 4);
    VALUE("trails", &g.trails, sizeof(g.trails));
    VALUE("time", &g.time, 4);
    VALUE("framecount", &rendered_frames, 4);
    VALUE("selected", &g.selected, 4);
    VALUE("selection", &ui.selection, 4);
    VALUE("weather", &g.weather, 4);
    VALUE("bounty", &a->bounty, 4);
    VALUE("blood", &a->blood, 4);
    VALUE("contracts", &g.contractsDone, 4);
    VALUE("mercs", g.mercActor, sizeof(g.mercActor));
    VALUE("mercuntil", g.mercUntil, sizeof(g.mercUntil));
    VALUE("identity", &g.identityVersion, 4);
    VALUE("interior", &g.interiorBuilding, 4);
    VALUE("roompos", &g.interiorX, 8);
    VALUE("context", &ui.contextKind, 8);
    VALUE("task", &a->taskKind, 4);
    VALUE("mapzoom", &ui.mapZoom, 4);
    int qaCount=ui.screen==SCREEN_ACTIONS?nearby_actions():0;char qaActions[ACTION_LIMIT][80]={{0}};for(int q=0;q<qaCount;q++)nearby_action_label(q,qaActions[q],80);
    VALUE("actioncount", &qaCount, 4);
    VALUE("actionlabels", qaActions, sizeof(qaActions));
    VALUE("rescuefreed",g.society.freed,sizeof(g.society.freed));
    VALUE("rescuehelper",g.society.rescueHelper,sizeof(g.society.rescueHelper));
    VALUE("feedcount", &ui.feedCount, 4);
    VALUE("feed", ui.feed, sizeof(ui.feed));
    VALUE("jobnear", g.society.jobNear, sizeof(g.society.jobNear));
    VALUE("jobfar", g.society.jobFar, sizeof(g.society.jobFar));
    VALUE("jobwarned", g.society.jobWarned, sizeof(g.society.jobWarned));
    VALUE("contractsraw", g.contracts, sizeof(g.contracts));
    VALUE("queuedactions", &g.society.queueCount[g.selected], 4);
    VALUE("mapchoice", &ui.mapChoice, 4);
    VALUE("mapcategory", &ui.mapCategory, 4);
    VALUE("mappin", &ui.mapX, 8);
    VALUE("waypoint", &ui.waypoint, 4);
    VALUE("unconscious", &a->unconscious, 4);
    VALUE("travel", &ui.travelX, 8);
    VALUE("carrying", &a->carrying, 4);
    VALUE("paused", &ui.paused, 4);
    VALUE("context_count", &ui.contextCount, 4);
    VALUE("stolen", a->stolen, sizeof(a->stolen));
    VALUE("job", &a->job, 4);
    VALUE("prison", &a->prisonTime, 4);
    VALUE("depth_route", &g.depth.route, 4);
    VALUE("depth_rescued", &g.depth.rescued, 4);
    VALUE("depth_reported", &g.depth.reported, 4);
    VALUE("depth_npcs", g.depth.npc, sizeof(g.depth.npc));
    VALUE("depth_captive", &g.depth.actors[g.selected].captive, 4);
    VALUE("depth_order", &g.depth.actors[g.selected].order, 4);
    VALUE("depth_group", &g.depth.actors[g.selected].group, 4);
    VALUE("depth_labor", &g.depth.buildings[a->taskIndex>=0&&a->taskIndex<MAX_BUILDINGS?a->taskIndex:0].labor, 4);
    for (int n = 0; n < g.buildingCount; n++)
        if (g.buildings[n].type == STORE) {
            VALUE("depot", g.buildings[n].storage, sizeof(g.buildings[n].storage));
            break;
        }
#undef VALUE
    fprintf(f, "}\n");
    fclose(f);
}
