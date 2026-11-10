#include "frontier_fixture.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

extern void init_actor(int, const char *, int, int, float, float, int);
extern void add_node(float, float, int, int, int, int);
extern void tick_expansion(float);

static Actor *field(void) {
    game_new(73819, 0);
    Actor *a = &g.actors[0];
    a->x = a->y = 400;
    a->autoattack = 0;
    for (int n = 1; n < g.actorCount; n++)
        g.actors[n].active = 0;
    for (int k = 0; k < POP_GROUPS; k++) {
        g.population.groups[k].x = 1200 + k % 8 * 12;
        g.population.groups[k].y = 1200 + k / 8 * 12;
        g.life.groups[k].goal = LIFE_SETTLED;
    }
    g.roadsideAid.day = g.day;
    g.eventTimer = g.weatherTimer = 1000;
    return a;
}

static int pack_slot(const Actor *a, int item) {
    for (int i = 0; i < PACK; i++)
        if (a->pack[i].item == item)
            return i;
    return -1;
}

static void action(int type) {
    int count = nearby_actions();
    for (int i = 0; i < count; i++)
        if (nearby_action(i)->type == type && !nearby_action_reason(i)) {
            nearby_action_choose(i);
            return;
        }
    assert(!"Expected an enabled nearby action");
}

static void advance(float seconds) {
    for (int i = 0; i < (int)(seconds * 10 + .5f); i++)
        simulate(.1f);
}

static void starts_and_ore(void) {
    for (int i = 0; i < START_SCENARIOS; i++) {
        game_new(73819, 0);
        character_begin(i);
        character_finish();
        int expected = i == 2 ? gear_id(79, 0, 0) : i == 4 ? gear_id(76, 0, 0) : NONE;
        assert(g.actors[0].gear[5] == expected);
        int t = startScenarios[i].town;
        assert(nearest_ore(towns[t].x, towns[t].y, 28, NODE_ORE) >= 0);
        for (int n = 0; n < g.nodeCount; n++)
            if (g.nodes[n].type <= NODE_COPPER && g.nodes[n].remaining)
                assert(town_near(g.nodes[n].x, g.nodes[n].y, 16) < 0);
    }
}

static void gear_and_chests(void) {
    Actor *a = field();
    int capacity = actor_capacity(a), heavy = gear_id(84, 1, 1);
    assert(inventory_add(a, heavy, 1));
    use_item(pack_slot(a, heavy));
    assert(a->gear[0] == heavy && weapon_handling(a, heavy) > 1.5f);
    a->xp[STRENGTH] = 12 * 90 * 90;
    assert(weapon_handling(a, heavy) < 1.05f && actor_capacity(a) > capacity);
    assert(buy_item(3, BEDROLL_ITEM));
    use_item(pack_slot(a, BEDROLL_ITEM));
    assert(ui.screen==SCREEN_PLACE);
    int placed=0;
    for(int i=0;i<16;i++){
        float angle=i*.39269908f,x=a->x+1.6f*cosf(angle),y=a->y+1.6f*sinf(angle);
        if(placement_valid(1,-1,x,y)){ui.placeX=x;ui.placeY=y;game_tick(.033f,(Input){.pressed=KEY_CROSS});placed=1;break;}
    }
    assert(placed&&ui.screen==SCREEN_WORLD);
    int bed = 0;
    while (bed < g.buildingCount && portable_bedroll(bed) <= 0) bed++;
    assert(bed < g.buildingCount && portable_bedroll(bed) > 0 && depth_build_ready(bed) && !g.baseExists);
    assert(!inventory_count(a, BEDROLL_ITEM));
    assert(save_game("build/v175-roll.sav") && load_game("build/v175-roll.sav"));
    a = &g.actors[0];
    assert(portable_bedroll(bed) > 0 && depth_build_ready(bed));
    Building *b = &g.buildings[bed];
    init_actor(1, "Bandit", RAIDER, HOSTILE, b->x + 4, b->y, 6);
    assert(rest_at(a, bed, b->x, b->y, 0, 0));
    action(ACTION_PACK_BEDROLL);
    assert(!inventory_count(a, BEDROLL_ITEM) && portable_bedroll(bed) > 0);
    a->taskKind = 0;
    action(ACTION_PACK_BEDROLL);
    assert(inventory_count(a, BEDROLL_ITEM) == 1 && portable_bedroll(bed) < 0);
    int before = g.buildingCount;
    use_item(pack_slot(a, BEDROLL_ITEM));
    assert(ui.screen==SCREEN_PLACE);
    placed=0;
    for(int i=0;i<16;i++){
        float angle=i*.39269908f,x=a->x+1.6f*cosf(angle),y=a->y+1.6f*sinf(angle);
        if(placement_valid(1,-1,x,y)){ui.placeX=x;ui.placeY=y;game_tick(.033f,(Input){.pressed=KEY_CROSS});placed=1;break;}
    }
    assert(placed && g.buildingCount == before && portable_bedroll(bed) > 0);

    add_node(a->x, a->y, NODE_LOCKED, SCAVENGERS, CLOTH, 3);
    int n = g.nodeCount - 1;
    g.nodes[n].locked = 0;
    int cloth = inventory_count(a, CLOTH);
    interact_context(1, n);
    assert(ui.screen == SCREEN_CHEST && g.nodes[n].qty == 3 && inventory_count(a, CLOTH) == cloth);
    assert(chest_owner() == SCAVENGERS && interior_chest_count() == 1);
    interior_chest_take(0);
    assert(g.nodes[n].qty == 2 && inventory_count(a, CLOTH) == cloth + 1);
    interior_chest_take(0);
    interior_chest_take(0);
    assert(g.nodes[n].remaining && !g.nodes[n].qty && !interior_chest_count());
}

static void wounds(void) {
    Actor *a = field();
    float max = species_health(a);
    a->health[CHEST] = max - 50;
    a->health[LARM] = max - 20;
    a->bleed = .05f;
    a->woundFlags = 1 << CHEST;
    advance(10);
    float hp = a->health[CHEST];
    assert(a->bleed == 0 && wound_needs_dressing(a) && a->health[LARM] == max - 20);
    advance(10);
    assert(a->health[CHEST] == hp);
    int choices = nearby_actions();
    for (int n = 0; n < choices; n++) assert(nearby_action(n)->type != ACTION_PLAY_DEAD);
    a->unconscious = .1f;
    advance(.3f);
    assert(playing_dead(a) && conscious(a));
    action(ACTION_SELF_AID);
    for (int phase = 0; phase <= LIMBS && a->taskKind == TASK_TREAT; phase++)
        advance(a->action + .2f);
    assert(playing_dead(a) && (a->treatedMask & (1 << CHEST)) && (a->treatedMask & (1 << LARM)));
    float severe = a->health[CHEST], mild = a->health[LARM];
    advance(20);
    assert(a->health[CHEST] > severe && a->health[CHEST] < severe + 2);
    assert(a->health[LARM] - mild > a->health[CHEST] - severe);
    float trail = a->health[CHEST];
    for (int i = 0; i < 100; i++)
        injury_tick(a, &g.depth.actors[0], .1f, 1);
    assert(a->health[CHEST] > trail + 10);
    game_tick(.05f, (Input){.ax = .7f});
    assert(!playing_dead(a));
}

static unsigned successful_seed(void) {
    for (unsigned n = 1; n < 1000; n++) {
        unsigned x = n;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        if (x % 100 < 5)
            return n;
    }
    assert(0);
    return 1;
}

static void jail(void) {
    Actor *a = field();
    int b = g.field.jail[0] - 1;
    assert(b >= 0 && g.buildings[b].type == JAIL);
    const Building *cell = &g.buildings[b];
    a->x = cell->x + 1.45f;
    a->y = cell->y + 5;
    init_actor(1, "Jail guard", GUARD, g.townOwner[0], a->x, a->y - .6f, 18);
    g.actors[1].homeTown = 0;
    float y = a->y;
    a->unconscious = 8;
    a->health[CHEST] = 55;
    a->bleed = 1;
    crime_add(a, g.townOwner[0], 120);
    assert(inventory_add_owned(a, RELIC, 2, OUTLAWS));
    a->gear[2] = HAT;
    a->gearStolen[2] = SHOGUN;
    custody_tick(.1f);
    assert(a->carriedBy == 1 && !a->prisonTime && !actor_inside(a) && fabsf(a->y - y) < 1);
    assert(save_game("build/v175-intake.sav") && load_game("build/v175-intake.sav"));
    advance(15);
    a = &g.actors[0];
    assert(a->carriedBy < 0 && a->prisonTime > 0 && actor_inside(a) == b + 1);
    assert(!inventory_count(a, RELIC) && !a->gear[2] && !a->bleed && a->health[CHEST] < 100);
    int wanted = wanted_amount(a, g.townOwner[0]);
    assert(jail_pick_start(0));
    g.rng = successful_seed();
    tick_expansion(a->action + .1f);
    assert(jail_cell_open(a) && wanted_amount(a, g.townOwner[0]) == wanted);
    assert(save_game("build/v175-cell-open.sav") && load_game("build/v175-cell-open.sav"));
    tick_expansion(5.1f);
    a = &g.actors[0];
    assert(!jail_cell_open(a) && a->prisonTime > 0 && wanted_amount(a, g.townOwner[0]) == wanted);
    assert(jail_pick_start(0));
    g.rng = successful_seed();
    tick_expansion(a->action + .1f);
    assert(jail_cell_open(a));
    g.interiorX = 3.5f;
    g.interiorY = 6.35f;
    game_tick(.03f, (Input){.pressed = KEY_CROSS});
    assert(!a->prisonTime && !actor_inside(a) && wanted_amount(a, g.townOwner[0]) == wanted + 60);
}

static void slavery(void) {
    Actor *a = field();
    int b = g.field.camp[0] - 1;
    assert(b >= 0 && g.buildings[b].type == SLAVE_YARD);
    const Building *camp = &g.buildings[b];
    a->x = camp->x + 1.45f;
    a->y = camp->y + .65f;
    init_actor(1, "Camp overseer", GUARD, camp->faction, a->x, a->y, 20);
    g.actors[1].homeTown = -1;
    int weapon = a->gear[0];
    a->unconscious = 6;
    custody_tick(.1f);
    assert(a->carriedBy == 1);
    custody_tick(.1f);
    assert(slave_in_camp(a) && a->gear[1] == SLAVE_GARB && !a->gear[0] && !inventory_count(a, MEDKIT));
    a->unconscious = 0;
    float strength = a->xp[STRENGTH];
    custody_tick(.8f);
    assert(a->xp[STRENGTH] > strength);
    assert(slave_pick_start(0));
    g.rng = successful_seed();
    custody_tick(a->action + .1f);
    assert(!slave_in_camp(a) && !g.depth.actors[0].captive && g.field.escaped[0]);
    assert(save_game("build/v175-escape.sav") && load_game("build/v175-escape.sav"));
    a = &g.actors[0];
    a->x = camp->x;
    a->y = camp->y;
    action(ACTION_PROPERTY);
    assert(ui.screen == SCREEN_CHEST && interior_chest_count() >= 4);
    for (int i = 0; i < interior_chest_count(); i++)
        if (interior_chest_item(i)->item == weapon) {
            interior_chest_take(i);
            break;
        }
    assert(inventory_count(a, weapon) == 1);
}

static void witness_and_trade(void) {
    Actor *a = field();
    init_actor(1, "Watch", GUARD, UNION, 399, 400, 10);
    Actor *watch = &g.actors[1];
    watch->facing = 1;
    a->sneaking = 1;
    assert(npc_sees(watch, a, npc_sight_range(watch)));
    a->x = 397;
    assert(!npc_sees(watch, a, npc_sight_range(watch)));
    a->x = 400;
    a->sneaking = 0;
    watch->health[LARM] = 50;
    int rep = g.rep[UNION];
    action(ACTION_TREAT_OTHER);
    advance(a->action + .2f);
    assert(g.rep[UNION] == rep + 3 && (watch->treatedMask & (1 << LARM)));

    int trader = -1;
    for (int k = 0; k < POP_GROUPS && trader < 0; k++) {
        WorldEvent *e = &g.population.groups[k];
        if (e->type != POP_SMUGGLERS || !population_materialize(k))
            continue;
        for (int i = 0; i < e->count; i++)
            if (population_vendor(e->members[i])) trader = e->members[i];
    }
    assert(trader >= 0);
    Actor *m = &g.actors[trader];
    a->x = m->x;
    a->y = m->y + .6f;
    dialogue_open(trader);
    assert(dialogue_action(trader, 0) == 0);
    assert(strcmp(dialogue_line(trader, 3), dialogue_line(trader, 5)));
    game_tick(.03f, (Input){.pressed = KEY_CROSS});
    assert(ui.screen == SCREEN_TRADE && ui.tradeTown == -1 && ui.roadTrader == trader + 1);
    int part = PARTS, stock = market_quantity(-1, part), silver = g.money;
    assert(stock > 0 && buy_item(-1, part) && market_quantity(-1, part) == stock - 1 && g.money < silver);

    a = field();
    init_actor(1, "Pursuer", RAIDER, HOSTILE, 395.5f, 400, 8);
    simulate(.1f);
    assert(g.actors[1].target == 0);
    a->x = 408;
    float x = g.actors[1].x;
    advance(1);
    assert(g.actors[1].target == 0 && g.actors[1].x > x + 1);
    ui.waypoint = 1;
    ui.travelX = a->x + 1;
    ui.travelY = a->y;
    simulate(.1f);
    assert(!ui.waypoint);
}

static void unwelcome_entry(void) {
    Actor *a = field();
    int building = g.field.jail[0] - 1;
    Building *b = &g.buildings[building];
    a->x = b->x + 1.45f;
    a->y = b->y + .65f;
    int guard = g.actorCount++;
    init_actor(guard, "Watch", GUARD, b->faction, a->x, a->y, 12);
    g.rep[b->faction] = -45;
    assert(interior_enter(building));
    assert(!depth_room(&g.actors[guard]));
    g.depth.actors[guard].room = building + 1;
    g.depth.actors[guard].rx = 3;
    g.depth.actors[guard].ry = .9f;
    depth_room_tick(1);
    assert(g.depth.actors[0].trespass >= 1 && g.actors[guard].target < 0);
    depth_room_tick(6);
    assert(g.actors[guard].target == 0);
    interior_leave();
    assert(!actor_inside(a) && !g.depth.actors[0].trespass);
}

int main(void) {
    starts_and_ore();
    gear_and_chests();
    wounds();
    jail();
    slavery();
    witness_and_trade();
    unwelcome_entry();
    puts("PASS: start gear, ore spacing, soft weapon handling, deployable beds, chest contents, treated wounds, stealth, custody, shackles, pursuit, road trade, warnings and pin arrival.");
}
