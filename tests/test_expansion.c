#include "game.h"
#include "combat_pose.h"
#include "legacy.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int total, failed;
#define CHECK(c, m)                                                                                          \
    do {                                                                                                     \
        total++;                                                                                             \
        if (!(c)) {                                                                                          \
            failed++;                                                                                        \
            printf("FAIL %02d: %s [%d]\n", total, m, __LINE__);                                              \
        } else                                                                                               \
            printf("PASS %02d: %s\n", total, m);                                                             \
    } while (0)
static void fresh(void) {
    game_new(73819, 0);
    for (int i = 1; i < g.actorCount; i++)
        g.actors[i].active = 0;
}
static void step(float seconds) {
    for (int i = 0; i < (int)(seconds * 100); i++)
        simulate(.01f);
}
static int slot(Actor *a, int it) {
    for (int i = 0; i < PACK; i++)
        if (a->pack[i].item == it)
            return i;
    return -1;
}
static void press(int k) { game_tick(.033f, (Input){k, k, 0, 0}); }
static uint32_t hashbytes(const void *p, size_t n) {
    uint32_t h = 2166136261u;
    const unsigned char *b = p;
    for (size_t i = 0; i < n; i++)
        h = (h ^ b[i]) * 16777619u;
    return h;
}
int main(void) {
    fresh();
    Actor *a = &g.actors[0];
    int valid = 0;
    for (int it = BASE_ITEMS; it < ITEMS; it++) {
        ItemDef *d = &itemDefs[it];
        valid += d->name && d->name[0] && d->slot >= 0 && d->slot < GEARS && d->price > 0 && d->weight > 0 &&
                 d->effect >= 0 && d->effect < 12;
    }
    CHECK(valid == FAMILIES * MATERIALS * QUALITIES,
          "all 2,112 gear variants have valid slots, effects and prices");
    CHECK(gear_id(FAMILIES - 1, 5, 3) == ITEMS - 1 && gear_id(-1, 0, 0) == NONE,
          "catalog boundaries are checked");
    character_begin(1);
    strcpy(ui.createName, "REN");
    ui.createSkin = 5;
    ui.createHair = 3;
    ui.createRace = 2;
    character_finish();
    a = &g.actors[0];
    CHECK(!strcmp(a->name, "REN") && a->origin == 1 && a->skin == 5 && a->hair == 3 && a->race == 2 &&
              g.money == 420,
          "creator applies appearance, ancestry and ronin origin");
    int oldmoney = g.money;
    character_begin(-1);
    ui.createSkin = 2;
    character_finish();
    CHECK(g.money == oldmoney && a->skin == 2, "barber preserves expedition progress");
    ui.screen = SCREEN_FORGE;
    press(KEY_SELECT);
    CHECK(ui.screen == SCREEN_HELP && ui.selection == 8, "context guide opens the forging chapter");
    press(KEY_CIRCLE);
    CHECK(ui.screen == SCREEN_FORGE, "guide returns to originating screen");
    fresh();
    a = &g.actors[0];
    a->x = 34;
    a->y = 32;
    g.tech = 3;
    a->xp[SMITHING] = 12 * 40 * 40;
    inventory_add(a, IRONBAR, 80);
    inventory_add(a, CLOTH, 40);
    inventory_add(a, WOOD, 30);
    inventory_add(a, COAL, 10);
    inventory_add(a, RELIC, 5);
    int piece = gear_id(44, 5, 3), bars = inventory_count(a, IRONBAR);
    CHECK(forge_equipment(piece) && inventory_count(a, piece) == 1 &&
              inventory_count(a, IRONBAR) == bars - 13,
          "master relic armor consumes exact forging ingredients");
    use_item(slot(a, piece));
    CHECK(a->gear[1] == piece, "forged armor equips into its visible body slot");
    Actor snapshot = *a;
    g.tech = 0;
    CHECK(!forge_equipment(piece) && !memcmp(a, &snapshot, sizeof(*a)), "locked forging is atomic");
    g.tech = 3;
    a->x = 42;
    a->y = 44;
    int before = g.buildingCount;
    CHECK(build_structure(6) && g.buildingCount == before + 1,
          "depot construction succeeds with materials outside town");
    int depot = g.buildingCount - 1, si = slot(a, ORE);
    if (si < 0) {
        inventory_add(a, ORE, 10);
        si = slot(a, ORE);
    }
    int ore = inventory_count(a, ORE);
    CHECK(storage_transfer(depot, 0, si) && g.buildings[depot].storage[0].item == ORE &&
              inventory_count(a, ORE) == ore - 1,
          "depot deposits inventory resources");
    CHECK(storage_transfer(depot, 1, 0) && inventory_count(a, ORE) == ore,
          "depot withdrawal restores inventory");
    a->stolen[si] = UNION;
    CHECK(!storage_transfer(depot, 0, si), "stolen goods cannot lose ownership through storage");
    a->stolen[si] = 0;
    fresh();
    a = &g.actors[0];
    inventory_add(a, ORE, 8);
    CHECK(accept_contract(0, 1) && !accept_contract(0, 1), "journal rejects a duplicate active contract");
    oldmoney = g.money;
    CHECK(complete_contract(0) && g.money == oldmoney + 240 && g.contractsDone == 1 && g.rep[UNION] == 8 &&
              inventory_count(a, ORE) == 0,
          "supply contract consumes goods and pays silver and reputation");
    CHECK(!complete_contract(0), "completed contract cannot pay twice");
    CHECK(accept_contract(0, 0), "delivery contract can be accepted");
    int cn = 0;
    while (g.contracts[cn].type != 0 || g.contracts[cn].active != 1)
        cn++;
    int dest = g.contracts[cn].target;
    a->x = towns[dest].x;
    a->y = towns[dest].y + 7;
    simulate(.1f);
    CHECK(g.contracts[cn].progress == 1, "delivery records reaching the destination");
    CHECK(!complete_contract(cn), "delivery requires returning to the issuer");
    a->x = 26;
    a->y = 30;
    CHECK(complete_contract(cn), "delivery pays on returning to the issuing town");
    g.prosperity[0] = 20;
    int high = item_price(0, RATION, 0);
    g.prosperity[0] = 90;
    CHECK(item_price(0, RATION, 0) < high, "local prosperity affects displayed market prices");
    ui.tradeTown = 0;
    ui.shopFilter = 2;
    int weapons = 1;
    for (int i = 0; i < shop_count(); i++)
        if (itemDefs[shop_item(i)].slot != 0)
            weapons = 0;
    CHECK(weapons && shop_count() > 0, "weapon market filter maps to available stock");
    fresh();
    a = &g.actors[0];
    int stolen = gear_id(34, 2, 1);
    inventory_add(a, stolen, 1);
    a->stolen[slot(a, stolen)] = UNION;
    use_item(slot(a, stolen));
    ui.screen = SCREEN_EQUIPPED;
    ui.selection = 1;
    press(KEY_CROSS);
    CHECK(a->stolen[slot(a, stolen)] == UNION, "stolen ownership survives equipping and removing gear");
    oldmoney = g.money;
    CHECK(!sell_item(0, slot(a, stolen)) && g.money == oldmoney, "selling to the victim faction is rejected");
    CHECK(sell_item(8, slot(a, stolen)), "Black Mesa can fence stolen gear");
    fresh();
    a = &g.actors[0];
    a->health[CHEST] = 30;
    a->bleed = 2;
    a->blood = 40;
    oldmoney = g.money;
    CHECK(doctor_heal(0) && a->health[CHEST] == 100 && a->bleed == 0 && g.money == oldmoney - 80,
          "doctor treats wounds for the stated fee");
    a->health[LARM] = 40;
    a->fatigue = 80;
    CHECK(start_sleep(0), "inn dialogue starts paid rest");
    step(2);
    CHECK(a->health[LARM] > 40 && a->fatigue < 80, "sleep restores wounds and fatigue");
    fresh();
    a = &g.actors[0];
    a->x = 300;
    a->y = 300;
    g.weather = 4;
    g.weatherTimer = 1000;
    step(10);
    float exposure = a->exposure;
    CHECK(exposure > 1, "snow exposure rises outside shelter");
    int sealed = NONE;
    for (int it = BASE_ITEMS; it < ITEMS; it++)
        if (itemDefs[it].slot == 1 && itemDefs[it].effect == 9 && itemDefs[it].material == 5)
            sealed = it;
    a->gear[1] = sealed;
    a->exposure = 0;
    step(10);
    CHECK(a->exposure < exposure, "sealed equipment reduces environmental exposure");
    fresh();
    a = &g.actors[0];
    int guard = 1;
    while (g.actors[guard].kind != GUARD)
        guard++;
    Actor *e = &g.actors[guard];
    e->active = 1;
    e->x = a->x + .5f;
    e->y = a->y;
    a->bounty = 100;
    a->unconscious = 30;
    simulate(.1f);
    CHECK(a->prisonTime > 0 && a->prisonTown == 0,
          "nearby law guards take defeated wanted survivors into custody");
    float x = a->x;
    move_actor(a, 1, 0, 1);
    CHECK(a->x == x, "custody prevents walking out of the holding area");
    dialogue_open(0);
    oldmoney = g.money;
    press(KEY_CROSS);
    CHECK(a->prisonTime == 0 && a->bounty == 0 && g.money == oldmoney - 100,
          "custody dialogue accepts bail and clears bounty");
    fresh();
    a = &g.actors[0];
    e = &g.actors[1];
    e->active = 1;
    e->kind = SETTLER;
    e->faction = HOSTILE;
    a->autoattack = 0;
    a->x = 40;
    a->y = 40;
    e->x = 41;
    e->y = 40;
    memset(e->gear, 0, sizeof(e->gear));
    a->gear[0] = gear_id(2, 3, 2);
    strike(0, 1);
    float sum = 0;
    for (int l = 0; l < LIMBS; l++)
        sum += e->health[l];
    CHECK(a->attackPhase == 1 && sum == 600, "starting an attack winds up without instant damage");
    float heavy = a->attackClock;
    a->attackPhase = 0;
    a->cooldown = 0;
    a->gear[0] = gear_id(3, 3, 2);
    strike(0, 1);
    CHECK(a->attackClock < heavy, "short blades wind up faster than heavy nodachi");
    e->x = 48;
    step(1);
    sum = 0;
    for (int l = 0; l < LIMBS; l++)
        sum += e->health[l];
    CHECK(sum == 600, "leaving reach before contact avoids the strike");
    e->x = 41;
    e->tx = 41;
    e->ty = 40;
    e->homeX = 41;
    e->homeY = 40;
    a->attackPhase = 0;
    a->cooldown = 0;
    a->lostMask = 1 << RARM;
    strike(0, 1);
    CHECK(a->attackWeapon == NONE, "a missing weapon arm forces unarmed attacks");
    fresh();
    a = &g.actors[0];
    e = &g.actors[1];
    a->x = 40;
    a->y = 40;
    a->autoattack = 0;
    e->x = e->homeX = e->tx = 41;
    e->y = e->homeY = e->ty = 40;
    e->active = 1;
    e->faction = HOSTILE;
    e->kind = SETTLER;
    e->autoattack = 0;
    a->attackPhase = 2;
    a->attackTarget = 1;
    a->attackWeapon = gear_id(0, 1, 1);
    a->attackClock = a->attackDuration = STRIKE_SECONDS;
    step(.08f);
    CHECK(e->hitFlash == 0, "no hit before the blade reaches the contact marker");
    step(.04f);
    CHECK(e->hitFlash > 0, "hit or block resolves at visible blade contact");
    float earned = a->xp[MELEE];
    step(.35f);
    CHECK(a->xp[MELEE] == earned, "follow-through cannot resolve the same strike twice");
    int seamless = 1;
    for (int f = 0; f < 32; f++)
        for (int phase = 1; phase < 4; phase++) {
            CombatPose end = combat_pose(gear_id(f, 1, 1), phase, 1);
            CombatPose next = combat_pose(gear_id(f, 1, 1), phase == 3 ? 0 : phase + 1, 0);
            if (fabsf(end.handX - next.handX) > .001f || fabsf(end.handY - next.handY) > .001f ||
                fabsf(end.blade - next.blade) > .001f || fabsf(end.step - next.step) > .001f ||
                fabsf(end.shift - next.shift) > .001f)
                seamless = 0;
        }
    CHECK(seamless, "all 32 weapon families join preparation, strike and recovery without pose snapping");
    fresh();
    a = &g.actors[0];
    a->gear[0] = gear_id(29, 4, 3);
    a->gear[5] = gear_id(83, 5, 3);
    a->prisonTime = 12;
    a->prisonTown = 0;
    a->skin = 7;
    g.weather = 4;
    g.contractsDone = 5;
    g.contracts[0] = (Contract){1, 0, 0, 3, 0, 1, 0, 220, 0, 0};
    g.townOwner[5] = OUTLAWS;
    CHECK(save_game("build/expanded.sav"), "expanded expedition writes successfully");
    fresh();
    CHECK(load_game("build/expanded.sav") && g.actors[0].gear[5] == gear_id(83, 5, 3) &&
              g.actors[0].prisonTime == 12 && g.actors[0].skin == 7 && g.weather == 4 &&
              g.contracts[0].target == 3 && g.townOwner[5] == OUTLAWS,
          "save roundtrip preserves appearance, gear, custody, weather, contracts and ownership");
    CHECK(sizeof(LegacyGame) == 47080, "legacy layout matches the shipped v0.1 save payload");
    CHECK(load_game("tests/fixtures/legacy-v01.dust") && g.baseExists && g.tech == 3 && g.actorCount > 100 &&
              g.actors[0].gear[1] == PLATE,
          "real v0.1 outpost fixture migrates equipment, buildings and research");
    CHECK(save_game("build/migrated.sav") && load_game("build/migrated.sav"),
          "migrated expedition can be saved and reloaded as v0.2");
    /* Corrupt a legacy field but recompute its checksum: structural rejection must remain atomic. */
    FILE *f = fopen("tests/fixtures/legacy-v01.dust", "rb");
    unsigned char header[20];
    LegacyGame *old = malloc(sizeof(*old));
    if (fread(header, 20, 1, f) != 1 || fread(old, sizeof(*old), 1, f) != 1)
        return 1;
    fclose(f);
    old->actors[0].gear[0] = 999999;
    uint32_t hash = hashbytes(old, sizeof(*old));
    memcpy(header + 16, &hash, 4);
    f = fopen("build/bad-legacy.sav", "wb");
    fwrite(header, 20, 1, f);
    fwrite(old, sizeof(*old), 1, f);
    fclose(f);
    free(old);
    uint32_t beforehash = hashbytes(&g, sizeof(g));
    CHECK(!load_game("build/bad-legacy.sav") && hashbytes(&g, sizeof(g)) == beforehash,
          "invalid legacy data cannot alter the live expedition");
    fresh();
    a = &g.actors[0];
    a->dead = 1;
    g.actors[1].squad = 1;
    g.actors[1].dead = 1;
    g.squadCount = 2;
    ui.screen = SCREEN_SQUAD;
    press(KEY_SQUARE);
    CHECK(g.selected == 1 && g.actors[g.selected].squad && g.squadCount == 1,
          "dismissing a fallen leader selects another roster member even when all are dead");
    ui.screen = SCREEN_SQUAD;
    press(KEY_SQUARE);
    CHECK(g.squadCount == 1 && save_game("build/fallen.sav") && load_game("build/fallen.sav"),
          "last fallen survivor remains a valid saveable expedition");
    fresh();
    g.day = 3;
    for (int t = 0; t < TOWNS; t++)
        g.prosperity[t] = 15;
    g.weatherTimer = 10000;
    for (int i = 0; i < 50; i++) {
        g.eventTimer = 0;
        simulate(.01f);
    }
    int changed = 0, flags = 1;
    for (int t = 1; t < TOWNS; t++)
        if (g.townOwner[t] != towns[t].faction) {
            changed++;
            for (int b = 0; b < g.buildingCount; b++)
                if (g.buildings[b].owner < 0 && town_near(g.buildings[b].x, g.buildings[b].y, 13) == t &&
                    g.buildings[b].faction != g.townOwner[t])
                    flags = 0;
        }
    CHECK(changed > 0 && flags, "town ownership events update settlement faction banners");
    printf("\n%d/%d expansion checks passed\n", total - failed, total);
    return failed ? 1 : 0;
}
