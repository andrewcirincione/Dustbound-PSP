#include "game.h"
#include "save_fixture.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define D g.depth
extern void init_actor(int, const char *, int, int, float, float, int);
static int checks;
#define C(x)                                                                                            \
    do {                                                                                                \
        checks++;                                                                                       \
        if (!(x)) {                                                                                     \
            fprintf(stderr, "DEPTH FAIL line %d: %s\n", __LINE__, #x);                                  \
            exit(1);                                                                                    \
        }                                                                                               \
    } while (0)
static void step(float seconds) {
    for (int i = 0; i < (int)(seconds * 10); i++)
        simulate(.1f);
}
static void key(int k) { game_tick(.033f, (Input){k, k, 0, 0}); }
static void isolate(void) {
    for (int i = 1; i < g.actorCount; i++)
        g.actors[i].active = 0;
    g.actors[0].autoattack = 0;
}
static void fresh(void) {
    game_new(73819, 0);
    ui.screen = SCREEN_WORLD;
}
static void near_actor(int n) {
    g.actors[0].x = g.actors[n].x + .65f;
    g.actors[0].y = g.actors[n].y;
}
static void near_site(int n) {
    g.actors[0].x = D.siteX[n] + .3f;
    g.actors[0].y = D.siteY[n];
}
static void ppm(const char *path) {
    static uint32_t buf[512 * 272];
    render_game(buf, 512);
    FILE *f = fopen(path, "wb");
    assert(f);
    fprintf(f, "P6\n480 272\n255\n");
    for (int y = 0; y < 272; y++)
        for (int x = 0; x < 480; x++) {
            uint32_t c = buf[y * 512 + x];
            unsigned char b[3] = {c, c >> 8, c >> 16};
            fwrite(b, 1, 3, f);
        }
    fclose(f);
}
int main(int argc, char **argv) {
    fresh();
    C(depth_validate(&g));
    for (int i = 0; i < DEPTH_NPCS; i++)
        C(D.npc[i] == -1);
    for (int i = 0; i < LANDMARKS; i++) {
        C(can_walk(D.siteX[i], D.siteY[i]));
        C(D.siteBuilding[i] >= 0);
    }
    save_prior_fixture("build/depth-v5.sav", 5);
    C(load_game("build/depth-v5.sav"));
    C(D.version == 1 && depth_validate(&g));
    int actors = g.actorCount;
    C(save_game("build/depth-roundtrip.sav") && load_game("build/depth-roundtrip.sav"));
    C(g.actorCount == actors && D.npc[NPC_NELL] == -1);
    fresh();
    isolate();
    Actor *a = &g.actors[0];
    a->race = 2;
    a->health[LARM] = 35;
    inventory_add(a, PARTS, 2);
    int kits = inventory_count(a, MEDKIT);
    C(!heal_actor(0, 0) && inventory_count(a, MEDKIT) == kits);
    a->taskKind = 3;
    a->action = 100;
    step(3);
    C(a->health[LARM] == 35);
    C(depth_repair(0, 0) && a->health[LARM] == 35);
    step(7);
    C(a->health[LARM] > 35);
    C(!a->bleed);
    a->race = 0;
    a->health[CHEST] = 50;
    depth_wound(a, CHEST, 50);
    C(D.actors[0].trauma[CHEST] > 0);
    C(heal_actor(0, 0));
    step(a->action + .15f);
    float trauma = D.actors[0].trauma[CHEST];
    a->taskKind = 3;
    a->action = 100;
    step(5);
    C(D.actors[0].trauma[CHEST] < trauma && a->health[CHEST] <= 100 - D.actors[0].trauma[CHEST]);
    fresh();
    isolate();
    a = &g.actors[0];
    memset(a->pack, 0, sizeof(a->pack));
    int shirt = gear_id(45, 0, 0);
    C(itemDefs[shirt].weight <= 5);
    inventory_add_owned(a, shirt, 1, UNION);
    C(depth_layer(0) && D.actors[0].layer == shirt && D.actors[0].layerOwner == UNION);
    C(depth_armor(a, CHEST, SABRE, 0) > 0 && depth_armor(a, CHEST, SABRE, 99) == 0);
    fresh();
    isolate();
    a = &g.actors[0];
    a->x = 42;
    a->y = 43;
    C(build_structure(0));
    int b = g.buildingCount - 1;
    C(!depth_build_ready(b));
    C(!craft_item(1));
    step(25);
    C(depth_build_ready(b));
    a->x += .5f;
    inventory_add(a, GRAIN, 3);
    inventory_add(a, WATER, 1);
    Actor before = *a;
    C(!craft_item(1) && !memcmp(&before, a, sizeof(*a)));
    C(depth_fuel(b));
    C(craft_item(1) && D.buildings[b].fuel == 3);
    /* Formations follow the active member, even if another member was the leader in an old save. */
    fresh();
    isolate();
    a = &g.actors[0];
    Actor *ally = &g.actors[1];
    *ally = *a;
    ally->active = 1;
    a->x = 41;
    a->y = 44;
    ally->x = 50;
    ally->y = 44;
    g.squadCount = 2;
    D.actors[1].group = 1;
    D.groupLeader[1] = 1;
    float x = ally->x;
    depth_group_job(ally, 1);
    C(ally->x < x);
    select_squad_member(1);
    x = a->x;
    depth_group_job(a, 1);
    C(a->x > x);
    select_squad_member(0);
    D.actors[0].queued = 1;
    D.actors[0].queue[0] = JOB_MEDIC;
    D.actors[0].queue[1] = JOB_MINE;
    D.actors[0].queue[2] = JOB_HOLD;
    a->taskKind = 0;
    a->target = -1;
    int seam = nearest_ore(towns[0].x, towns[0].y, 150, NODE_ORE);
    C(seam >= 0);
    a->x = g.nodes[seam].x;
    a->y = g.nodes[seam].y;
    ui.playerIdle = 5;
    depth_queue(a);
    C(assigned_job(a) == JOB_MINE && a->job == JOB_FOLLOW);
    key(KEY_RIGHT);
    key(KEY_TRIANGLE);
    key(KEY_RIGHT);
    C(D.actors[0].paused);
    /* Captives escape with time, are interrupted by guards, retain confiscated
     * gear. */
    fresh();
    isolate();
    a = &g.actors[0];
    int captive = 1;
    init_actor(captive, "Shackled traveler", SETTLER, FREE, D.siteX[3], D.siteY[3], 3);
    D.actors[captive].captive = 1;
    a->xp[LOCKPICK] = 117612;
    near_actor(captive);
    C(depth_capture_action(captive));
    step(8);
    C(!D.actors[captive].captive && D.actors[captive].recognized > 0 && D.freed == 1);
    C(!g.actors[captive].squad);
    near_actor(captive);
    society_rescue_order(captive, ACTION_RESCUED_RECRUIT);
    C(g.actors[captive].squad);
    C(save_game("build/depth-escape.sav") && load_game("build/depth-escape.sav"));
    C(g.actors[captive].squad);
    fresh();
    isolate();
    captive = 1;
    init_actor(captive, "Shackled traveler", SETTLER, FREE, D.siteX[3], D.siteY[3], 3);
    D.actors[captive].captive = 1;
    near_actor(captive);
    int guard = 2;
    init_actor(guard, "Yard guard", GUARD, OUTLAWS, D.siteX[3] + 1, D.siteY[3], 12);
    C(depth_capture_action(captive));
    step(.2f);
    C(D.actors[captive].captive && D.actors[captive].escape == 0);
    /* Swept projectiles strike the intervening ally, and stop at actual
     * structures. */
    fresh();
    isolate();
    a = &g.actors[0];
    a->x = 40;
    a->y = 40;
    a->xp[CROSSBOWS] = 117612;
    ally = &g.actors[1];
    *ally = *a;
    ally->active = 1;
    ally->x = 42;
    ally->y = 40;
    memset(ally->gear, 0, sizeof(ally->gear));
    Actor *foe = &g.actors[2];
    *foe = *ally;
    foe->x = 45;
    foe->faction = HOSTILE;
    foe->squad = 0;
    g.rng = 1255;
    C(depth_fire(0, 2, CROSSBOW));
    for (int i = 0; i < 6; i++)
        depth_projectiles(.05f);
    C(ally->hitFlash > 0 && foe->hitFlash == 0);
    for (int j = 0; j < PROJECTILES; j++)
        D.shots[j].active = 0;
    ally->active = 0;
    int wall = g.buildingCount++;
    g.buildings[wall] = (Building){.x = 42, .y = 40, .type = WALL};
    C(depth_fire(0, 2, CROSSBOW));
    for (int i = 0; i < 12; i++)
        depth_projectiles(.05f);
    C(foe->hitFlash == 0);
    /* Crossing water must not use pedestrian collision. */
    g.buildingCount = 0;
    int water = 0;
    for (int yy = 20; yy < WORLD - 20 && !water; yy += 3)
        for (int xx = 20; xx < WORLD - 20 && !water; xx += 3)
            if (terrain_water(xx, yy)) {
                a->x = xx;
                a->y = yy;
                foe->x = xx + 5;
                foe->y = yy;
                water = 1;
            }
    C(water && !can_walk(a->x, a->y));
    C(depth_fire(0, 2, CROSSBOW));
    depth_projectiles(.02f);
    C(D.shots[0].active);
    /* A full pool can reuse ordinary fallen raiders, never named residents. */
    fresh();
    isolate();
    int ordinary = -1, named = g.civil.eliteActor[0] - 1;
    C(named >= 0);
    for (int i = 1; i < g.actorCount; i++)
        if (g.actors[i].kind == RAIDER && civil_elite_index(i) < 0 &&
            !politics_protected(i) && !regional_named(i) && !living_owned(i) && g.actors[i].carriedBy < 0) {
            ordinary = i;
            break;
        }
    C(ordinary >= 0);
    g.actorCount = MAX_ACTORS;
    g.actors[named].dead = 1;
    g.actors[ordinary].dead = 1;
    D.actors[ordinary].captive = 1;
    D.actors[ordinary].layer = shirt;
    g.baseExists = 1;
    g.baseX = 42;
    g.baseY = 43;
    g.raidTimer = 0;
    step(.2f);
    C(g.actors[named].dead);
    C(g.actors[ordinary].active && !g.actors[ordinary].dead && !D.actors[ordinary].captive && !D.actors[ordinary].layer);
    fresh();
    isolate();
    a = &g.actors[0];
    int shop = 2;
    a->x = g.buildings[shop].x + 1.45f;
    a->y = g.buildings[shop].y + .65f;
    a->target = -1;
    C(interior_enter(shop));
    ally = &g.actors[1];
    *ally = *a;
    ally->active = 1;
    ally->squad = 0;
    ally->faction = HOSTILE;
    D.actors[1].room = shop + 1;
    D.actors[1].rx = 3.5f;
    D.actors[1].ry = 5.4f;
    g.interiorX = 3.5f;
    g.interiorY = 6.1f;
    a->autoattack = 1;
    step(2);
    C(a->attackPhase || ally->hitFlash || a->hitFlash);
    C(save_game("build/depth-room.sav") && load_game("build/depth-room.sav"));
    C(D.actors[1].room == shop + 1);
    /* Indoor loot uses room coordinates, not the doorway's exterior position. */
    a = &g.actors[0];
    ally = &g.actors[1];
    ally->unconscious = 100;
    ally->pack[0] = (Stack){ORE, 1};
    D.actors[1].rx = .5f;
    D.actors[1].ry = .5f;
    ui.lootActor = 1;
    C(!loot_take(0, 0));
    D.actors[1].rx = g.interiorX + .3f;
    D.actors[1].ry = g.interiorY;
    C(loot_take(0, 0));
    /* Private interiors need a conscious occupant to witness trespass. */
    fresh();
    isolate();
    int house = -1;
    for (int i = 0; i < g.buildingCount; i++)
        if (g.buildings[i].type == HOUSE) {
            house = i;
            break;
        }
    C(house >= 0);
    a = &g.actors[0];
    a->x = g.buildings[house].x + 1.45f;
    a->y = g.buildings[house].y + .65f;
    C(interior_enter(house));
    depth_room_tick(13);
    C(!a->bounty && D.actors[0].trespass == 0);
    ally = &g.actors[1];
    *ally = *a;
    ally->active = 1;
    ally->squad = 0;
    ally->faction = g.buildings[house].faction;
    D.actors[1].room = house + 1;
    D.actors[1].rx = g.interiorX + .5f;
    D.actors[1].ry = g.interiorY;
    depth_room_tick(13);
    C(a->bounty == 80);
    a->gear[0] = CROSSBOW;
    D.actors[0].order = ORDER_HOLD_FIRE;
    ally->faction = HOSTILE;
    a->attackPhase = 0;
    depth_room_tick(.1f);
    C(!a->attackPhase);
    /* Reject corrupted v6 state, including NaNs and invalid actor references. */
    fresh();
    D.actors[0].trauma[0] = NAN;
    C(!depth_validate(&g));
    D.actors[0].trauma[0] = 0;
    D.npc[0] = MAX_ACTORS;
    C(!depth_validate(&g));
    fresh();
    g.dayTime = 23;
    C(!depth_trade_open(0));
    g.dayTime = 8;
    C(depth_trade_open(0));
    /* Seven simulated days: bounded state; exhaustive buy/resell spread across
     * catalog. */
    isolate();
    g.actors[0].race = 2;
    g.actors[0].job = JOB_HOLD;
    step(12600);
    C(g.day >= 8 && depth_validate(&g));
    for (int town = 0; town < TOWNS; town++)
        for (int reputation = -100; reputation <= 100; reputation += 20) {
            g.rep[g.townOwner[town]] = reputation;
            for (int item = 1; item < ITEMS; item++)
                C(item_price(town, item, 1) <= item_price(town, item, 0));
        }
    printf("PASS: %d depth checks, seven-day campaign, all catalog price spreads.\n", checks);
    if (argc > 1) {
        char path[512];
        /* Prepared blood-loss scenario: the controller orders travel while
         * conscious, then observes a real knockout during the journey. */
        fresh();
        isolate();
        g.actors[0].x = 34;
        g.actors[0].y = 31;
        g.actors[0].blood = 40;
        g.actors[0].bleed = 2;
        snprintf(path, sizeof(path), "%s/DUST_1.sav", argv[1]);
        C(save_game(path));
        fresh();
        isolate();
        near_site(3);
        D.actors[0].captive = 1;
        D.actors[0].confiscated = g.actors[0].gear[0];
        g.actors[0].gear[0] = 0;
        snprintf(path, sizeof(path), "%s/DUST_4.sav", argv[1]);
        C(save_game(path));
        fresh();
        isolate();
        g.actors[0].x = 42;
        g.actors[0].y = 43;
        inventory_add(&g.actors[0], GRAIN, 3);
        inventory_add(&g.actors[0], WATER, 1);
        inventory_add(&g.actors[0], COAL, 2);
        inventory_add(&g.actors[0], PARTS, 2);
        inventory_add(&g.actors[0], gear_id(45, 0, 0), 1);
        C(build_structure(0));
        snprintf(path, sizeof(path), "%s/DUST_5.sav", argv[1]);
        C(save_game(path));
        fresh();
        ui.noticeTime = 0;
        for (int screen = SCREEN_EXPEDITION; screen <= SCREEN_LAYERS; screen++) {
            ui.screen = screen;
            ui.selection = 0;
            ui.depthSite = 0;
            snprintf(path, sizeof(path), "%s/screen-%d.ppm", argv[1], screen);
            ppm(path);
        }
        for (int i = 20; i < GUIDE_PAGES; i++) {
            ui.screen = SCREEN_HELP;
            ui.selection = i;
            snprintf(path, sizeof(path), "%s/guide-%d.ppm", argv[1], i);
            ppm(path);
        }
    }
    return 0;
}
