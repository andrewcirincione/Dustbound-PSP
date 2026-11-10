#include "frontier_fixture.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int total, failed;
#define CHECK(x, msg)                                                                                        \
    do {                                                                                                     \
        total++;                                                                                             \
        if (!(x)) {                                                                                          \
            failed++;                                                                                        \
            printf("FAIL %02d: %s (line %d)\n", total, msg, __LINE__);                                       \
        } else                                                                                               \
            printf("PASS %02d: %s\n", total, msg);                                                           \
    } while (0)
static void fresh(void) { game_new(73819, 0); }
static void finish_build(void) {
    int b=g.buildingCount-1;
    for(int i=0;i<1200&&g.depth.buildings[b].labor>0;i++)simulate(.1f);
    if(g.buildings[b].type==CAMPFIRE||g.buildings[b].type==FORGE)depth_fuel(b);
}
static void clear_pack(Actor *a) { memset(a->pack, 0, sizeof(a->pack)); }
static int find_kind(int k) {
    for (int i = 0; i < g.actorCount; i++)
        if (g.actors[i].kind == k)
            return i;
    return -1;
}
static void press(int key) {
    game_tick(.033f, (Input){key, key, 0, 0});
    game_tick(.033f, (Input){0, 0, 0, 0});
}
int main(void) {
    fresh();
    Actor *a = &g.actors[0];
    CHECK(g.actorCount > 140 && g.buildingCount >= 150 && g.nodeCount > 450,
          "world includes sixteen towns, residents, hostiles and resources");
    CHECK(g.squadCount == 1 && g.money == 650, "wanderer starts with one member and 650 silver");
    CHECK(can_walk(a->x, a->y), "player starts on walkable ground");
    CHECK(!can_walk(g.buildings[0].x, g.buildings[0].y) && !can_walk(-1, 2), "buildings and map boundaries block movement");
    clear_pack(a);
    CHECK(inventory_add(a, ORE, 5) && inventory_count(a, ORE) == 5, "inventory can add and count a stack");
    CHECK(!inventory_remove(a, ORE, 6) && inventory_count(a, ORE) == 5, "insufficient removal is atomic");
    CHECK(inventory_remove(a, ORE, 5) && inventory_count(a, ORE) == 0, "removal clears an exhausted slot");
    for (int i = 1; i <= PACK; i++)
        inventory_add(a, i, 1);
    CHECK(!inventory_add(a, TONIC, 1), "full inventory rejects new items");
    int m = g.money, s = g.stock[0][TONIC];
    CHECK(!buy_item(0, TONIC) && g.money == m && g.stock[0][TONIC] == s,
          "failed purchase does not consume money or shop stock");
    fresh();
    a = &g.actors[0];
    m = g.money;
    int ask = item_price(0, LEATHER, 0);
    CHECK(buy_item(0, LEATHER) && g.money == m - ask && inventory_count(a, LEATHER) == 1,
          "buying updates money, stock and inventory");
    int slot = -1;
    for (int i = 0; i < PACK; i++)
        if (a->pack[i].item == LEATHER)
            slot = i;
    use_item(slot);
    CHECK(a->gear[1] == LEATHER && inventory_count(a, RAGS) == 1,
          "equipping armor swaps old armor back into pack");
    m = g.money;
    for (int i = 0; i < PACK; i++)
        if (a->pack[i].item == RAGS)
            slot = i;
    int quoted = item_price(0, RAGS, 1);
    CHECK(sell_item(0, slot) && g.money == m + quoted, "selling pays the displayed resale price");
    g.money = 0;
    s = g.stock[0][RATION];
    CHECK(!buy_item(0, RATION) && g.stock[0][RATION] == s, "unaffordable trade cannot alter stock");
    fresh();
    int r = find_kind(RECRUIT);
    CHECK(recruit_actor(r) && g.squadCount == 2 && g.actors[r].squad,
          "recruitment creates a controllable squad member");
    CHECK(!recruit_actor(r) && g.squadCount == 2, "same companion cannot be recruited twice");
    press(KEY_L);
    CHECK(g.selected == r, "L cycles the controlled squad member");
    g.selected = 0;
    a = &g.actors[0];
    g.money = 10000;
    for (int i = 0; i < g.actorCount; i++)
        if (g.actors[i].kind == RECRUIT)
            recruit_actor(i);
    CHECK(g.squadCount == 8, "squad recruitment enforces eight-member cap");
    fresh();
    a = &g.actors[0];
    float sp = movement_speed(a), xp = a->xp[ATHLETICS];
    a->x = 40;
    a->y = 40;
    move_actor(a, 1, 0, 1);
    CHECK(a->x > 40 && a->xp[ATHLETICS] > xp, "movement trains athletics");
    inventory_add(a, ORE, 80);
    CHECK(movement_speed(a) < sp * .5f, "heavy loads slow movement");
    clear_pack(a);
    a->lostMask = 1 << LLEG;
    CHECK(movement_speed(a) < sp * .3f, "a missing leg severely reduces movement");
    inventory_add(a, LEG, 1);
    inventory_add(a, PARTS, 2);
    use_item(0);
    game_tick(.033f,(Input){KEY_CROSS,KEY_CROSS,0,0});
    game_tick(.033f,(Input){KEY_CROSS,KEY_CROSS,0,0});
    CHECK((a->prostheticMask & (1 << LLEG)) && movement_speed(a) > sp * .8f,
          "equipping a prosthetic restores leg function");
    fresh();
    a = &g.actors[0];
    int enemy = find_kind(RAIDER);
    Actor *e = &g.actors[enemy];
    a->x = 40;
    a->y = 40;
    e->x = 41;
    e->y = 40;
    for (int i = 0; i < GEARS; i++)
        e->gear[i] = 0;
    for (int i = 1; i < g.actorCount; i++)
        if (i != enemy)
            g.actors[i].active = 0;
    e->kind = PLAYER; /* Stationary nonsquad training target; civilians now flee assault. */
    a->autoattack = 0;
    float health = 0;
    for (int i = 0; i < LIMBS; i++)
        health += e->health[i];
    int bleeding = 0;
    for (int i = 0; i < 15; i++) {
        a->cooldown = 0;
        a->attackPhase = 0;
        e->stun = 0;
        strike(0, enemy);
        for (int j = 0; j < 12; j++) {
            simulate(.1f);
            bleeding |= e->bleed > 0;
        }
    }
    float after = 0;
    for (int i = 0; i < LIMBS; i++)
        after += e->health[i];
    CHECK(after < health && a->xp[MELEE] > 48, "melee causes body-part damage and skill XP");
    CHECK(bleeding, "cutting weapons cause bleeding");
    a->health[CHEST] = 40;
    a->bleed = 2;
    int kits = fixture_dressings(a);
    e->active = 0;
    a->autoattack = 0;
    int aid = heal_actor(0, 0);
    for (int j = 0; j < 130; j++) simulate(.1f);
    CHECK(aid && a->bleed == 0 && (a->treatedMask & (1 << CHEST)) && a->health[CHEST] < 45 &&
              fixture_dressings(a) < kits && fixture_dressings(a) > kits - 100,
          "first aid takes time, stops bleeding and starts gradual healing");
    a->hunger = 30;
    int food = inventory_count(a, RATION);
    simulate(.1f);
    CHECK(a->hunger > 70 && inventory_count(a, RATION) == food - 1,
          "hungry squad automatically eats rations");
    a->unconscious = 10;
    a->health[HEAD] = 100;
    a->health[CHEST] = -2;
    simulate(1);
    CHECK(a->health[HEAD] == 100 && a->health[CHEST] > -2,
          "knockout recovery does not damage healthy body parts");
    a->unconscious = 0;
    a->blood = 15;
    simulate(.1f);
    CHECK(a->unconscious > 0, "blood loss causes unconsciousness");
    a->blood = .01f;
    a->bleed = 10;
    simulate(1);
    CHECK(a->dead, "fatal blood loss causes persistent death");
    fresh();
    a = &g.actors[0];
    enemy = find_kind(RAIDER);
    e = &g.actors[enemy];
    a->x = 40;
    a->y = 40;
    e->x = 45;
    e->y = 40;
    a->gear[0] = CROSSBOW;
    inventory_add(a, BOLTS, 2);
    e->kind = PLAYER; /* Stationary nonsquad training target; civilians now flee assault. */
    a->autoattack = 0;
    for (int i = 1; i < g.actorCount; i++)
        if (i != enemy)
            g.actors[i].active = 0;
    strike(0, enemy);
    for (int j = 0; j < 15; j++)
        simulate(.1f);
    CHECK(inventory_count(a, BOLTS) == 1, "crossbow attacks consume ammunition at range");
    a->cooldown = 0;
    a->attackPhase = 0;
    inventory_remove(a, BOLTS, 1);
    // Test ammo fallback against a valid standing target, independently of
    // whether the preceding shot stunned it under the current armor balance.
    e->stun = e->unconscious = 0;
    strike(0, enemy);
    CHECK(a->gear[0] == NONE && inventory_count(a, CROSSBOW) == 1,
          "empty crossbow is retained and switches to unarmed");
    fresh();
    a = &g.actors[0];
    int bc = g.buildingCount;
    for(int i=1;i<g.actorCount;i++)g.actors[i].active=0; /* Isolate construction from random encounters. */
    for (int k = 0; k < POP_GROUPS; k++) {
        g.population.groups[k].x = 1140 + k%10*14;
        g.population.groups[k].y = 1140 + k/10*14;
        g.life.groups[k].goal = LIFE_SETTLED;
    }
    CHECK(!build_structure(0) && g.buildingCount == bc, "town boundary prevents unauthorized construction");
    a->x = 42;
    a->y = 43;
    CHECK(build_structure(0) && g.baseExists && g.buildingCount == bc + 1,
          "camp construction consumes materials and establishes an outpost");
    CHECK(inventory_count(a, WOOD) == 5, "campfire consumes exactly three timber");
    CHECK(!build_structure(0), "construction prevents overlapping structures");
    inventory_add(a, MEAT, 2);
    inventory_add(a, WATER, 1);
    a->x += .5f; /* Walk into workstation reach. */
    finish_build();
    CHECK(craft_item(1) && inventory_count(a, MEAT) == 0, "campfire recipe supports meat substitution");
    a->x = 60;
    a->y = 40;
    CHECK(!craft_item(0), "crafting requires a nearby workstation");
    inventory_add(a, WOOD, 50);
    inventory_add(a, IRONBAR, 30);
    inventory_add(a, ORE, 6);
    CHECK(build_structure(3), "forge can be constructed from timber and iron bars");
    a->x += .5f;
    finish_build();
    int forge = g.buildingCount - 1;
    CHECK(!craft_item(0), "forge requires entry");
    fixture_door(a, forge);
    CHECK(interior_enter(forge), "use the forge door");
    fixture_service(ROOM_FORGE);
    CHECK(craft_item(0) && inventory_count(a, ORE) == 3, "forge smelts three ore into one iron bar");
    interior_leave();
    a->x = 66;
    a->y = 40;
    CHECK(build_structure(4), "research bench can be built");
    inventory_add(a, RELIC, 3);
    g.money = 2000;
    a->x += .5f;
    finish_build();
    CHECK(research_tech() && g.tech == 1 && inventory_count(a, RELIC) == 2,
          "research consumes relic and money and unlocks a tier");
    fixture_door(a, forge);
    CHECK(interior_enter(forge), "return to the forge");
    fixture_service(ROOM_FORGE);
    CHECK(craft_item(3) && inventory_count(a, SABRE) == 1, "research unlocks sabre crafting");
    fresh();
    a = &g.actors[0];
    int seam = nearest_ore(towns[0].x, towns[0].y, 150, NODE_ORE);
    CHECK(seam >= 0, "an iron seam remains outside the settlement");
    if (seam < 0) return 1;
    a->x = g.nodes[seam].x;
    a->y = g.nodes[seam].y;
    for (int i = 1; i < g.actorCount; i++) g.actors[i].active = 0;
    find_context();
    CHECK(ui.contextKind == 1, "resource context finds nearby iron deposit");
    interact();
    int ore = inventory_count(a, ORE);
    for (int i = 0; i < 35; i++)
        simulate(.1f);
    CHECK(inventory_count(a, ORE) > ore && a->xp[LABOUR] > 48,
          "gathering completes, consumes node stock, trains labour");
    a->taskKind = 0;
    int node = -1;
    for (int i = 0; i < g.nodeCount; i++)
        if (g.nodes[i].type == NODE_RUIN) {
            node = i;
            break;
        }
    for (int i = 1; i < g.actorCount; i++)
        g.actors[i].active = 0;
    a->autoattack = 0;
    a->x = g.nodes[node].x;
    a->y = g.nodes[node].y;
    find_context();
    interact();
    for (int i = 0; i < 10 && g.nodes[node].locked; i++) {
        for (int j = 0; j < 50; j++)
            simulate(.1f);
        interact();
    }
    CHECK(!g.nodes[node].locked && a->xp[LOCKPICK] > 48,
          "lock tools can open sealed ruins and train lockpicking");
    fresh();
    a = &g.actors[0];
    press(KEY_TRIANGLE);
    CHECK(ui.screen == SCREEN_PACK, "triangle opens inventory");
    press(KEY_R);
    CHECK(ui.screen == SCREEN_PACK, "R preserves the character screen while switching members");
    press(KEY_CIRCLE);
    press(KEY_DOWN);
    CHECK(ui.screen == SCREEN_SKILLS, "world D-pad Down opens skills");
    press(KEY_CIRCLE);
    CHECK(ui.screen == SCREEN_WORLD, "circle closes an overlay");
    float time = g.time;
    press(KEY_R);
    game_tick(.1f, (Input){0, 0, 0, 0});
    CHECK(ui.paused && g.time == time, "tactical pause stops simulation");
    press(KEY_R);
    press(KEY_START);
    time = g.time;
    game_tick(.1f, (Input){0, 0, 0, 0});
    CHECK(g.time == time, "menus stop simulation");
    press(KEY_CIRCLE);
    press(KEY_SELECT);
    CHECK(ui.screen == SCREEN_MAP, "select opens world map");
    press(KEY_TRIANGLE); /* Free cursor travel is in local view. */
    ui.mapX = 40;
    ui.mapY = 40;
    press(KEY_CROSS);
    CHECK(ui.screen == SCREEN_WORLD && ui.waypoint, "map places a manual navigation pin");
    r = find_kind(RECRUIT);
    ui.screen = SCREEN_RECRUIT;
    ui.selection = r;
    press(KEY_DOWN);
    CHECK(ui.selection == r, "dialogue directional input preserves recruit identity");
    fresh();
    r = find_kind(RECRUIT);
    recruit_actor(r);
    g.actors[0].x = 31.8f;
    g.actors[0].y = 29;
    g.actors[r].x = 31.7f;
    g.actors[r].y = 29;
    find_context();
    CHECK(ui.contextKind == 1 && g.nodes[ui.contextIndex].item == LEATHER,
          "healthy followers do not intercept nearby loot");
    g.actors[r].health[HEAD] = 50;
    find_context();
    CHECK(ui.contextKind == 0 && ui.contextIndex == r, "injured followers remain available for first aid");
    fresh();
    g.money = 1234;
    g.day = 4;
    g.actors[0].gear[1] = PLATE;
    g.actors[0].xp[SMITHING] = 234;
    CHECK(save_game("build/roundtrip.sav"), "save creates a complete expedition file");
    g.money = 9;
    CHECK(load_game("build/roundtrip.sav") && g.money == 1234 && g.day == 4 && g.actors[0].gear[1] == PLATE &&
              g.actors[0].xp[SMITHING] == 234,
          "save/load preserves economy, time, armor and skill progress");
    FILE *f = fopen("build/roundtrip.sav", "r+b");
    fseek(f, 100, SEEK_SET);
    fputc(77, f);
    fclose(f);
    g.money = 999;
    CHECK(!load_game("build/roundtrip.sav") && g.money == 999,
          "corrupted save is rejected without altering live state");
    CHECK(!load_game("build/no-such-save.sav"), "missing save is handled safely");
    fresh();
    int invalid = 0;
    for (int i = 0; i < 20000; i++) {
        simulate(.05f);
        for (int j = 0; j < g.actorCount; j++) {
            Actor *b = &g.actors[j];
            if (!isfinite(b->x) || !isfinite(b->y) || !isfinite(b->blood) || b->x < 0 || b->x > WORLD ||
                b->y < 0 || b->y > WORLD)
                invalid++;
        }
    }
    CHECK(invalid == 0, "1000-second whole-world simulation keeps actors finite and in bounds");
    CHECK(g.dayTime > 8 && g.time > 970, "long simulation advances world time");
    printf("\n%d/%d passed\n", total - failed, total);
    return failed ? 1 : 0;
}
