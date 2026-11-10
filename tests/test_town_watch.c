#include "frontier_fixture.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

extern void init_actor(int, const char *, int, int, float, float, int);
extern void tick_expansion(float);

static Actor *isolated(void) {
    game_new(73819, 0);
    for (int n = 1; n < g.actorCount; n++)
        g.actors[n].active = 0;
    for (int k = 0; k < POP_GROUPS; k++) {
        g.population.groups[k].x = 1200 + k % 8 * 12;
        g.population.groups[k].y = 1200 + k / 8 * 12;
        g.life.groups[k].goal = LIFE_SETTLED;
    }
    g.roadsideAid.day = g.day;
    g.eventTimer = g.weatherTimer = 1000;
    Actor *a = &g.actors[0];
    a->x = a->y = 400;
    a->autoattack = 0;
    return a;
}

static int town_building(int town, int type) {
    for (int n = 0; n < g.buildingCount; n++)
        if (g.buildings[n].type == type && town_near(g.buildings[n].x, g.buildings[n].y, 17) == town)
            return n;
    assert(!"Expected a town service building");
    return -1;
}

static void step(float seconds) {
    for (int i = 0; i < (int)(seconds * 20 + .5f); i++)
        game_tick(.05f, (Input){0});
}

static void key(int button) { game_tick(.05f, (Input){.pressed = button}); }

static void at_door(Actor *a, int n) {
    fixture_door(a, n);
}

static void officer(int n, int town, float x, float y) {
    init_actor(n, "Market steward", GUARD, g.townOwner[town], x, y, 24);
    g.actors[n].homeTown = town;
    g.actors[n].voice = ROLE_SEATED_LEADER;
}

static void charges_and_payment(void) {
    Actor *a = isolated();
    int town = 3, f = g.townOwner[town], jail = g.field.jail[town] - 1;
    assert(f == SCAVENGERS && jail >= 0);
    at_door(a, jail);
    officer(1, town, a->x + .5f, a->y);
    Actor *b = &g.actors[1];
    crime_charge(a, f, 40, CRIME_THEFT);
    assert(!hostile(a, b));
    char text[100];
    crime_description(a, f, text, sizeof(text));
    assert(!strcmp(text, "Theft"));
    assert(save_game("build/v176-theft.sav") && load_game("build/v176-theft.sav"));
    crime_description(a, f, text, sizeof(text));
    assert(!strcmp(text, "Theft"));

    dialogue_open(1);
    ui.selection = 1;
    int silver = g.money;
    key(KEY_CROSS);
    assert(ui.dialogPage == DIALOG_BOUNTY && g.money == silver);
    key(KEY_DOWN);
    key(KEY_CROSS);
    assert(ui.dialogPage == 5 && g.money == silver && wanted_amount(a, f) == 40);
    ui.screen = SCREEN_WORLD;
    step(.1f);
    assert(ui.screen == SCREEN_DIALOG && ui.dialogPage == DIALOG_SUMMONS);
    b->target = b->attackTarget = 0;
    b->attackPhase = 1;
    g.living.anger[1] = 1;
    key(KEY_CROSS);
    assert(ui.screen == SCREEN_WORLD && g.money == silver - 40 && !wanted_amount(a, f));
    assert(!hostile(a, b) && b->target == -1 && !b->attackPhase && !g.living.anger[1]);
    assert(!g.charges[0][f]);

    crime_charge(a, f, 40, CRIME_TRESPASS);
    step(.1f);
    assert(ui.dialogPage == DIALOG_SUMMONS);
    key(KEY_DOWN);
    key(KEY_CROSS);
    assert(a->carriedBy == 1 && ui.screen == SCREEN_WORLD && g.money == silver - 40);
    step(4);
    assert(a->prisonTime > 0 && actor_inside(a) == jail + 1 && a->carriedBy < 0);

    a->xp[LOCKPICK] = 0;
    inventory_remove(a, TOOLS, inventory_count(a, TOOLS));
    assert(jail_pick_start(0));
    g.rng = 1; /* xorshift32 first roll 69, a failed novice attempt */
    tick_expansion(a->action + .1f);
    assert(a->xp[LOCKPICK] > 0 && !jail_cell_open(a) && a->prisonTime > 0);
    g.field.cellOpen[0] = 4;
    g.interiorX = 3.5f;
    g.interiorY = 6.3f;
    key(KEY_CROSS);
    assert(!actor_inside(a) && !a->prisonTime && wanted_amount(a, f) >= SERIOUS_BOUNTY);
    assert(g.charges[0][f] & CRIME_ESCAPE);
    actor_leave_room(1);
    assert(hostile(b, a));
    step(.1f);
    assert(b->target == 0);
    a->unconscious = 8;
    custody_tick(.1f);
    assert(a->carriedBy == 1 && g.field.intake[0] == jail + 1);
}

static void clothing_and_trespass(void) {
    Actor *a = isolated();
    int town = 3, jail = g.field.jail[town] - 1, faction = g.townOwner[town];
    at_door(a, jail);
    officer(1, town, a->x + .6f, a->y);
    starting_gear(a, 0);
    assert(disguise_faction(a) == faction);
    g.society.suspicion[0] = 99.99f;
    step(.2f);
    assert(!wanted_amount(a, faction) && ui.screen == SCREEN_WORLD);

    a = isolated();
    town = 11;
    jail = g.field.jail[town] - 1;
    faction = g.townOwner[town];
    at_door(a, jail);
    officer(1, town, a->x + .65f, a->y);
    Actor *watch = &g.actors[1];
    watch->gear[0] = SABRE;
    starting_gear(a, 1);
    assert(!hostile(watch, a));
    a->gear[2] = NONE;
    a->health[HEAD] = a->health[CHEST] = 3;
    a->health[LARM] = a->health[RARM] = 5;
    a->health[LLEG] = a->health[RLEG] = 5;
    assert(!wanted_amount(a, faction) && hostile(watch, a));
    step(18);
    assert(!a->dead && a->bleed == 0 && a->prisonTime > 0 && actor_inside(a) == jail + 1);
    assert(g.charges[0][faction] & CRIME_TRESPASS);
    assert(a->treatedMask & ((1 << HEAD) | (1 << CHEST)));
}

static void shop_and_inn(void) {
    Actor *a = isolated();
    int town = 3, shop = town_building(town, SHOP), inn = town_building(town, BAR);
    int forge = town_building(1, FORGE), bench = town_building(town, BENCH);
    assert(!town_design(town)->smith && town_design(1)->smith);
    at_door(a, shop);
    a->y += 1.2f;
    init_actor(1, "Counter keeper", TRADER, g.townOwner[town], a->x, a->y, 6);
    g.actors[1].homeTown = town;
    at_door(&g.actors[1], shop);
    assert(interior_enter(shop));
    assert(!actor_inside(&g.actors[1]));
    assert(town_npc_routine(1, .1f) && !actor_inside(&g.actors[1]));
    g.interiorY = 4.5f;
    assert(town_npc_routine(1, .1f) && actor_inside(&g.actors[1]) == shop + 1);
    for (int i = 0; i < 220; i++) town_npc_routine(1, .05f);
    fixture_service(ROOM_CHEST);
    a->sneaking = 1;
    assert(npc_sees(&g.actors[1], a, 6));
    interior_interact();
    assert(ui.screen == SCREEN_CHEST && interior_chest_count() == 2);
    interior_chest_take(0);
    interior_chest_take(0);
    assert(wanted_amount(a, g.townOwner[town]) == 80);
    interior_leave();
    assert(actor_inside(&g.actors[1]) == shop + 1);
    g.dayTime = 23;
    for (int i = 0; i < 220; i++) town_npc_routine(1, .05f);
    assert(!actor_inside(&g.actors[1]));
    assert(!building_has_interior(BENCH));
    at_door(a, bench);
    assert(!interior_enter(bench));

    InteriorProp props[ROOM_PROPS];
    int beds = 0, count = interior_floor_props(inn, inn_has_upstairs(inn), props);
    for (int i = 0; i < count; i++) beds += props[i].action == ROOM_REST;
    assert(beds == town_design(town)->beds && beds > town_design(1)->beds);
    g.dayTime = 8;
    at_door(a, forge);
    assert(interior_enter(forge));
    fixture_service(ROOM_SMITH_TRADE);
    interior_interact();
    assert(ui.screen == SCREEN_TRADE && ui.tradeSmith && shop_count() > 0);
    for (int i = 0; i < shop_count(); i++) {
        int slot = itemDefs[shop_item(i)].slot;
        assert(slot >= 0 && slot < 5);
    }
}

static void stock_and_recovery(void) {
    Actor *a = isolated();
    int town = 3, f = g.townOwner[town];
    assert(inventory_add_owned(a, RELIC, 1, f));
    int slot = 0;
    while (a->pack[slot].item != RELIC) slot++;
    float chance = stolen_sale_chance(town, slot);
    a->xp[STEALTH] = a->xp[THEFT] = 117612;
    assert(stolen_sale_chance(town, slot) > chance);
    a->xp[STEALTH] = a->xp[THEFT] = 0;
    int silver = g.money;
    g.rng = 1;
    assert(!sell_item(town, slot) && g.money == silver && a->pack[slot].item == RELIC);
    assert(g.charges[0][f] & CRIME_STOLEN_SALE);
    g.rng = 200;
    while (1) {
        unsigned x = g.rng;
        x ^= x << 13; x ^= x >> 17; x ^= x << 5;
        if (x % 1000 < 20) break;
        g.rng++;
    }
    assert(sell_item(town, slot) && g.money > silver && !a->pack[slot].item);
    init_actor(1, "Bandit", RAIDER, HOSTILE, a->x - 10, a->y, 3);
    Actor *b = &g.actors[1];
    living_gear(b, 7, 0, 17);
    assert(inventory_count(b, RATION) <= 1 && !inventory_count(b, TONIC) && !inventory_count(b, MEDKIT));
    b->dead = 1;
    b->carriedBy = 0;
    a->carrying = 1;
    key(KEY_CROSS);
    assert(ui.screen == SCREEN_ACTIONS);
    ui.selection = fixture_action(ACTION_PUT_DOWN);
    assert(ui.selection >= 0);
    key(KEY_CROSS);
    assert(b->active && b->carriedBy < 0 && a->carrying < 0 && depth_distance(a, b) < TALK_REACH);

    a->health[LARM] = 65;
    a->unconscious = .06f;
    step(.2f);
    assert(conscious(a) && playing_dead(a));
    assert(heal_actor(0, 0));
    step(a->action + .1f);
    assert(playing_dead(a) && a->treatedMask & (1 << LARM));
    game_tick(.05f, (Input){.ax = .8f});
    assert(!playing_dead(a));
    ui.feedCount = ui.logCount = 0;
    gain(a, LOCKPICK, 200);
    weapon_class_gain(a, SABRE, 200);
    assert(!ui.feedCount && !ui.logCount);
    log_msg("The final log entry.");
    step(2.5f);
    int priority, offset;
    assert(!strcmp(log_current(&priority, &offset), "The final log entry.") && offset == 0);
}

int main(void) {
    charges_and_payment();
    clothing_and_trespass();
    shop_and_inn();
    stock_and_recovery();
    puts("PASS: charges, review without payment, the watch, theft detection and sale odds, inn and smithy, carried bodies and prone recovery.");
}
