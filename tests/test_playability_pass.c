#include "../src/render.c"
#include <assert.h>
extern void init_actor(int n, const char *name, int kind, int faction, float x, float y, int skill);

static void press(int key) {
    game_tick(.033f, (Input){.held = key, .pressed = key});
}
static void idle(float seconds) {
    for (int i = 0; i < (int)(seconds * 20); i++)
        game_tick(.05f, (Input){0});
}
static void setup(void) {
    game_new(73819, 1);
    for (int n = 0; n < g.actorCount; n++) {
        Actor *a = &g.actors[n];
        a->active = n < 3;
        a->autoattack = 0;
        a->target = -1;
        a->job = JOB_HOLD;
        if (n < 3) {
            a->x = 400 + n;
            a->y = 400;
        }
    }
    for (int k = 0; k < POP_GROUPS; k++) {
        WorldEvent *e = &g.population.groups[k];
        e->x = 1200 + k % 8 * 12;
        e->y = 1200 + k / 8 * 12;
        g.life.groups[k].goal = LIFE_SETTLED;
    }
    ui.feedCount = ui.logCount = 0;
}
static int action_row(int type, int kind) {
    int count = nearby_actions();
    for (int i = 0; i < count; i++)
        if (nearby_action(i)->type == type && nearby_action(i)->kind == kind)
            return i;
    return -1;
}
static void squad_control(void) {
    setup();
    Actor *ash = &g.actors[0], *reed = &g.actors[1], *vale = &g.actors[2];
    assert(can_walk(400, 400) && can_walk(410, 400));
    ash->job = vale->job = JOB_FOLLOW;
    reed->x = 410;
    g.depth.actors[2].group = 2;
    g.depth.groupLeader[2] = 2;
    press(KEY_L);
    idle(.1f);
    assert(g.selected == 1);
    idle(2);
    assert(ash->x > 401 && vale->x > 403 && reed->x == 410);
    press(KEY_RIGHT);
    assert(ui.screen == SCREEN_SQUAD);
    press(KEY_TRIANGLE);
    assert(ui.squadPage == 1);
    press(KEY_DOWN);
    press(KEY_RIGHT);
    assert(g.depth.actors[1].queue[0] == JOB_MINE);

    setup();
    ash->job = JOB_MEDIC;
    reed->x = 407;
    reed->health[LARM] = 32;
    reed->bleed = 1;
    int kit = inventory_count(ash, MEDKIT);
    idle(4.9f);
    assert(ash->x == 400 && inventory_count(ash, MEDKIT) == kit);
    game_tick(.1f, (Input){.ax = -.8f});
    float moved = ash->x;
    assert(moved < 400);
    idle(4.9f);
    assert(ash->x == moved && inventory_count(ash, MEDKIT) == kit);
    idle(.5f);
    assert(ash->x > moved && strstr(ui.workNotice, "REED"));
    press(KEY_SQUARE);
    moved = ash->x;
    idle(6);
    assert(ash->x == moved && ash->autoattack && inventory_count(ash, MEDKIT) == kit);
    press(KEY_SQUARE);
    idle(20);
    assert(reed->bleed == 0 && inventory_count(ash, MEDKIT) < kit);
    puts("PASS: Follow changes lead across formations; priorities tab; selected medic waits for idle and passive stance.");
}
static void transfers_and_recovery(void) {
    setup();
    Actor *ash = &g.actors[0], *reed = &g.actors[1], *vale = &g.actors[2];
    int reedKits = inventory_count(reed, MEDKIT), valeKits = inventory_count(vale, MEDKIT);
    press(KEY_TRIANGLE);
    int slot = 0;
    while (ash->pack[slot].item != MEDKIT) slot++;
    ui.selection = slot;
    press(KEY_SQUARE);
    assert(ui.screen == SCREEN_PASS);
    press(KEY_DOWN);
    assert(pass_recipient(ui.selection) == 2);
    vale->x = 410;
    press(KEY_CROSS);
    assert(ui.screen == SCREEN_PASS && inventory_count(vale, MEDKIT) == valeKits);
    vale->x = 402;
    press(KEY_CROSS);
    assert(ui.screen == SCREEN_PACK && ui.selection == slot);
    assert(inventory_count(vale, MEDKIT) == valeKits + 1 && inventory_count(reed, MEDKIT) == reedKits);
    press(KEY_CIRCLE);
    ash->bleed = 2;
    ash->health[LARM] = 25;
    g.depth.actors[0].trauma[LARM] = 20;
    assert(save_game("build/v172/bleeding.sav"));
    press(KEY_CROSS);
    assert(ui.screen == SCREEN_ACTIONS);
    int row = action_row(ACTION_SELF_AID, 0);
    assert(row >= 0 && !nearby_action_reason(row));
    ui.selection = row;
    press(KEY_CROSS);
    assert(ui.screen == SCREEN_WORLD && ash->taskKind == TASK_TREAT && ash->bleed > 0);
    idle(ash->action + .2f);
    assert(ash->bleed == 0 && ash->treatedMask & (1 << LARM));
    float hp = ash->health[LARM], trauma = g.depth.actors[0].trauma[LARM];
    idle(20);
    assert(ash->health[LARM] > hp + 1 && g.depth.actors[0].trauma[LARM] < trauma);
    assert(save_game("build/v172/recovery.sav"));
    assert(load_game("build/v172/recovery.sav") && g.actors[0].treatedMask & (1 << LARM));
    puts("PASS: unavailable recipients cannot take items; handoff to the requested ally; self-aid and treated recovery persist.");
}
static void camps_and_theft(void) {
    setup();
    Actor *ash = &g.actors[0];
    population_generate(0, POP_SCAVENGERS);
    WorldEvent *e = &g.population.groups[0];
    PopulationState *m = &g.population.state[0];
    e->x = m->siteX = ash->x;
    e->y = m->siteY = ash->y;
    m->offsetX[0] = m->offsetY[0] = 0;
    g.life.groups[0].camp = 0;
    int row = action_row(ACTION_POPULATION, 1);
    assert(row >= 0 && strstr(nearby_action_reason(row), "Need 2 meat"));
    assert(!e->spawned);
    nearby_action_choose(row);
    assert(!e->spawned && !inventory_count(ash, MEAT));
    inventory_add(ash, MEAT, 2);
    assert(!nearby_action_reason(row));
    int rations = inventory_count(ash, RATION);
    nearby_action_choose(row);
    assert(inventory_count(ash, MEAT) == 0 && inventory_count(ash, RATION) == rations + 1);

    setup();
    init_actor(3, "Camp scavenger", SETTLER, SCAVENGERS, 401, 400, 6);
    g.nodeCount = 1;
    g.nodes[0] = (Node){.type = NODE_LOOT, .x = 400, .y = 400, .owner = SCAVENGERS,
                         .remaining = 1, .item = CLOTH, .qty = 1};
    row = action_row(ACTION_CONTEXT, 1);
    char label[100];
    nearby_action_label(row, label, sizeof(label));
    assert(strstr(label, "STEAL") && strstr(label, factionNames[SCAVENGERS]));
    nearby_action_choose(row);
    assert(g.theftWarnings & (1 << SCAVENGERS));
    assert(wanted_amount(ash, SCAVENGERS) == 0 && strstr(ui.notice, "Hands off"));
    g.nodes[0].remaining = 1;
    nearby_action_choose(action_row(ACTION_CONTEXT, 1));
    assert(wanted_amount(ash, SCAVENGERS) == 80 && strstr(ui.logs[0], factionNames[SCAVENGERS]));
    puts("PASS: camp cook availability has no side effects; ownership is visible; first scavenger warning and repeat bounty.");
}
static void retreat_and_news(void) {
    setup();
    Actor *ash = &g.actors[0], *reed = &g.actors[1];
    init_actor(3, "Scavenger", SETTLER, HOSTILE, 404, 400, 6);
    Actor *npc = &g.actors[3];
    idle(8);
    assert(npc->x > 412);
    float x = npc->x, y = npc->y;
    idle(5);
    assert(distance(x, y, npc->x, npc->y) < 1);
    assert(ash->x == 400);
    ui.feedCount = ui.logCount = 0;
    gain(reed, MEDICINE, 400);
    reed->bleed = 2;
    log_tick(.03f);
    assert(strstr(ui.feed[0], "REED is bleeding") && ui.feedPriority[0] > ui.feedPriority[1]);
    char levelup[80];
    snprintf(levelup, sizeof(levelup), "REED reached %s", skillNames[MEDICINE]);
    assert(strstr(ui.feed[1], levelup));
    int priority, offset;
    log_tick(.9f);
    log_current(&priority, &offset);
    assert(offset > 0);
    puts("PASS: fleeing scavenger holds a safe position; urgent squad warning queues ahead of ally level-up and scrolls.");
}
static void scenarios_and_atlas(void) {
    for (int i = 0; i < START_SCENARIOS; i++) {
        game_new(73819, 0);
        ui.screen = SCREEN_TITLE;
        press(KEY_CROSS);
        assert(ui.screen == SCREEN_SCENARIOS);
        for (int n = 0; n < i; n++) press(KEY_DOWN);
        press(KEY_CROSS);
        assert(ui.screen == SCREEN_CREATE && ui.createOrigin == i);
        press(KEY_UP);
        press(KEY_CROSS);
        assert(ui.screen == SCREEN_WORLD && g.playOrigin == i);
        Actor *a = &g.actors[g.selected];
        int town = startScenarios[i].town;
        assert(can_walk(a->x, a->y) && g.discovered[town]);
        assert(distance(a->x, a->y, towns[town].x, towns[town].y) < 18);
        assert(a->x > 150 && a->y > 150 && a->x < WORLD - 150 && a->y < WORLD - 150);
        assert(g.squadCount == (i == 4 ? 3 : 1));
    }
    MapView view = {400, 400, 145, 120, 1, .5f};
    float x, y;
    map_sample(view, 164, 100, &x, &y);
    assert(map_x(view, x, y) == 164 && map_y(view, x, y) == 100);
    assert(map_x(view, 400, 380) > view.x && map_y(view, 400, 380) < view.y);
    press(KEY_SELECT);
    press(KEY_TRIANGLE);
    float u = view_horizontal(ui.mapX, ui.mapY), v = view_vertical(ui.mapX, ui.mapY);
    game_tick(.1f, (Input){.held = KEY_RIGHT | KEY_UP});
    assert(view_horizontal(ui.mapX, ui.mapY) > u && view_vertical(ui.mapX, ui.mapY) < v);
    puts("PASS: every scenario is walkable and inland; world and map cursor use the same isometric axes.");
}
int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    squad_control();
    transfers_and_recovery();
    camps_and_theft();
    retreat_and_news();
    scenarios_and_atlas();
    return 0;
}
