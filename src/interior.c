#include "game.h"
#include <math.h>
extern int conscious(const Actor *);
int building_has_interior(int type) {
    return type <= TOWER || type == FORGE || type == BENCH || type == STORE;
}
int actor_inside(const Actor *a) { return depth_room(a); }
int interior_props(int building, InteriorProp *p) {
    int type = g.buildings[building].type, n = 0;
    p[n++] = (InteriorProp){5.8f, .9f, .65f, .5f, 21, ROOM_NONE};
    p[n++] = (InteriorProp){6.1f, 5.6f, .25f, .25f, 22, ROOM_NONE};
    if (type == SHOP) {
        p[n++] = (InteriorProp){3.0f, 2.1f, 1.25f, .55f, 17, ROOM_TRADE};
        p[n++] = (InteriorProp){1.1f, 4.4f, .5f, .4f, 20, ROOM_NONE};
        p[n++] = (InteriorProp){5.1f, 4.5f, .8f, .6f, 18, ROOM_NONE};
    } else if (type == FORGE || type == BENCH) {
        p[n++] = (InteriorProp){
            1.5f, 1.6f, .8f, .7f, type == FORGE ? 19 : 18, type == FORGE ? ROOM_FORGE : ROOM_RESEARCH};
        p[n++] = (InteriorProp){4.5f, 3.7f, 1.1f, .5f, 17, ROOM_NONE};
        p[n++] = (InteriorProp){1.0f, 4.5f, .5f, .4f, 20, ROOM_NONE};
    } else if (type == STORE) {
        p[n++] = (InteriorProp){2.0f, 2.0f, .5f, .4f, 20, ROOM_STORAGE};
        p[n++] = (InteriorProp){4.4f, 3.7f, .5f, .4f, 20, ROOM_STORAGE};
        p[n++] = (InteriorProp){1.1f, 4.5f, .5f, .4f, 20, ROOM_STORAGE};
    } else {
        p[n++] = (InteriorProp){1.2f, 2.0f, .75f, .9f, 16, ROOM_REST};
        p[n++] = (InteriorProp){4.6f, 4.0f, .8f, .7f, 18, ROOM_NONE};
        if (type == BAR)
            p[n++] = (InteriorProp){4.0f, 1.2f, 1.1f, .5f, 17, ROOM_TRADE};
        else
            p[n++] = (InteriorProp){1.0f, 4.5f, .5f, .4f, 20, ROOM_NONE};
    }
    return n;
}
int interior_enter(int building) {
    Actor *a = &g.actors[g.selected];
    if (building < 0 || building >= g.buildingCount || !building_has_interior(g.buildings[building].type) ||
        !conscious(a) || a->prisonTime > 0 || a->carrying >= 0)
        return 0;
    Building *b = &g.buildings[building];
    if (distance(a->x, a->y, b->x + 1.45f, b->y + .65f) > 2.1f)
        return 0;
    if (a->attackPhase || a->target >= 0) {
        log_msg("Break away from combat before entering.");
        return 0;
    }
    if (b->faction == HOSTILE || (b->owner < 0 && g.rep[b->faction] < -25)) {
        log_msg("The occupants bar the door to your faction.");
        return 0;
    }
    if(!depth_build_ready(building)) {log_msg("Finish construction before entering.");return 0;}
    g.interiorBuilding = building + 1;
    g.interiorActor = g.selected;
    g.interiorX = 3.5f;
    g.interiorY = 6.1f;
    ui.waypoint = 0;
    a->taskKind = 0;
    a->attackPhase = 0;
    a->target = -1;
    a->movedTimer = 0;
    depth_enter_room(building);
    log_msg("Inside. Walk to a service; \001 use. Return to the door to leave.");
    return 1;
}
void interior_leave(void) {
    if (!g.interiorBuilding)
        return;
    Actor *a = &g.actors[g.interiorActor];
    Building *b = &g.buildings[g.interiorBuilding - 1];
    a->x = b->x + 1.45f;
    a->y = b->y + .65f;
    a->taskKind = 0;
    a->target = -1;
    depth_leave_room(g.interiorBuilding-1);
    g.interiorBuilding = 0;
    ui.waypoint = 0;
    log_msg("Back on the frontier.");
    find_context();
}
int interior_context(void) {
    if (!g.interiorBuilding)
        return -2;
    if (distance(g.interiorX, g.interiorY, 3.5f, 6.7f) < 1.1f)
        return -1;
    InteriorProp p[8];
    int n = interior_props(g.interiorBuilding - 1, p), best = -2;
    float near = 2.0f;
    for (int i = 0; i < n; i++)
        if (p[i].action) {
            float d = distance(g.interiorX, g.interiorY, p[i].x, p[i].y);
            if (d < near) {
                near = d;
                best = i;
            }
        }
    return best;
}
const char *interior_prompt(void) {
    int c = interior_context();
    if (c == -1)
        return "LEAVE BUILDING";
    if (c < 0)
        return "WALK TO A SERVICE";
    InteriorProp p[8];
    interior_props(g.interiorBuilding - 1, p);
    switch (p[c].action) {
    case ROOM_TRADE:
        return "TRADE AT COUNTER";
    case ROOM_REST:
        return "SLEEP IN BED";
    case ROOM_FORGE:
        return "FORGE EQUIPMENT";
    case ROOM_RESEARCH:
        return "RESEARCH";
    case ROOM_STORAGE:
        return "OPEN DEPOT";
    }
    return "EXPLORE";
}
void interior_interact(void) {
    Actor *a = &g.actors[g.selected];
    if (!conscious(a)) {
        log_msg("This survivor cannot act.");
        return;
    }
    int c = interior_context();
    if (c == -1) {
        interior_leave();
        return;
    }
    /* Explicit furniture selection must never be intercepted by a nearby NPC. */
    if (c < 0) {
        log_msg("Walk closer to a counter, bed or workstation.");
        return;
    }
    int id = g.interiorBuilding - 1;
    Building *b = &g.buildings[id];
    InteriorProp p[8];
    interior_props(id, p);
    int town = town_near(b->x, b->y, 14);
    ui.selection = ui.scroll = 0;
    switch (p[c].action) {
    case ROOM_TRADE:
        if (town < 0) {
            log_msg("No merchant works here.");
            break;
        }
        if(!depth_trade_open(town))break;
        ui.tradeTown = town;
        ui.sell = ui.shopFilter = 0;
        ui.screen = SCREEN_TRADE;
        break;
    case ROOM_REST:
        if (b->owner < 0 && g.money < 25) {
            log_msg("A bed costs 25 silver.");
            break;
        }
        if (b->owner < 0)
            g.money -= 25;
        a->taskKind = 3;
        a->taskIndex = id;
        a->action = 180;
        a->autoattack = 0;
        log_msg("Resting indoors. Move to wake; bandage bleeding first.");
        break;
    case ROOM_FORGE:
        ui.screen = SCREEN_FORGE;
        break;
    case ROOM_RESEARCH:
        ui.screen = SCREEN_RESEARCH;
        break;
    case ROOM_STORAGE:
        ui.storageIndex = id;
        ui.storageSide = 0;
        ui.screen = SCREEN_STORAGE;
        break;
    }
}
static int room_clear(float x, float y) {
    if (x < .4f || y < .4f || x > 6.6f || y > 6.6f)
        return 0;
    InteriorProp p[8];
    int n = interior_props(g.interiorBuilding - 1, p);
    for (int i = 0; i < n; i++)
        if (fabsf(x - p[i].x) < p[i].w + .18f && fabsf(y - p[i].y) < p[i].d + .18f)
            return 0;
    return 1;
}
void interior_move(float dx, float dy, float dt) {
    Actor *a = &g.actors[g.selected];
    if (!conscious(a))
        return;
    float length = sqrtf(dx * dx + dy * dy);
    if (length < .15f)
        return;
    float wx = (dx + dy) * .7071f, wy = (dy - dx) * .7071f, speed = movement_speed(a) * dt;
    if (length > 1) {
        wx /= length;
        wy /= length;
    }
    float x = g.interiorX + wx * speed, y = g.interiorY + wy * speed;
    if (room_clear(x, g.interiorY))
        g.interiorX = x;
    if (room_clear(g.interiorX, y))
        g.interiorY = y;
    a->taskKind = 0;
    a->anim += dt * 7;
    a->movedTimer = .15f;
    if (dx != 0)
        a->facing = dx > 0;
}
