#include "game.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

int npc_role(const Actor *a) {
    if (a->kind == TRADER)
        return ROLE_MERCHANT;
    if (a->kind == RECRUIT)
        return ROLE_RECRUIT;
    if (a->voice >= ROLE_INNKEEPER && a->voice <= ROLE_SCOUT)
        return a->voice;
    if (a->kind == GUARD)
        return ROLE_MARSHAL;
    if (a->kind == CARAVAN)
        return ROLE_SCOUT;
    if (strstr(a->name, "Inn"))
        return ROLE_INNKEEPER;
    return ROLE_CITIZEN;
}
const char *npc_role_name(const Actor *a) {
    static const char *names[] = {"RESIDENT", "MERCHANT", "RECRUIT", "MARSHAL", "INNKEEPER",
                                  "MEDIC",    "SMITH",    "BARBER",  "SCOUT"};
    return names[npc_role(a)];
}
static const int services[][5] = {{6, 8, -1},       {0, 6, 8, -1}, {6, 8, -1},
                                  {1, 4, 6, 8, -1}, {2, 6, 8, -1}, {3, 6, 8, -1},
                                  {7, 6, 8, -1},    {5, 6, 8, -1}, {6, 8, -1}};
int dialogue_count(int actor) {
    if (actor < 0 || actor >= g.actorCount)
        return 0;
    int n = 0;
    while (n < 5 && services[npc_role(&g.actors[actor])][n] >= 0)
        n++;
    return n;
}
int dialogue_action(int actor, int row) {
    if (row < 0 || row >= dialogue_count(actor))
        return 8;
    return services[npc_role(&g.actors[actor])][row];
}
const char *dialogue_option(int actor, int row) {
    static const char *labels[] = {
        "Show me your stock.",        "Any paying work?",         "Rent a bed. 25 silver",
        "Treat my wounds. 80 silver", "Bounties and local news.", "Change my appearance.",
        "What have you heard?",       "Use the smithy.",          "Safe travels."};
    int action = dialogue_action(actor, row);
    if (action == 6 && npc_role(&g.actors[actor]) == ROLE_RECRUIT)
        return "Join my expedition. 300 silver";
    return labels[action];
}
float context_distance(void) {
    Actor *a = &g.actors[g.selected];
    int n = ui.contextIndex;
    if (n < 0)
        return 999;
    if (ui.contextKind == 0)
        return distance(a->x, a->y, g.actors[n].x, g.actors[n].y);
    if (ui.contextKind == 1)
        return distance(a->x, a->y, g.nodes[n].x, g.nodes[n].y);
    Building *b = &g.buildings[n];
    return distance(a->x, a->y, b->x + (ui.contextKind == 3 ? 1.45f : 0),
                    b->y + (ui.contextKind == 3 ? .65f : 0));
}
float context_reach(void) {
    return ui.contextKind == 1 ? WORK_REACH : ui.contextKind == 0 ? TALK_REACH : 1.7f;
}
const char *context_name(void) {
    int n = ui.contextIndex;
    if (n < 0)
        return "NO NEARBY TARGET";
    if (ui.contextKind == 0)
        return g.actors[n].name;
    if (ui.contextKind == 1) {
        static const char *s[] = {"IRON DEPOSIT",  "COPPER DEPOSIT", "DEADWOOD",
                                  "SALVAGE CHEST", "LOCKED CHEST",   "OLD RUINS"};
        return s[g.nodes[n].type];
    }
    static const char *s[] = {"FRONTIER HOME",  "TRADING POST", "INN",     "WATCH HOUSE", "WINDPUMP",
                              "WELL",           "CAMPFIRE",     "BEDROLL", "CROP FIELD",  "SMITHY",
                              "RESEARCH BENCH", "PALISADE",     "DEPOT"};
    return s[g.buildings[n].type];
}
const char *context_action(void) {
    int n = ui.contextIndex;
    if (n < 0)
        return "WALK TO A PERSON, DOOR OR RESOURCE";
    if (ui.contextKind == 3)
        return "ENTER";
    if (ui.contextKind == 0) {
        Actor *b = &g.actors[n];
        return b->squad                        ? "FIRST AID / CARRY"
               : b->dead || b->unconscious > 0 ? "LOOT"
               : g.actors[g.selected].sneaking ? "PICKPOCKET"
                                               : "TALK";
    }
    if (ui.contextKind == 1) {
        Node *b = &g.nodes[n];
        return b->locked                ? "PICK LOCK"
               : b->type <= NODE_COPPER ? "MINE"
               : b->type == NODE_WOOD   ? "GATHER"
                                        : "TAKE";
    }
    int t = g.buildings[n].type;
    return t == WELL                   ? "DRAW WATER"
           : t == FARM                 ? (g.buildings[n].progress >= 100 ? "HARVEST" : "WATER CROP")
           : t == BED || t == CAMPFIRE ? "REST"
           : t == FORGE                ? "CRAFT"
           : t == BENCH                ? "RESEARCH"
                                       : "OPEN STORAGE";
}

/* Expand travel distances fourfold without enlarging towns or sprite scale.
   The exploration bitmap remains 384 squared: one cell covers four world units.
   Version 4 preserves the v3 byte layout, but records this new coordinate system. */
static Town oldTowns[TOWNS];
static int captured, expanded;
void world_layout(int large) {
    if (!captured) {
        memcpy(oldTowns, towns, sizeof(oldTowns));
        captured = 1;
    }
    for (int t = 0; t < TOWNS; t++) {
        towns[t] = oldTowns[t];
        if (large) {
            towns[t].x = oldTowns[t].x * 4 - 78;
            towns[t].y = oldTowns[t].y * 4 - 81;
        }
    }
    expanded = large;
}
int world_is_expanded(void) { return expanded; }
typedef struct {
    int baseExists;
    float baseX, baseY;
} OldBase;
static void expand_point(float *x, float *y, const OldBase *old) {
    float ox = *x, oy = *y;
    if (old->baseExists && distance(ox, oy, old->baseX, old->baseY) < 24) {
        *x = old->baseX * 4 - 78 + ox - old->baseX;
        *y = old->baseY * 4 - 81 + oy - old->baseY;
    } else {
        int t = -1;
        float best = 18;
        for (int j = 0; j < TOWNS; j++) {
            float d = distance(ox, oy, oldTowns[j].x, oldTowns[j].y);
            if (d < best) {
                best = d;
                t = j;
            }
        }
        *x = t >= 0 ? oldTowns[t].x * 4 - 78 + ox - oldTowns[t].x : ox * 4 - 78;
        *y = t >= 0 ? oldTowns[t].y * 4 - 81 + oy - oldTowns[t].y : oy * 4 - 81;
    }
    *x = fminf(WORLD - 3, fmaxf(3, *x));
    *y = fminf(WORLD - 3, fmaxf(3, *y));
}
void spread_frontier(Game *save) {
    world_layout(1);
    /* Only base coordinates are read from the old state during point mapping. */
    OldBase base = {0};
    base.baseExists = save->baseExists;
    base.baseX = save->baseX;
    base.baseY = save->baseY;
    for (int i = 0; i < save->actorCount; i++) {
        Actor *a = &save->actors[i];
        expand_point(&a->x, &a->y, &base);
        expand_point(&a->homeX, &a->homeY, &base);
        expand_point(&a->tx, &a->ty, &base);
        a->routeLen = a->routePos = 0;
        a->routeClock = 0;
    }
    for (int i = 0; i < save->buildingCount; i++)
        expand_point(&save->buildings[i].x, &save->buildings[i].y, &base);
    for (int i = 0; i < save->nodeCount; i++)
        expand_point(&save->nodes[i].x, &save->nodes[i].y, &base);
    save->baseX = fmaxf(3, base.baseX * 4 - 78);
    save->baseY = fmaxf(3, base.baseY * 4 - 81);
    /* Old cells represented individual world units. Reproject explored cells,
       including compact settlements, into the new four-unit survey grid. */
    unsigned char oldSeen[sizeof(save->seen)];
    memcpy(oldSeen, save->seen, sizeof(oldSeen));
    memset(save->seen, 0, sizeof(save->seen));
    for (int y = 0; y < FOG_SIZE; y++)
        for (int x = 0; x < FOG_SIZE; x++) {
            int k = y * FOG_SIZE + x;
            if (!(oldSeen[k >> 3] & (1 << (k & 7))))
                continue;
            float wx = x + .5f, wy = y + .5f;
            expand_point(&wx, &wy, &base);
            int nk = ((int)wy / 4) * FOG_SIZE + (int)wx / 4;
            save->seen[nk >> 3] |= 1 << (nk & 7);
        }
    for (int i = 0; i < save->actorCount; i++)
        if (save->actors[i].squad) {
            int x = (int)save->actors[i].x / 4, y = (int)save->actors[i].y / 4;
            for (int yy = y - 2; yy <= y + 2; yy++)
                for (int xx = x - 2; xx <= x + 2; xx++)
                    if (xx >= 0 && yy >= 0 && xx < FOG_SIZE && yy < FOG_SIZE) {
                        int k = yy * FOG_SIZE + xx;
                        save->seen[k >> 3] |= 1 << (k & 7);
                    }
        }
    /* Add specialists to migrated expeditions as well as new games. */
    for (int i = 0; i < save->actorCount; i++) {
        Actor *a = &save->actors[i];
        if (!strcmp(a->name, "Gate guard") || !strcmp(a->name, "Gate warden")) {
            a->kind = SETTLER;
            a->voice = ROLE_MEDIC;
            snprintf(a->name, 16, "Town medic");
        } else if (!strcmp(a->name, "Wayfarer")) {
            a->voice = ROLE_BARBER;
            snprintf(a->name, 16, "Trail barber");
        } else if (a->kind == CARAVAN) {
            a->kind = SETTLER;
            a->voice = ROLE_SMITH;
            snprintf(a->name, 16, "Frontier smith");
        }
    }
}
