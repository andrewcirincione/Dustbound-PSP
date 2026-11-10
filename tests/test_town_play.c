#include "frontier_fixture.h"
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
    fixture_door(a, id);
}
static void shops_and_theft(void) {
    Actor *a = fresh();
    int town = 3, shop = building_in(town, SHOP), faction = g.townOwner[town];
    for (int i = 0; i < g.buildingCount; i++)
        if (g.buildings[i].type == HOUSE)
            assert(distance(g.buildings[i].x, g.buildings[i].y, g.buildings[shop].x, g.buildings[shop].y) > 3);
    assert(nearest_ore(towns[town].x, towns[town].y, 28, NODE_ORE) >= 0);
    assert(depth_named(0) < 0 && g.depth.npc[NPC_NELL] == -1);
    at_door(a, shop);
    assert(interior_enter(shop));
    fixture_service(ROOM_CHEST);
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
    crime_clear(a, faction);
    g.day += 2;
    step(2);
    assert(ui.screen == SCREEN_WORLD);
    g.day = refresh;
    at_door(a, shop);
    assert(interior_enter(shop));
    fixture_service(ROOM_CHEST);
    interior_interact();
    assert(interior_chest_count() == 2);
    printf("PASS: Dry Market spacing, local ore, owned stock, chest persistence and replenishment, pocket search and expiry.\n");
}
static void homes(void) {
    game_new(73819, 0);
    g.actors[0].x = towns[3].x;
    g.actors[0].y = towns[3].y + 3;
    int present = 0, vacant = 0, bare = 0;
    for (int hour = 1; hour <= 20; hour += 19) {
        g.dayTime = hour;
        step(900);
        for (int i = 0; i < g.buildingCount; i++) {
            if (g.buildings[i].type != HOUSE)
                continue;
            int occupant = home_occupant(i);
            present += occupant >= 0;
            vacant += occupant < 0;
            InteriorProp props[ROOM_PROPS];
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
static void inn_traffic(void) {
    game_new(73819, 0);
    int town = 3, inn = building_in(town, BAR) + 1;
    g.actors[0].x = towns[town].x;
    g.actors[0].y = towns[town].y + 3;
    g.dayTime = 8;
    step(250);
    int guests = 0, keeper = -1, resident = -1;
    for (int i = 1; i < g.actorCount; i++) {
        Actor *a = &g.actors[i];
        if (depth_room(a) != inn) continue;
        if (npc_role(a) == ROLE_INNKEEPER) keeper = i;
        else guests++;
        if (npc_role(a) == ROLE_CITIZEN) resident = i;
        for (int j = 1; j < i; j++)
            if (depth_room(&g.actors[j]) == inn)
                assert(depth_distance(a, &g.actors[j]) > 1.1f);
    }
    assert(guests >= 2 && keeper >= 0 && resident >= 0);
    step(550);
    assert(!depth_room(&g.actors[resident]));
    assert(depth_room(&g.actors[keeper]) == inn ||
           building_door_distance(inn - 1, g.actors[keeper].x, g.actors[keeper].y) < 3);
    puts("PASS: Dry Market guests occupy separate places and return to the street while the keeper stays close.");
}
static void chest_migration(void) {
    Actor *a = fresh();
    int home = building_in(3, HOUSE);
    float x = g.buildings[home].x, y = g.buildings[home].y;
    int node = g.nodeCount;
    add_node(x, y, NODE_LOCKED, SCAVENGERS, RELIC, 2);
    assert(g.nodeCount == node + 1);
    g.frontier.version = 1;
    assert(save_game("build/v174/overlapping-chest.sav") && load_game("build/v174/overlapping-chest.sav"));
    const Node *chest = &g.nodes[node];
    assert(g.frontier.version == 3 && distance(chest->x,chest->y,x,y) > 1.5f && can_walk(chest->x,chest->y));
    assert(chest->item == RELIC && chest->owner == SCAVENGERS && chest->qty == 2);
    assert(a->active && !actor_inside(a));
    puts("PASS: a saved chest moves clear of a building and keeps its contents and owner.");
}
static void escort_job(void) {
    Actor *a = fresh();
    int town = 3;
    a->x = towns[town].x;
    a->y = towns[town].y + 3;
    assert(accept_contract(town, CONTRACT_ESCORT));
    int n = escort_contact(0);
    assert(n >= 0);
    Actor *convoy = &g.actors[n];
    float x, y;
    assert(contract_location(0,&x,&y) == town && x == convoy->x && y == convoy->y);
    step(180);
    assert(!g.society.jobStage[0] && convoy->x == x && convoy->y == y && !g.society.jobNear[0]);
    a->x = x;
    a->y = y;
    step(45);
    assert(g.society.jobStage[0] == 1 && g.society.jobNear[0] > 0 && distance(x,y,convoy->x,convoy->y) > 1);
    a->x = towns[town].x;
    a->y = towns[town].y;
    assert(contract_location(0,&x,&y) == town && x == convoy->x && y == convoy->y);
    float work = g.society.jobNear[0];
    step(70);
    assert(g.society.jobWarned[0] && convoy->x == x && convoy->y == y && g.society.jobNear[0] == work);
    a->x = x;
    a->y = y;
    step(45);
    assert(!g.society.jobWarned[0] && g.society.jobNear[0] > work && distance(x,y,convoy->x,convoy->y) > 1);
    puts("PASS: escorts wait to rendezvous, give a live marker, pause on separation and resume with the squad.");
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
    fixture_service(ROOM_REST);
    InteriorProp props[ROOM_PROPS];
    interior_props(inn, props);
    InteriorProp mattress = props[interior_context()];
    assert(save_game("build/v174/bed-ready.sav"));
    interior_interact();
    assert(a->taskKind == 3 && g.interiorX == mattress.x && g.interiorY == mattress.y);
    game_tick(.1f, (Input){.ax = .8f});
    assert(!a->taskKind && distance(g.interiorX, g.interiorY, mattress.x, mattress.y) > .3f);
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
    a->woundFlags = (1u << LARM) | (1u << RARM);
    assert(heal_actor(0, 0) && a->action > mild + 1);
    a->bleed = 1.3f;
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
        int offers = 0;
        while (contract_offer(t, offers) >= 0) offers++;
        assert(offers >= 0 && offers <= 2);
        seen[offers]++;
    }
    assert(seen[0] && seen[1] && seen[2]);
    int town = 3;
    for (int i = 1; i <= 4; i++)
        init_actor(i, "Road predator", BEAST, WILDLIFE, towns[town].x + (i == 4 ? 112 : 25 + i), towns[town].y, 7);
    assert(accept_contract(town, CONTRACT_HUNT));
    float leadX, leadY;
    assert(contract_location(0, &leadX, &leadY) == town);
    assert(leadX == g.actors[1].x && leadY == g.actors[1].y);
    g.actors[1].unconscious = 30;
    assert(contract_location(0, &leadX, &leadY) == town);
    assert(leadX == g.actors[2].x && leadY == g.actors[2].y);
    g.actors[1].unconscious = 0;
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
    assert(strstr(text, towns[town].name) && strstr(text, "Speak to") && contract_ready(0));
    assert(!complete_contract(0));
    a->x = towns[town].x;
    a->y = towns[town].y + 3;
    assert(save_game("build/v174/claim-ready.sav"));
    assert(complete_contract(0) && !complete_contract(0) && g.money == money + contract_reward(town, CONTRACT_HUNT));
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
static void settlement_layouts(void) {
    Actor *a = fresh();
    int rooms = 0, largeInns = 0, smallInns = 0, forges = 0, noForges = 0;
    for (int town = 0; town < TOWNS; town++) {
        const TownDesign *plan = town_design(town);
        largeInns += plan->beds >= 5;
        smallInns += plan->beds <= 2;
        ui.tradeTown = town;
        ui.tradeSmith = TRADE_GENERAL;
        assert(shop_count() <= 14);
        ui.tradeSmith = TRADE_INN;
        assert(shop_count() >= 2 && shop_count() <= 4);
        ui.tradeSmith = TRADE_SMITH;
        int stock = shop_count();
        forges += stock > 0;
        noForges += stock == 0;
        if (plan->smith) {
            int fine = 0, bestWeaponMaterial = 0;
            assert(stock >= 7 && stock <= 11);
            for (int row = 0; row < stock; row++) {
                int item = shop_item(row);
                if (itemDefs[item].quality < 2)
                    continue;
                fine++;
                int expensive = item_price(town, item, 0);
                g.frontier.venue[town][item] = TRADE_GENERAL;
                assert(expensive > item_price(town, item, 0));
                g.frontier.venue[town][item] = TRADE_SMITH;
                if (itemDefs[item].slot == 0 && itemDefs[item].material > bestWeaponMaterial)
                    bestWeaponMaterial = itemDefs[item].material;
            }
            assert(fine >= 3);
            if (plan->marketTier == 2 && !starter_country(towns[town].x, towns[town].y))
                assert(bestWeaponMaterial >= 4);
        }
        int ore = nearest_ore(towns[town].x, towns[town].y, 33, NODE_ORE);
        assert(ore >= 0 && town_near(g.nodes[ore].x, g.nodes[ore].y, 16) < 0);
        for (int b = 0; b < g.buildingCount; b++) {
            if (building_town(b) != town || !building_has_interior(g.buildings[b].type))
                continue;
            float w, d;
            int height;
            building_footprint(b, &w, &d, &height);
            const Building *shell = &g.buildings[b];
            for (int side = -1; side <= 1; side += 2) {
                assert(!can_walk(shell->x+side*(w-.02f),shell->y));
                assert(!can_walk(shell->x,shell->y+side*(d-.02f)));
                assert(!structure_blocks(b,shell->x+side*(w+.25f),shell->y,.13f));
                assert(!structure_blocks(b,shell->x,shell->y+side*(d+.25f),.13f));
            }
            for (int j = 0; j < b; j++)
                if (building_town(j) == town && building_has_interior(g.buildings[j].type)) {
                    float ww, dd;
                    building_footprint(j, &ww, &dd, &height);
                    assert(fabsf(g.buildings[b].x - g.buildings[j].x) >= w + ww ||
                           fabsf(g.buildings[b].y - g.buildings[j].y) >= d + dd);
                }
            at_door(a, b);
            assert(can_walk(a->x, a->y) && interior_enter(b) && interior_walkable(b + 1, g.interiorX, g.interiorY));
            rooms++;
            interior_leave();
        }
    }
    assert(rooms >= 110 && largeInns && smallInns && forges && noForges);
    puts("PASS: all nineteen settlements have walkable doors, spaced buildings, regional ore and tiered markets.");
}

static void lodging_and_bounties(void) {
    Actor *a = fresh();
    int lodge = g.frontier.lodge[0] - 1;
    assert(lodge >= 0 && !lodge_joined(lodge));
    at_door(a, lodge);
    assert(interior_enter(lodge));
    fixture_service(ROOM_MEMBERSHIP);
    int silver = g.money = 1200;
    interior_interact();
    assert(lodge_joined(lodge) && g.money == silver - 280 && lodge_joined(g.frontier.lodge[1] - 1));
    fixture_service(ROOM_TRAIN);
    int weapon = a->gear[0] = SABRE, skill = weapon_class(weapon);
    float limit = 12 * 19 * 19;
    g.society.weaponXP[0][skill] = limit - 15;
    interior_interact();
    assert(a->taskKind == TASK_TRAIN);
    step(190);
    assert(weapon_class_level(a, skill) == 20 && !a->taskKind);
    weapon_class_gain(a, weapon, 1e8f);
    gain(a, LOCKPICK, 1e8f);
    assert(weapon_class_level(a, skill) == 150 && level(a, LOCKPICK) == 150);
    fixture_service(ROOM_REST);
    init_actor(1, "Injured ally", PLAYER, FREE, a->x, a->y, 5);
    Actor *ally = &g.actors[1];
    ally->squad = 1;
    g.squadCount = 2;
    ally->unconscious = 50;
    ally->carriedBy = 0;
    a->carrying = 1;
    int row = fixture_action(ACTION_LAY_IN_BED);
    assert(row >= 0);
    nearby_action_choose(row);
    assert(a->carrying < 0 && ally->carriedBy < 0 && actor_in_bed(ally) && depth_room(ally) == lodge + 1);
    interior_leave();
    assert(depth_room(ally) == lodge + 1);

    int town = 0, jail = g.field.jail[town] - 1;
    at_door(a, jail);
    init_actor(2, "Jailkeeper", GUARD, g.townOwner[town], a->x, a->y, 10);
    g.actors[2].homeTown = town;
    init_actor(3, "Wanted raider", RAIDER, HOSTILE, a->x, a->y, 5);
    Actor *target = &g.actors[3];
    target->bountyReward = 150;
    target->unconscious = 30;
    target->carriedBy = 0;
    a->carrying = 3;
    assert(bounty_payers(3) & (1u << g.actors[2].faction));
    int money = g.money;
    row = fixture_action(ACTION_BOUNTY_TURN_IN);
    assert(row >= 0);
    nearby_action_choose(row);
    assert(g.money == money + 600 && a->carrying < 0 && target->prisonTime > 0 && depth_room(target) == jail + 1);
    assert(!bounty_turn_in(2) && !bounty_payers(3));
    puts("PASS: shared lodge access, skill caps, placement on a bed and a single bounty payout through the jailkeeper.");
}

static void nearby_tabs_and_health(void) {
    Actor *a = fresh();
    init_actor(1, "Witness", SETTLER, FREE, a->x, a->y + .8f, 5);
    init_actor(2, "Downed bandit", RAIDER, HOSTILE, a->x, a->y - .8f, 5);
    Actor *bandit = &g.actors[2];
    bandit->health[CHEST] = -10;
    bandit->bleed = 2;
    bandit->unconscious = 30;
    add_build(a->x + 1, a->y, BED, FREE, 0);
    g.contracts[0] = (Contract){.active = 1, .type = CONTRACT_HUNT, .town = 0,
                                .needed = 3, .baseline = -1};
    ui.screen = SCREEN_WORLD;
    press(KEY_CROSS);
    assert(ui.screen == SCREEN_ACTIONS);
    NearbyTabs tabs;
    nearby_action_tabs(&tabs);
    assert(tabs.tabCount == 3 && tabs.actors[tabs.activeTab] == 1);
    press(KEY_RIGHT);
    nearby_action_tabs(&tabs);
    assert(tabs.actors[tabs.activeTab] == 2);
    press(KEY_RIGHT);
    nearby_action_tabs(&tabs);
    assert(tabs.actors[tabs.activeTab] == -1);
    press(KEY_LEFT);
    nearby_action_tabs(&tabs);
    assert(tabs.actors[tabs.activeTab] == 2);
    int health = -1;
    for (int row = 0; row < tabs.rowCount; row++)
        if (nearby_action(tabs.rows[row])->type == ACTION_INSPECT_HEALTH)
            health = tabs.rows[row];
    assert(health >= 0);
    ui.selection = health;
    press(KEY_CROSS);
    assert(ui.screen == SCREEN_INSPECT_HEALTH && ui.nearbyTarget == 2 && g.selected == 0);
    float time = g.time;
    press(KEY_CROSS);
    assert(bandit->bleed == 2 && g.time == time && ui.screen == SCREEN_INSPECT_HEALTH);
    press(KEY_CIRCLE);
    assert(ui.screen == SCREEN_ACTIONS && ui.selection == health);
    ui.selection = fixture_action(ACTION_EXECUTE);
    assert(ui.selection >= 0);
    press(KEY_CROSS);
    assert(ui.screen == SCREEN_EXECUTE && !bandit->dead);
    press(KEY_CIRCLE);
    assert(ui.screen == SCREEN_ACTIONS && !bandit->dead);
    press(KEY_CROSS);
    int kills = g.kills;
    press(KEY_CROSS);
    assert(ui.screen == SCREEN_WORLD && bandit->dead && bandit->defeatCredited &&
           g.kills == kills + 1 && g.contracts[0].progress == 1);
    assert(!execute_opponent(2) && !can_execute(1));
    assert(fixture_action(ACTION_EXECUTE) < 0);
    puts("PASS: per-person tabs, read-only limb health, cancel and confirm execution, and single hunt credit.");
}
int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    settlement_layouts();
    lodging_and_bounties();
    nearby_tabs_and_health();
    shops_and_theft();
    homes();
    inn_traffic();
    chest_migration();
    escort_job();
    rest_food_and_locks();
    contracts();
    combat();
    return 0;
}
