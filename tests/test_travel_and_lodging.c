#include "frontier_fixture.h"
#include "civilization.h"
#include <stdio.h>
#include <string.h>

extern void init_actor(int, const char *, int, int, float, float, int);
extern int add_build(float, float, int, int, int);

static Actor *fresh(void) {
    game_new(73819, 0);
    g.weather = 0;
    g.weatherTimer = g.eventTimer = 10000;
    g.roadsideAid.day = g.day;
    for (int k = 0; k < POP_GROUPS; k++) {
        g.population.groups[k].x = 1200;
        g.population.groups[k].y = 1200;
        g.life.groups[k].goal = LIFE_SETTLED;
    }
    for (int i = 1; i < g.actorCount; i++)
        g.actors[i].active = 0;
    return &g.actors[g.selected];
}
static int building_at(int town, int type) {
    for (int i = 0; i < g.buildingCount; i++)
        if (g.buildings[i].type == type && building_town(i) == town)
            return i;
    assert(!"Missing building");
    return -1;
}
static void press(int key) {
    game_tick(.0333f, (Input){.held = key, .pressed = key});
}
static void step(float seconds) {
    for (int i = 0; i < (int)(seconds * 30); i++)
        game_tick(1.f / 30, (Input){0});
}
static void inn_floors(void) {
    Actor *a = fresh();
    int inn = building_at(3, BAR), small = building_at(1, BAR);
    assert(inn_has_upstairs(inn) && !inn_has_upstairs(small));
    InteriorProp props[ROOM_PROPS];
    int bars = 0, beds = 0, stairs = 0;
    int count = interior_floor_props(inn, 0, props);
    for (int i = 0; i < count; i++) {
        bars += props[i].action == ROOM_TRADE;
        beds += props[i].action == ROOM_REST;
        stairs += props[i].action == ROOM_STAIRS;
    }
    assert(bars == 1 && !beds && stairs == 1);
    count = interior_floor_props(inn, 1, props);
    for (int i = 0; i < count; i++)
        beds += props[i].action == ROOM_REST;
    assert(beds == town_design(3)->beds);
    fixture_door(a, inn);
    assert(interior_enter(inn));
    fixture_service(ROOM_STAIRS);
    interior_interact();
    assert(actor_floor(a) == 1 && room_floor_walkable(inn + 1, 1, depth_x(a), depth_y(a)));
    init_actor(1, "Keeper", SETTLER, g.townOwner[3], a->x, a->y, 12);
    actor_enter_room(1, inn);
    assert(depth_distance(a, &g.actors[1]) > 100 && !npc_sees(&g.actors[1], a, 20));
    fixture_service(ROOM_REST);
    a->sneaking = 1;
    a->fatigue = 75;
    interior_interact();
    assert(actor_in_bed(a) && !a->sneaking);
    press(KEY_CIRCLE);
    step(3);
    assert(actor_in_bed(a) && !a->sneaking && a->fatigue > 72 && a->fatigue < 74);
    assert(save_game("build/inn-upstairs.sav") && load_game("build/inn-upstairs.sav"));
    assert(actor_floor(a) && actor_in_bed(a));
    game_tick(.1f, (Input){.ax = .7f});
    assert(!actor_in_bed(a));
    fixture_service(ROOM_STAIRS);
    interior_interact();
    assert(!actor_floor(a));
    interior_leave();
    assert(!depth_room(a));
}
static void medic_and_movement(void) {
    Actor *a = fresh();
    int inn = building_at(3, BAR);
    fixture_door(a, inn);
    init_actor(1, "Town medic", SETTLER, g.townOwner[3], a->x, a->y, 35);
    Actor *m = &g.actors[1];
    m->voice = ROLE_MEDIC;
    a->unconscious = 15;
    roadside_aid_finished(1, g.selected);
    assert(a->carriedBy == 1);
    roadside_aid_tick(.05f);
    assert(actor_in_bed(a) && actor_floor(a) && depth_room(a) == inn + 1 && a->carriedBy < 0);
    InteriorProp props[ROOM_PROPS];
    int count = interior_floor_props(inn, 1, props), on_bed = 0;
    for (int i = 0; i < count; i++)
        on_bed |= props[i].action == ROOM_REST && distance(props[i].x, props[i].y, depth_x(a), depth_y(a)) < .01f;
    assert(on_bed);
    assert(!limb_crawling(a));
    a->woundFlags |= WOUND_CRITICAL_AWAKE;
    assert(!limb_crawling(a));
    a->health[LLEG] = a->health[RLEG] = 2;
    assert(limb_crawling(a));

    a = fresh();
    init_actor(1, "Traveling guard", GUARD, g.actors[g.selected].faction, a->x + 1, a->y, 15);
    m->health[LARM] = 60;
    m->bleed = 1.1f;
    inventory_add(m, MEDKIT, 1);
    assert(actor_self_aid(1) && treatment_patient(m) == 1);
    step(22);
    assert(m->bleed < .01f && (m->treatedMask & (1 << LARM)));
}
static void pack_and_coach(void) {
    Actor *a = fresh();
    int inn = building_at(3, BAR);
    fixture_door(a, inn);
    reveal_area(a);
    assert(interior_enter(inn));
    fixture_service(ROOM_STAIRS);
    interior_interact();
    assert(inventory_add_owned(a, CLOTH, 3, UNION));
    int slot = 0;
    while (a->pack[slot].item != CLOTH || a->stolen[slot] != UNION)
        slot++;
    assert(inventory_drop(a, slot, 2) && a->pack[slot].qty == 1);
    int drop = g.nodeCount - 1;
    assert(g.nodes[drop].type == NODE_DROPPED && g.drops[drop].floor && node_distance(a, drop) < .1f);
    assert(save_game("build/inn-drop.sav") && load_game("build/inn-drop.sav"));
    fixture_service(ROOM_STAIRS);
    interior_interact();
    assert(node_distance(a, drop) > 100);
    fixture_service(ROOM_STAIRS);
    interior_interact();
    g.interiorX = g.drops[drop].x;
    g.interiorY = g.drops[drop].y;
    interact_context(1, drop);
    assert(!g.nodes[drop].remaining && a->pack[slot].qty == 3 && a->stolen[slot] == UNION);

    int target = 0;
    g.money = 10000;
    int money = g.money;
    g.visitedTowns &= ~(1u << target);
    g.discovered[target] = 1;
    assert(!coach_travel(3, target) && g.money == money);
    g.visitedTowns |= 1u << target;
    g.dayTime = 23.9f;
    int price = coach_price(3, target), day = g.day;
    assert(coach_travel(3, target));
    assert(g.money == money - price && !depth_room(a) && town_near(a->x, a->y, 14) == target && g.day == day + 1);
    set_save_root("build");
    assert(save_slot(0));
    SaveInfo info;
    assert(save_inspect(0, &info) && info.day == g.day && info.minute == (int)(g.dayTime * 60));
}
static void early_raider(void) {
    Actor *a = fresh();
    float x = towns[3].x + 14, y = towns[3].y;
    assert(starter_country(x, y));
    int raider = 3;
    while (civil_elite_index(raider) >= 0 || depth_named(raider) >= 0 || regional_named(raider)) raider++;
    init_actor(raider, "Heavy raider", RAIDER, HOSTILE, x, y, 110);
    init_actor(2, "Road trader", TRADER, SCAVENGERS, x + 1, y, 5);
    int weapon = gear_id(2, MATERIALS - 1, QUALITIES - 1);
    assert(itemDefs[weapon].power > 23);
    a = &g.actors[2];
    memset(a->gear, 0, sizeof(a->gear));
    int hits = 0;
    for (int i = 0; i < 24; i++) {
        combat_impact(raider, 2, weapon);
        assert(a->impactDamage <= 23);
        hits += a->impactDamage > 0;
        for (int j = 0; j < LIMBS; j++) a->health[j] = species_health(a);
        a->x = x + 1;
        a->y = y;
        a->bleed = 0;
    }
    assert(hits > 4);
}

int main(void) {
    inn_floors();
    medic_and_movement();
    pack_and_coach();
    early_raider();
    puts("PASS: inn floors, occupied beds, medic recovery, crawling, NPC aid, dropped gear, coach fares and save time.");
    return 0;
}
