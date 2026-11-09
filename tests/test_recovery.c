#include "frontier_fixture.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

extern void init_actor(int, const char *, int, int, float, float, int);
extern int add_build(float, float, int, int, int);
extern int conscious(const Actor *);
extern void tick_expansion(float);

static Actor *fresh(void) {
    game_new(73819, 0);
    Actor *a = &g.actors[0];
    a->x = a->y = 400;
    a->autoattack = 0;
    for (int n = 1; n < g.actorCount; n++)
        g.actors[n].active = 0;
    for (int k = 0; k < POP_GROUPS; k++) {
        WorldEvent *e = &g.population.groups[k];
        e->x = 1200 + k % 8 * 12;
        e->y = 1200 + k / 8 * 12;
        g.life.groups[k].goal = LIFE_SETTLED;
    }
    g.roadsideAid.day = g.day;
    g.eventTimer = g.weatherTimer = 1000;
    ui.feedCount = ui.logCount = 0;
    return a;
}
static void press(int key) { game_tick(.033f, (Input){.held = key, .pressed = key}); }
static void step(float seconds) {
    for (int i = 0; i < (int)(seconds * 20); i++)
        game_tick(.05f, (Input){0});
}
static unsigned roll_seed(int modulus, int result) {
    for (unsigned seed = 1; seed < 10000; seed++) {
        unsigned x = seed;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        if ((int)(x % modulus) == result)
            return seed;
    }
    assert(0);
    return 1;
}
static void bleeding_cut(Actor *a, float hp, float blood, float bleed) {
    a->health[CHEST] = hp;
    a->blood = blood;
    a->bleed = bleed;
    a->woundFlags |= 1u << CHEST;
    g.depth.actors[a - g.actors].trauma[CHEST] = 16;
}

static void lay_in_bed(Actor *a) {
    int bed = add_build(a->x, a->y, BED, FREE, 0);
    assert(bed >= 0);
    a->taskKind = 3;
    a->taskIndex = bed;
    a->action = REST_DURATION;
}
static void injuries(void) {
    Actor *a = fresh();
    float weak = coma_threshold(a);
    a->xp[TOUGHNESS] = 12 * 69 * 69;
    assert(coma_threshold(a) < weak - 30);
    a->health[CHEST] = -7;
    injury_knockout(a, 20);
    assert(a->unconscious > 0 && !recovery_coma(a));
    step(17);
    assert(conscious(a) && a->health[CHEST] == -7);
    a = fresh();
    a->health[CHEST] = -180;
    injury_knockout(a, 160);
    step(19);
    assert(!a->dead && recovery_coma(a));
    dress_wounds(a);
    lay_in_bed(a);
    step(90);
    assert(conscious(a));
    a = fresh();
    a->health[HEAD] = -90;
    simulate(.05f);
    assert(a->dead);
    a = fresh();
    bleeding_cut(a, 50, .1f, 8);
    simulate(.1f);
    assert(a->dead);
    a = fresh();
    bleeding_cut(a, -19, 90, 1.3f);
    injury_knockout(a, 36);
    step(60);
    assert(recovery_coma(a) && a->bleed == 0 && (a->woundFlags & WOUND_CUTS));
    dress_wounds(a);
    lay_in_bed(a);
    step(50);
    assert(conscious(a) && !(a->woundFlags & WOUND_CUTS));
    a = fresh();
    a->race = 2;
    a->health[CHEST] = -21;
    injury_knockout(a, 34);
    step(28);
    assert(recovery_coma(a) && a->health[CHEST] == -21);
    injury_repair(a, &g.depth.actors[0]);
    lay_in_bed(a);
    step(30);
    assert(conscious(a) && a->health[CHEST] > 0);
    puts("PASS: Toughness changes coma threshold; ordinary knockouts wake; severe chest trauma can recover; head trauma and bleeding can kill.");
}

static void timed_treatments(void) {
    Actor *a = fresh();
    a->health[LARM] = 40;
    bleeding_cut(a, 45, 70, 1.5f);
    assert(save_game("build/v174/field-cut.sav"));
    step(3);
    assert(a->health[LARM] == 40 && a->health[CHEST] < 45 && a->blood < 70);
    float hp = a->health[CHEST], blood = a->blood;
    int kits = fixture_dressings(a);
    assert(heal_actor(0, 0));
    assert(a->taskKind == TASK_TREAT && a->health[CHEST] == hp && a->blood == blood);
    step(1);
    assert(fixture_dressings(a) == kits && a->bleed > 0);
    assert(save_game("build/v174/treating.sav") && load_game("build/v174/treating.sav"));
    a = &g.actors[0];
    step(a->action + .15f);
    assert(a->bleed == 0 && a->taskKind == TASK_TREAT && (a->woundFlags & WOUND_CUTS));
    assert(fixture_dressings(a) < kits && fixture_dressings(a) > kits - 100);
    for (int phase = 0; phase < LIMBS && a->taskKind == TASK_TREAT; phase++)
        step(a->action + .15f);
    assert(!(a->woundFlags & WOUND_CUTS) && (a->treatedMask & (1 << LARM)));
    assert(a->health[CHEST] < 48);
    hp = a->health[CHEST];
    step(8);
    assert(a->health[CHEST] > hp + .3f && a->health[CHEST] < hp + 1);
    a = fresh();
    bleeding_cut(a, 48, 75, 1.5f);
    kits = fixture_dressings(a);
    assert(heal_actor(0, 0));
    game_tick(.1f, (Input){.ax = .8f});
    assert(!a->taskKind && fixture_dressings(a) == kits && a->bleed > 0);

    a = fresh();
    bleeding_cut(a, 49, 70, .8f);
    assert(begin_treatment(0, 0, TREAT_PRESSURE, -1));
    step(1);
    assert(a->bleed > 0);
    step(2);
    assert(a->bleed == 0 && a->health[CHEST] < 51);
    inventory_add(a, TONIC, 1);
    int slot = 0;
    while (a->pack[slot].item != TONIC)
        slot++;
    blood = a->blood;
    use_item(slot);
    assert(a->blood == blood && a->woundFlags & WOUND_TONIC);
    step(3);
    assert(a->blood > blood + 2 && a->blood < blood + 5);

    a = fresh();
    a->race = 2;
    a->health[LARM] = 45;
    inventory_add(a, PARTS, 1);
    assert(depth_repair(0, 0) && a->health[LARM] == 45);
    step(6);
    assert(a->health[LARM] > 45 && a->health[LARM] < 50 && a->woundFlags & WOUND_REPAIRS);
    step(22);
    assert(a->health[LARM] > 48 && a->health[LARM] < 52);

    a = fresh();
    a->x = towns[0].x;
    a->y = towns[0].y + 2;
    bleeding_cut(a, 38, 76, .8f);
    int money = g.money;
    assert(doctor_heal(0) && a->health[CHEST] == 38 && g.money == money);
    step(a->action + .2f);
    assert(a->bleed == 0 && a->health[CHEST] < 43 && g.money == money - 80);
    step(55);
    assert(a->health[CHEST] >= 95);
    puts("PASS: cuts worsen and untreated wounds retain damage; field dressings, repairs and clinic care are timed; movement cancels treatment and saves retain it.");
}

static void custody(void) {
    Actor *a = fresh();
    const Building *jail = &g.buildings[g.field.jail[0] - 1];
    a->x = jail->x + 1.45f;
    a->y = jail->y + 2.5f;
    init_actor(1, "Jail guard", GUARD, g.townOwner[0], a->x, a->y - .6f, 14);
    g.actors[1].homeTown = 0;
    crime_add(a, g.townOwner[0], 100);
    bleeding_cut(a, -30, 8, 18);
    injury_knockout(a, 42);
    simulate(.05f);
    assert(a->carriedBy == 1 && a->taskKind == TASK_JAIL_CARE && !a->prisonTime);
    step(4);
    assert(a->prisonTown == 0 && actor_inside(a) == g.field.jail[0] &&
           a->taskKind == TASK_JAIL_CARE && a->health[CHEST] < 0 && a->bleed > 0);
    assert(!hostile(&g.actors[1], a));
    assert(save_game("build/v174/jail-bleeding.sav"));
    g.actors[1].active = 0;
    assert(!jail_pick_start(0));
    step(a->action + .2f);
    assert(a->bleed == 0 && a->prisonTime > 0 && a->health[CHEST] < 0 && !a->dead);
    step(45);
    assert(conscious(a) && a->prisonTime > 0 && a->blood > 25);

    a->xp[LOCKPICK] = 117612;
    assert(save_game("build/v174/jail-awake.sav"));
    press(KEY_CROSS);
    assert(ui.screen == SCREEN_DIALOG);
    press(KEY_DOWN);
    press(KEY_CROSS);
    assert(ui.screen == SCREEN_WORLD && a->taskKind == TASK_CELL_LOCK && a->prisonTime > 0);
    assert(save_game("build/v174/jail-picking.sav") && load_game("build/v174/jail-picking.sav"));
    a = &g.actors[0];
    step(1);
    assert(a->taskKind == TASK_CELL_LOCK && a->prisonTime > 0);
    g.rng = roll_seed(100, 0);
    tick_expansion(a->action + .01f);
    assert(jail_cell_open(a) && wanted_amount(a, g.townOwner[0]) == 100);
    g.interiorX = 3.5f;
    g.interiorY = 6.4f;
    press(KEY_CROSS);
    assert(!a->prisonTime && !actor_inside(a));
    float x = a->x;
    move_actor(a, 0, 1, .2f);
    assert(a->x == x && conscious(a));

    a = fresh();
    a->prisonTime = 180;
    a->prisonTown = 0;
    inventory_remove(a, TOOLS, inventory_count(a, TOOLS));
    assert(jail_pick_start(0) && !a->taskIndex && a->action > 10);
    g.rng = roll_seed(100, 99);
    tick_expansion(a->action + .01f);
    assert(a->prisonTime < 180 && wanted_amount(a, g.townOwner[0]) == 0);
    assert(jail_pick_start(0));
    g.rng = roll_seed(100, 0);
    tick_expansion(a->action + .01f);
    assert(!a->prisonTime && wanted_amount(a, g.townOwner[0]) == SERIOUS_BOUNTY);
    puts("PASS: jail intake always begins gradual care, including when the guard leaves. Timed lockpicking supports tools, wire, save/load and failed attempts.");
}

static void agility(void) {
    Actor *a = fresh();
    float initial = movement_speed(a);
    a->xp[ATHLETICS] = 12 * 39 * 39;
    assert(movement_speed(a) > initial * 1.6f);
    init_actor(1, "Pursuer", RAIDER, HOSTILE, a->x - 3.5f, a->y, 5);
    g.actors[1].gear[0] = NONE;
    float before = distance(a->x, a->y, g.actors[1].x, g.actors[1].y);
    for (int i = 0; i < 100; i++)
        game_tick(.05f, (Input){.ax = .7f, .ay = .7f});
    assert(distance(a->x, a->y, g.actors[1].x, g.actors[1].y) > before + 4);
    puts("PASS: a trained runner opens a gap against an active pursuer.");
}

static void strangers(void) {
    int skipped = 0, treated = 0, carried = 0, bedded = 0;
    for (unsigned seed = 1; seed < 80 && !(skipped && treated && bedded); seed++) {
        Actor *a = fresh();
        g.roadsideAid.day = 0;
        g.rng = seed;
        bleeding_cut(a, -26, 42, 3.2f);
        injury_knockout(a, 45);
        int bed = add_build(405, 400, BED, FREE, 0);
        assert(bed >= 0 && can_walk(405, 400));
        g.depth.buildings[bed].labor = 0;
        roadside_aid_tick(.05f);
        if (!g.roadsideAid.actor) {
            skipped++;
            g.rng = roll_seed(5, 0);
            roadside_aid_tick(.05f);
            assert(!g.roadsideAid.actor);
            continue;
        }
        int m = g.roadsideAid.actor - 1;
        assert(distance(a->x, a->y, g.actors[m].x, g.actors[m].y) > 14);
        if (!treated) assert(save_game("build/v174/roadside.sav"));
        for (int i = 0; i < 620 && !a->dead && a->taskKind != 3; i++) {
            step(.05f);
            if (a->carriedBy == m && !carried) {
                carried = 1;
                assert(save_game("build/v174/roadside-carry.sav"));
            }
        }
        treated += a->bleed == 0 && !a->dead;
        if (a->taskKind == 3) {
            assert(a->taskIndex == bed && a->carriedBy == -1 && distance(a->x, a->y, 405, 400) < .1f);
            bedded++;
        }
    }
    assert(skipped && treated && carried && bedded);
    puts("PASS: rescue rolls are rare and non-repeating; a traveler physically approaches, treats and carries the patient to an available bed.");
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    injuries();
    timed_treatments();
    custody();
    agility();
    strangers();
    puts("ALL RECOVERY CHECKS PASSED");
    return 0;
}
