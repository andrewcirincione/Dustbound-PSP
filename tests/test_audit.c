#include "frontier_fixture.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
static int checks;
#define CHECK(x)                                                                                             \
    do {                                                                                                     \
        checks++;                                                                                            \
        if (!(x)) {                                                                                          \
            fprintf(stderr, "AUDIT FAIL line %d: %s\n", __LINE__, #x);                                       \
            return 1;                                                                                        \
        }                                                                                                    \
    } while (0)
static void fresh(void) {
    game_new(73819, 0);
    ui.screen = SCREEN_WORLD;
    for (int i = 1; i < g.actorCount; i++)
        g.actors[i].active = 0;
    g.actors[0].autoattack = 0;
}
static void empty(Actor *a) {
    memset(a->pack, 0, sizeof(a->pack));
    memset(a->stolen, 0, sizeof(a->stolen));
}
static void press(int k) {
    game_tick(.033f, (Input){k, k, 0, 0});
    game_tick(.033f, (Input){0});
}
static void step(float t) {
    for (int i = 0; i < (int)(t * 30); i++)
        simulate(1.f / 30);
}
static int owned(const Actor *a, int it, int owner) {
    for (int i = 0; i < PACK; i++)
        if (a->pack[i].item == it && a->stolen[i] == owner)
            return i;
    return -1;
}
static int stored(int b, int it) {
    int q = 0;
    for (int i = 0; i < 16; i++)
        if (g.buildings[b].storage[i].item == it)
            q += g.buildings[b].storage[i].qty;
    return q;
}
int main(void) {
    fresh();
    Actor *a = &g.actors[0], *b = &g.actors[1];
    empty(a);
    empty(b);
    CHECK(inventory_add(a, SABRE, 1));
    CHECK(inventory_add_owned(a, SABRE, 2, UNION));
    CHECK(owned(a, SABRE, 0) != owned(a, SABRE, UNION));
    int hot = owned(a, SABRE, UNION);
    a->gear[0] = STAFF;
    a->gearStolen[0] = SHOGUN;
    use_item(hot);
    CHECK(a->gear[0] == SABRE && a->gearStolen[0] == UNION);
    CHECK(a->pack[hot].qty == 1 && a->stolen[hot] == UNION);
    CHECK(owned(a, STAFF, SHOGUN) >= 0);
    CHECK(inventory_transfer(a, b, hot, 1));
    CHECK(a->pack[hot].item == 0 && a->stolen[hot] == 0);
    CHECK(owned(b, SABRE, UNION) >= 0 && owned(a, SABRE, 0) >= 0);
    CHECK(!inventory_add(a, ORE, 1000));
    CHECK(!inventory_transfer(a, a, 0, 1));
    ui.screen = SCREEN_EQUIPPED;
    ui.selection = 0;
    press(KEY_CROSS);
    CHECK(a->gear[0] == 0 && a->gearStolen[0] == 0 && owned(a, SABRE, UNION) >= 0);
    int clean = owned(a, SABRE, 0);
    hot = owned(a, SABRE, UNION);
    int money = g.money;
    CHECK(sell_item(8, hot));
    CHECK(g.money > money && a->pack[clean].qty == 1);
    empty(a);
    inventory_add(a, MEDKIT, 1);
    inventory_add_owned(a, MEDKIT, 1, UNION);
    a->health[CHEST] = 50;
    use_item(owned(a, MEDKIT, UNION));
    CHECK(a->taskKind == TASK_TREAT && owned(a, MEDKIT, UNION) >= 0 && a->health[CHEST] == 50);
    for (int i = 0; i < 180 && a->taskKind == TASK_TREAT; i++)
        simulate(.05f);
    int used = 0;
    for (int j = 0; j < PACK; j++)
        if (portion_base(a->pack[j].item) == MEDKIT && a->stolen[j] == UNION)
            used = portion_percent(a->pack[j].item);
    CHECK(used > 0 && used < 100 && fixture_dressings(a) == 100 + used && a->health[CHEST] > 50);
    /* Full-pack craft failure must restore both cargo and ownership bytes. */
    fresh();
    a = &g.actors[0];
    empty(a);
    a->x = 42;
    a->y = 44;
    g.buildingCount = 1;
    g.buildings[0] = (Building){.x = 42, .y = 44, .type = FORGE, .owner = 0};
    fixture_door(a, 0);
    CHECK(interior_enter(0));
    fixture_service(ROOM_FORGE);
    for (int i = 0; i < PACK; i++) {
        a->pack[i] = (Stack){gear_id(i, 0, 0), 2};
        a->stolen[i] = SHOGUN;
    }
    a->pack[0] = (Stack){ORE, 6};
    Actor before = *a;
    CHECK(!craft_item(0));
    CHECK(!memcmp(a, &before, sizeof(*a)));
    a->pack[1] = (Stack){0, 0};
    a->stolen[1] = 0;
    CHECK(craft_item(0));
    CHECK(owned(a, IRONBAR, SHOGUN) >= 0);
    empty(a);
    inventory_add(a, ORE, 3);
    inventory_add_owned(a, ORE, 3, UNION);
    CHECK(inventory_input_owner(a, ORE, 3) == 0 && inventory_input_owner(a, ORE, 4) == UNION);
    CHECK(craft_item(0));
    CHECK(owned(a, IRONBAR, 0) >= 0 && owned(a, ORE, UNION) >= 0);
    CHECK(craft_item(0));
    CHECK(owned(a, IRONBAR, UNION) >= 0);
    /* Navigation: help and cancel are exact round trips; save-and-title never writes immediately. */
    fresh();
    g.actors[g.selected].pack[17] = (Stack){ORE, 1};
    ui.screen = SCREEN_PACK;
    ui.selection = 17;
    ui.scroll = 9;
    press(KEY_SELECT);
    press(KEY_CIRCLE);
    CHECK(ui.screen == SCREEN_PACK && ui.selection == 17 && ui.scroll == 9);
    ui.screen = SCREEN_TITLE;
    ui.selection = 1;
    press(KEY_CROSS);
    CHECK(ui.screen == SCREEN_SAVES);
    press(KEY_CIRCLE);
    CHECK(ui.screen == SCREEN_TITLE && ui.selection == 1);
    ui.screen = SCREEN_MENU;
    ui.menuTab = 0;
    ui.selection = 5;
    press(KEY_CROSS);
    CHECK(ui.screen == SCREEN_SAVES && ui.saveExit);
    ui.selection = 5;
    press(KEY_CROSS);
    CHECK(ui.screen == SCREEN_CONFIRM && ui.slot == 5);
    press(KEY_CIRCLE);
    CHECK(ui.screen == SCREEN_SAVES && ui.selection == 5);
    press(KEY_CIRCLE);
    CHECK(ui.screen == SCREEN_MENU && ui.selection == 5);
    /* Target selection prefers actions within reach and the chord never changes the controlled actor. */
    fresh();
    a = &g.actors[0];
    a->x = 42;
    a->y = 44;
    g.buildingCount = 1;
    g.nodeCount = 1;
    g.buildings[0] = (Building){.x = 43.5f, .y = 44, .type = WELL};
    g.nodes[0] = (Node){.x = 43.3f, .y = 44, .type = NODE_ORE, .remaining = 20};
    find_context();
    CHECK(ui.contextKind == 2);
    CHECK(ui.contextCount == 1);
    int selected = g.selected;
    game_tick(.033f, (Input){KEY_L, KEY_L});
    game_tick(.033f, (Input){KEY_L | KEY_CROSS, KEY_CROSS});
    game_tick(.033f, (Input){0});
    CHECK(g.selected == selected && ui.contextKind == 2);
    interact();
    CHECK(a->taskKind == 0);
    a->x = 42.4f;
    ui.contextChoice = 0;
    find_context();
    CHECK(ui.contextKind == 1);
    interact();
    CHECK(a->taskKind == 1);
    /* Body recovery is explicit; theft remains marked without an omniscient automatic bounty. */
    fresh();
    a = &g.actors[0];
    b = &g.actors[1];
    a->x = b->x = 42;
    a->y = b->y = 44;
    b->active = b->squad = 1;
    b->dead = 0;
    b->unconscious = 100;
    b->bleed = 2;
    b->carriedBy = -1;
    g.squadCount = 2;
    g.nodeCount = g.buildingCount = 0;
    interact();
    CHECK(ui.screen == SCREEN_ASSIST);
    ui.selection = 1;
    press(KEY_CROSS);
    CHECK(a->carrying == 1 && b->carriedBy == 0 && b->bleed > 0);
    press(KEY_CROSS);
    CHECK(ui.screen == SCREEN_ACTIONS);
    ui.selection = fixture_action(ACTION_PUT_DOWN);
    CHECK(ui.selection >= 0);
    press(KEY_CROSS);
    CHECK(a->carrying == -1 && b->carriedBy == -1);
    empty(a);
    empty(b);
    inventory_add_owned(b, SABRE, 3, SHOGUN);
    ui.lootActor = 1;
    ui.screen = SCREEN_LOOT;
    CHECK(loot_take(0, 1));
    CHECK(inventory_count(a, SABRE) == 3 && owned(a, SABRE, SHOGUN) >= 0 && !b->pack[0].item);
    b->unconscious = 0;
    inventory_add(b, ORE, 2);
    CHECK(!loot_take(0, 0));
    b->squad = 0;
    b->faction = UNION;
    b->dead = 1;
    a->bounty = 0;
    CHECK(loot_take(0, 1));
    CHECK(a->bounty == 0 && owned(a, ORE, UNION) >= 0);
    inventory_add(b, ORE, 2);
    a->x += 2;
    CHECK(!loot_take(0, 1));
    /* Stations and services enforce reach, including the actual indoor workbench. */
    fresh();
    a = &g.actors[0];
    g.buildingCount = 1;
    g.buildings[0] = (Building){.x = 42, .y = 44, .type = FORGE, .owner = 0};
    a->x = 46;
    a->y = 44;
    CHECK(nearby_station(FORGE, 1.7f) < 0);
    a->x = 43;
    CHECK(nearby_station(FORGE, 1.7f) < 0);
    g.interiorBuilding = 1;
    g.interiorActor = 0;
    g.interiorX = 3.5;
    g.interiorY = 6.1;
    CHECK(nearby_station(FORGE, 1.7f) < 0);
    g.interiorX = 2.5;
    g.interiorY = 2.5;
    CHECK(nearby_station(FORGE, 1.7f) == 0);
    g.interiorBuilding = 0;
    a->x = towns[0].x + 5;
    a->y = towns[0].y + 4;
    b = &g.actors[1];
    b->active = 1;
    b->dead = 0;
    b->unconscious = 0;
    b->voice = ROLE_BARBER;
    b->kind = SETTLER;
    b->x = a->x + 6;
    b->y = a->y;
    CHECK(nearby_service(ROLE_BARBER) < 0);
    b->x = a->x + 1;
    CHECK(nearby_service(ROLE_BARBER) == 1);
    dialogue_open(1);
    ui.selection = 0;
    press(KEY_CROSS);
    CHECK(ui.screen == SCREEN_CREATE);
    /* Cooks fetch BOTH inputs, physically walk, cook, then bank output. */
    fresh();
    a = &g.actors[0];
    empty(a);
    a->x = 44;
    a->y = 44;
    a->job = JOB_COOK;
    g.buildingCount = 2;
    g.nodeCount = 0;
    g.buildings[0] = (Building){.x = 42, .y = 44, .type = STORE, .owner = 0};
    g.buildings[0].storage[0] = (Stack){GRAIN, 18};
    g.buildings[0].storage[1] = (Stack){WATER, 6};
    g.buildings[1] = (Building){.x = 48, .y = 44, .type = CAMPFIRE, .owner = 0};
    CHECK(strstr(job_status(a), "water"));
    ui.playerIdle = 5;
    step(75);
    CHECK(stored(0, RATION) > 0);
    CHECK(stored(0, GRAIN) < 18 && stored(0, WATER) < 6);
    CHECK(inventory_count(a, RATION) >= 2);
    CHECK(a->x > 42 && a->x < 49);
    for(int i=0;i<LANDMARKS;i++)g.depth.siteBuilding[i]=-1; /* This fixture replaces all world buildings. */
    memset(g.trails.bed, 0, sizeof(g.trails.bed));
    memset(g.field.jail, 0, sizeof(g.field.jail));
    memset(g.field.camp, 0, sizeof(g.field.camp));
    memset(g.frontier.lodge, 0, sizeof(g.frontier.lodge));
    CHECK(save_game("build/v052-workers.sav"));
    int q = stored(0, RATION);
    CHECK(load_game("build/v052-workers.sav"));
    CHECK(stored(0, RATION) == q && g.actors[0].job == JOB_COOK);
    a = &g.actors[0];
    a->job = JOB_MEDIC;
    empty(a);
    CHECK(strstr(job_status(a), "dressings"));
    b = &g.actors[1];
    b->squad = 1;
    b->dead = 0;
    for (int i = 0; i < LIMBS; i++)
        b->health[i] = 100;
    b->health[LLEG] = 20;
    CHECK(needs_aid(b));
    /* Profession perks are bounded, and UI uses the same calculations. */
    fresh();
    a = &g.actors[0];
    int wood = build_wood_cost(3), price = research_cost();
    a->xp[ENGINEERING] = 117612;
    a->xp[SCIENCE] = 117612;
    a->xp[FARMING] = 117612;
    CHECK(build_wood_cost(3) == wood - 3 && build_wood_cost(0) == 1);
    CHECK(research_cost() == price * 80 / 100 && harvest_yield(a) == 8);
    a->xp[COOKING] = 12 * 69 * 69;
    a->x = 42;
    a->y = 44;
    g.buildingCount = 1;
    g.buildings[0] = (Building){.x = 42, .y = 44, .type = CAMPFIRE, .owner = 0};
    empty(a);
    inventory_add(a, GRAIN, 3);
    inventory_add(a, WATER, 1);
    char info[240];
    recipe_status(1, info, sizeof(info));
    CHECK(strstr(info, "MAKES 4"));
    CHECK(craft_item(1) && inventory_count(a, RATION) == 4);
    /* Replay identical combat rolls to isolate toughness and perception. */
    fresh();
    a = &g.actors[0];
    b = &g.actors[1];
    g.actorCount = 2;
    g.buildingCount = g.nodeCount = 0;
    a->x = 42;
    a->y = 44;
    a->gear[0] = SABRE;
    a->xp[MELEE] = 117612;
    b->active = 1;
    b->squad = 0;
    b->kind = PLAYER;
    b->faction = FREE;
    b->x = 42.7f;
    b->y = 44;
    b->dead = 0;
    b->unconscious = b->stun = b->bleed = 0;
    b->autoattack = 0;
    memset(b->gear, 0, sizeof(b->gear));
    memset(b->xp, 0, sizeof(b->xp));
    static Game combatBase;
    combatBase = g;
    float wounds[2] = {0, 0};
    for (int high = 0; high < 2; high++) {
        g = combatBase;
        g.rng = 1255;
        g.actors[1].xp[TOUGHNESS] = high ? 117612 : 0;
        strike(0, 1);
        step(1.5f);
        for (int l = 0; l < LIMBS; l++)
            wounds[high] += 100 - g.actors[1].health[l];
    }
    CHECK(wounds[0] > 0 && wounds[1] < wounds[0] * .85f && wounds[1] > wounds[0] * .75f);
    g = combatBase;
    g.actors[0].gear[0] = CROSSBOW;
    g.actors[0].xp[CROSSBOWS] = 0;
    inventory_add(&g.actors[0], BOLTS, 10);
    combatBase = g;
    int hits[2] = {0, 0};
    for (int high = 0; high < 2; high++)
        for (int roll = 0; roll < 100; roll++) {
            g = combatBase;
            g.rng = 100 + roll * 997;
            g.actors[0].xp[PERCEPTION] = high ? 117612 : 0;
            strike(0, 1);
            step(2.5f);
            hits[high] += g.actors[1].impactKind == 1;
        }
    CHECK(hits[1] > hits[0]);
    /* Invalid routes don't start, and custody respects tactical pause. */
    fresh();
    ui.screen = SCREEN_MAP;
    ui.mapX = g.buildings[0].x;
    ui.mapY = g.buildings[0].y;
    press(KEY_CROSS);
    CHECK(!ui.waypoint && ui.screen == SCREEN_MAP);
    ui.screen = SCREEN_WORLD;
    a = &g.actors[0];
    a->prisonTime = 100;
    a->prisonTown = 0;
    ui.paused = 1;
    float time = g.time;
    for (int i = 0; i < 10; i++)
        game_tick(.1f, (Input){0});
    CHECK(g.time == time);
    printf("PASS: %d critical inventory, navigation, rescue, job, skill and range checks.\n", checks);
    return 0;
}
