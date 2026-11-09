#include "game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

extern void init_actor(int, const char *, int, int, float, float, int);
extern int add_build(float, float, int, int, int);
extern void add_node(float, float, int, int, int, int);

static Actor *fresh(void) {
    game_new(73819, 0);
    g.weatherTimer = g.eventTimer = 10000;
    g.roadsideAid.day = g.day;
    for (int k = 0; k < POP_GROUPS; k++) {
        g.population.groups[k].x = 1200;
        g.population.groups[k].y = 1200;
        g.life.groups[k].goal = LIFE_SETTLED;
    }
    for (int n = 1; n < g.actorCount; n++)
        g.actors[n].active = 0;
    g.actors[0].autoattack = 0;
    g.actors[0].target = -1;
    return &g.actors[0];
}
static void press(int key) { game_tick(.05f, (Input){.held = key, .pressed = key}); }
static void step(int frames) {
    for (int i = 0; i < frames; i++)
        game_tick(.05f, (Input){0});
}
static int building_in(int town, int type) {
    for (int i = 0; i < g.buildingCount; i++)
        if (g.buildings[i].type == type && town_near(g.buildings[i].x, g.buildings[i].y, 15) == town)
            return i;
    assert(0);
    return -1;
}
static void at_door(Actor *a, int id) {
    a->x = g.buildings[id].x + 1.45f;
    a->y = g.buildings[id].y + .65f;
}
static void shops_and_theft(void) {
    Actor *a = fresh();
    int town = 3, shop = building_in(town, SHOP), faction = g.townOwner[town];
    for (int i = 0; i < g.buildingCount; i++)
        if (g.buildings[i].type == HOUSE)
            assert(distance(g.buildings[i].x, g.buildings[i].y, g.buildings[shop].x, g.buildings[shop].y) > 3);
    assert(nearest_ore(towns[town].x, towns[town].y, 17, NODE_ORE) >= 0);
    assert(depth_named(0) < 0 && g.depth.npc[NPC_NELL] == -1);
    at_door(a, shop);
    assert(interior_enter(shop));
    g.interiorX = 1.9f;
    g.interiorY = 4.5f;
    assert(save_game("build/v174/chest-ready.sav"));
    interior_interact();
    assert(ui.screen == SCREEN_CHEST && interior_chest_count() == 2);
    float before = g.time;
    step(15);
    assert(g.time == before);
    for (int i = 0; i < 3; i++)
        press(KEY_CROSS);
    assert(interior_chest_count() == 0 && g.townLife.theftCount[town] == 3);
    int stolen = 0;
    for (int i = 0; i < PACK; i++)
        if (a->stolen[i] == faction)
            stolen += a->pack[i].qty;
    assert(stolen == 3);
    int refresh = g.townLife.chestRefresh[shop];
    assert(save_game("build/v174/chest-roundtrip.sav") && load_game("build/v174/chest-roundtrip.sav"));
    a = &g.actors[0];
    assert(g.interiorBuilding == shop + 1 && g.townLife.chestRefresh[shop] == refresh);
    interior_interact();
    assert(ui.screen == SCREEN_CHEST && !interior_chest_count());
    press(KEY_CIRCLE);
    interior_leave();

    int guard = 1;
    init_actor(guard, "Dry Market watch", GUARD, faction, a->x, a->y, 12);
    g.actors[guard].homeTown = town;
    g.dayTime += .20f;
    assert(save_game("build/v174/search-ready.sav"));
    step(2);
    assert(ui.screen == SCREEN_DIALOG && ui.dialogPage == DIALOG_INSPECTION);
    press(KEY_CROSS);
    assert(ui.screen == SCREEN_WORLD && wanted_amount(a, faction) == 40);
    for (int i = 0; i < PACK; i++)
        assert(a->stolen[i] != faction);
    g.day += 2;
    step(2);
    assert(ui.screen == SCREEN_WORLD);
    g.day = refresh;
    at_door(a, shop);
    assert(interior_enter(shop));
    g.interiorX = 1.9f;
    g.interiorY = 4.5f;
    interior_interact();
    assert(interior_chest_count() == 2);
    printf("PASS: Dry Market spacing, local ore, owned stock, chest persistence and replenishment, pocket search and expiry.\n");
}
static void homes(void) {
    game_new(73819, 0);
    int present = 0, vacant = 0, bare = 0;
    for (int hour = 8; hour <= 20; hour += 12) {
        g.dayTime = hour;
        for (int i = 0; i < g.buildingCount; i++) {
            if (g.buildings[i].type != HOUSE)
                continue;
            int occupant = home_occupant(i);
            present += occupant >= 0;
            vacant += occupant < 0;
            InteriorProp props[8];
            int has_chest = 0, count = interior_props(i, props);
            for (int j = 0; j < count; j++)
                has_chest |= props[j].action == ROOM_CHEST;
            bare += !has_chest;
        }
    }
    assert(present && vacant && bare);
    for (int t = 0; t < TOWNS; t++) {
        int contact = work_contact(t);
        if (contact >= 0)
            assert(g.actors[contact].homeTown == t && g.actors[contact].faction == g.townOwner[t]);
    }
    printf("PASS: homes can have occupants, be vacant, or have no loot chest; work contacts belong to their town.\n");
}
static void rest_food_and_locks(void) {
    Actor *a = fresh();
    int bed = building_in(0, BED);
    a->x = g.buildings[bed].x + 1;
    a->y = g.buildings[bed].y;
    int money = g.money;
    assert(start_sleep(0) && a->x == g.buildings[bed].x && a->y == g.buildings[bed].y);
    assert(a->taskKind == 3 && g.money == money - 25);

    a = fresh();
    int inn = building_in(0, BAR);
    at_door(a, inn);
    assert(interior_enter(inn));
    g.interiorX = 2.35f;
    g.interiorY = 2;
    assert(save_game("build/v174/bed-ready.sav"));
    interior_interact();
    assert(a->taskKind == 3 && g.interiorX == 1.2f && g.interiorY == 2);
    game_tick(.1f, (Input){.ax = .8f});
    assert(!a->taskKind && g.interiorX > 1.9f);
    interior_leave();
    a->x = a->y = 400;
    a->hunger = 32;
    inventory_remove(a, RATION, inventory_count(a, RATION));
    init_actor(1, "Carry partner", PLAYER, FREE, 401, 400, 5);
    g.actors[1].squad = 1;
    g.actors[1].gear[5] = NONE;
    g.squadCount = 2;
    int rations = inventory_count(&g.actors[1], RATION);
    assert(rations > 0);
    step(1);
    assert(a->hunger > 75 && inventory_count(&g.actors[1], RATION) == rations - 1);

    int node = g.nodeCount;
    add_node(a->x, a->y, NODE_LOCKED, FREE, RELIC, 1);
    assert(!inventory_count(a, TOOLS));
    interact_context(1, node);
    assert(a->taskKind == 2);
    a->taskKind = 0;

    a->health[LARM] = 86;
    assert(heal_actor(0, 0));
    float mild = a->action;
    a->taskKind = 0;
    a->health[LARM] = 26;
    a->health[RARM] = 52;
    a->bleed = 1.3f;
    a->woundFlags = (1u << LARM) | (1u << RARM);
    assert(heal_actor(0, 0) && a->action > mild + 1);
    float clock = g.time, blood = a->blood, remaining = a->action;
    const int menus[] = {SCREEN_PACK, SCREEN_SKILLS, SCREEN_MENU, SCREEN_MAP, SCREEN_JOURNAL,
                         SCREEN_HELP, SCREEN_TRADE, SCREEN_DIALOG};
    for (unsigned i = 0; i < sizeof(menus) / sizeof(*menus); i++) {
        ui.screen = menus[i];
        step(8);
        assert(g.time == clock && a->blood == blood && a->action == remaining);
    }
    printf("PASS: actual bed placement and wake-up, shared auto-eating, lockpicking without tools, treatment severity and menu pause.\n");
}
static void contracts(void) {
    Actor *a = fresh();
    int seen[3] = {0};
    for (int t = 0; t < TOWNS; t++) {
        int offers = work_dialogue_count(t) - 1;
        assert(offers >= 0 && offers <= 2);
        seen[offers]++;
    }
    assert(seen[0] && seen[1] && seen[2]);
    int town = 3;
    for (int i = 1; i <= 4; i++)
        init_actor(i, "Road predator", BEAST, WILDLIFE, towns[town].x + (i == 4 ? 112 : 25 + i), towns[town].y, 7);
    assert(accept_contract(town, CONTRACT_HUNT));
    int money = g.money;
    a->x = g.actors[4].x;
    a->y = g.actors[4].y;
    g.townLife.squadHitAt[4] = g.time + 45;
    g.actors[4].blood = 20.001f;
    g.actors[4].bleed = 1;
    simulate(.05f);
    assert(g.contracts[0].progress == 0);
    for (int i = 1; i <= 3; i++) {
        a->x = g.actors[i].x;
        a->y = g.actors[i].y;
        g.townLife.squadHitAt[i] = g.time + 45;
        g.actors[i].blood = 20.001f;
        g.actors[i].bleed = 1;
        simulate(.05f);
        assert(g.contracts[0].progress == i);
    }
    g.actors[1].unconscious = 0;
    g.actors[1].blood = 20.001f;
    g.actors[1].bleed = 1;
    g.townLife.squadHitAt[1] = g.time + 45;
    simulate(.05f);
    assert(g.contracts[0].progress == 3);
    char text[200];
    contract_description(0, text, sizeof(text));
    assert(strstr(text, towns[town].name) && strstr(text, "claim") && contract_ready(0));
    assert(!complete_contract(0));
    a->x = towns[town].x;
    a->y = towns[town].y + 3;
    assert(save_game("build/v174/claim-ready.sav"));
    assert(complete_contract(0) && !complete_contract(0) && g.money == money + 420);
    assert(accept_contract(town, CONTRACT_ORE));
    assert(inventory_add(a, ORE, 1));
    contract_tick();
    assert(g.contracts[0].progress == 1);
    assert(strstr(ui.notice, "1/8"));
    assert(inventory_add(a, ORE, 7));
    contract_tick();
    assert(contract_ready(0));
    assert(complete_contract(0));
    printf("PASS: sparse postings and counted local defeats, progress and single payment in the right town.\n");
}
static void combat(void) {
    Actor *a = fresh();
    int t = 3, faction = g.townOwner[t];
    a->x = towns[t].x;
    a->y = towns[t].y + 3;
    for (int n = 1; n <= 3; n++) {
        init_actor(n, n == 3 ? "Dust hound" : "Town defender",
            n == 3 ? BEAST : n == 2 ? RECRUIT : GUARD, n == 3 ? WILDLIFE : faction,
            a->x + n * .5f, a->y, 10);
        g.actors[n].homeTown = t;
    }
    simulate(.05f);
    assert(g.actors[1].target == 3 && g.actors[2].target == 3);
    assert(!a->attackPhase);
    float full = movement_speed(a);
    a->health[LLEG] = 12;
    assert(limb_limping(a) && movement_speed(a) < full * .60f);
    a->health[RLEG] = 0;
    a->health[LLEG] = 0;
    assert(limb_crawling(a) && movement_speed(a) < full * .20f);
    a->health[LLEG] = a->health[RLEG] = species_health(a);

    a = fresh();
    a->x = a->y = 400;
    init_actor(1, "Practice raider", RAIDER, HOSTILE, 401.1f, 400, 12);
    g.depth.actors[1].order = ORDER_BLOCK;
    assert(save_game("build/v174/fight-ready.sav"));
    a->autoattack = 1;
    int starts = 0;
    float started = 0, last_cooldown = 0;
    int phase = 0;
    for (int frame = 0; frame < 200 && starts < 3; frame++) {
        simulate(.05f);
        if (a->attackPhase == 1 && phase != 1) {
            if (starts)
                assert(g.time - started <= last_cooldown + .18f);
            starts++;
            started = g.time;
            last_cooldown = a->cooldown;
        }
        phase = a->attackPhase;
    }
    assert(starts == 3);
    press(KEY_SQUARE);
    assert(!a->autoattack && !a->attackPhase);
    printf("PASS: town guards and armed allies defend, injuries slow movement, Fight repeats at cooldown, Passive cancels.\n");
}
int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    shops_and_theft();
    homes();
    rest_food_and_locks();
    contracts();
    combat();
    return 0;
}
