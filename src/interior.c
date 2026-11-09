#include "game.h"
#include <math.h>
#include <string.h>
extern int conscious(const Actor *);
static int private_chest(int building) {
    const Building *b = &g.buildings[building];
    return b->owner < 0 && b->faction > FREE &&
           (b->type == SHOP || (b->type == HOUSE && (g.seed + building * 17u) % 3 != 0));
}
static void restock_chest(int building) {
    if (!private_chest(building) || g.day < g.townLife.chestRefresh[building])
        return;
    Building *b = &g.buildings[building];
    unsigned mix = (g.seed ^ (unsigned)building * 2654435761u) + g.day * 97u;
    static const int home[] = {RATION, CLOTH, WATER, WOOD, GRAIN};
    static const int store[] = {MEDKIT, IRONBAR, CLOTH, RATION, BOLTS, HIDE};
    memset(b->storage, 0, sizeof(b->storage));
    if (b->type == SHOP) {
        b->storage[0] = (Stack){store[mix % 6], 2};
        b->storage[1] = (Stack){store[(mix % 6 + 1 + mix % 5) % 6], 1};
    } else if (mix % 5 != 0) {
        b->storage[0] = (Stack){home[mix % 5], 1 + mix % 2};
    }
    g.townLife.chestRefresh[building] = g.day + 2 + mix % 3;
}
static Stack *chest_stack(int row, int *slot) {
    Stack *stacks;
    int count;
    if (ui.chestKind == CHEST_PROPERTY) {
        if (ui.storageIndex < 0 || ui.storageIndex >= SLAVE_CAMPS)
            return NULL;
        stacks = g.field.seized[ui.storageIndex];
        count = SEIZED_STACKS;
    } else {
        if (!g.interiorBuilding || g.interiorBuilding != ui.storageIndex + 1 ||
            !private_chest(ui.storageIndex))
            return NULL;
        stacks = g.buildings[ui.storageIndex].storage;
        count = 16;
    }
    for (int i = 0; i < count; i++)
        if (stacks[i].item && stacks[i].qty && row-- == 0) {
            if (slot) *slot = i;
            return &stacks[i];
        }
    return NULL;
}
const Stack *interior_chest_item(int row) {
    if (ui.chestKind != CHEST_FIELD)
        return chest_stack(row, NULL);
    const Node *node = &g.nodes[ui.storageIndex];
    static Stack contents;
    contents = (Stack){node->item, node->qty};
    return row == 0 && !node->locked && node->remaining && node->item && node->qty ? &contents : NULL;
}
int chest_owner(void) {
    if (ui.chestKind == CHEST_PROPERTY)
        return FREE;
    if (ui.chestKind == CHEST_FIELD)
        return g.nodes[ui.storageIndex].owner;
    return g.buildings[ui.storageIndex].faction;
}
int interior_chest_count(void) {
    if (ui.chestKind == CHEST_FIELD)
        return interior_chest_item(0) != NULL;
    int count = 0;
    while (chest_stack(count, NULL))
        count++;
    return count;
}
void interior_chest_take(int row) {
    Actor *a = &g.actors[g.selected];
    const Stack *contents = interior_chest_item(row);
    if (!contents || !conscious(a))
        return;
    int item = contents->item, owner = chest_owner(), slot = 0;
    Stack *stack = ui.chestKind == CHEST_FIELD ? NULL : chest_stack(row, &slot);
    if (ui.chestKind == CHEST_PROPERTY)
        owner = g.field.seizedOwner[ui.storageIndex][slot];
    if (!inventory_add_owned(a, item, 1, owner)) {
        log_msg("Pack full.");
        return;
    }
    if (stack) {
        if (!--stack->qty) {
            stack->item = NONE;
            if (ui.chestKind == CHEST_PROPERTY)
                g.field.seizedOwner[ui.storageIndex][slot] = 0;
        }
    } else {
        g.nodes[ui.storageIndex].qty--;
    }
    log_msg("Took %s from the chest.", itemDefs[item].name);
    if (owner && ui.chestKind != CHEST_PROPERTY)
        witnessed_theft(a, owner, 6, 18);
}
/* Match an occupied home to an actual resident. Service NPCs keep their posts. */
int home_occupant(int building) {
    const Building *b = &g.buildings[building];
    int town = town_near(b->x, b->y, 18);
    if (b->type != HOUSE || b->owner >= 0 || town < 0)
        return -1;
    int night = g.dayTime < 7 || g.dayTime >= 19;
    for (int i = 0; i < g.actorCount; i++) {
        const Actor *a = &g.actors[i];
        if (a->squad || !conscious(a) || a->kind != SETTLER || a->homeTown != town ||
            a->faction != b->faction || depth_room(a) || a->prisonTime > 0 ||
            g.depth.actors[i].captive || a->target >= 0 || depth_named(i) >= 0 ||
            regional_named(i) || population_owned(i) || civil_elite_index(i) >= 0 ||
            distance(a->x, a->y, b->x, b->y) > 16)
            continue;
        int role = npc_role(a);
        if (role != ROLE_CITIZEN && role != ROLE_BARBER && role != ROLE_SMITH)
            continue;
        unsigned shift = g.seed + i * 17u + g.day;
        if (night ? shift % 5 == 0 : (shift + (int)g.dayTime / 3) % 5 != 0)
            continue;
        int chosen = -1;
        unsigned best = 0;
        for (int j = 0; j < g.buildingCount; j++) {
            const Building *home = &g.buildings[j];
            if (home->type != HOUSE || home->owner >= 0 || home->faction != a->faction ||
                distance(home->x, home->y, towns[town].x, towns[town].y) > 18)
                continue;
            unsigned score = (shift ^ (j * 65537u)) * 2654435761u;
            if (chosen < 0 || score > best) {
                best = score;
                chosen = j;
            }
        }
        if (chosen == building)
            return i;
    }
    return -1;
}
int actor_in_bed(const Actor *a) {
    int bed = a->taskIndex;
    if (a->taskKind != 3)
        return 0;
    if (bed <= -1000)
        return 1;
    if (bed < 0 || bed >= g.buildingCount)
        return 0;
    int type = g.buildings[bed].type;
    return type == BED || type == BAR || type == HOUSE || type == TOWER;
}
int rest_at(Actor *a, int bed, float x, float y, int room, int price) {
    if (!conscious(a) || a->attackPhase == 1 || a->attackPhase == 2 || a->carrying >= 0 || a->prisonTime > 0)
        return 0;
    for (int i = 0; i < g.actorCount; i++) {
        const Actor *other = &g.actors[i];
        if (other != a && other->active && !other->dead && other->taskKind == 3 &&
            depth_room(other) == room && distance(x, y, depth_x(other), depth_y(other)) < .9f) {
            log_msg("That bed is occupied.");
            return 0;
        }
    }
    if (g.money < price) {
        log_msg("A bed costs %d silver.", price);
        return 0;
    }
    if (a->taskKind == 3 && a->taskIndex == bed)
        return 1;
    g.money -= price;
    if (room) {
        g.interiorX = x;
        g.interiorY = y;
    } else {
        a->x = x;
        a->y = y;
    }
    a->taskKind = 3;
    a->taskIndex = bed;
    a->action = REST_DURATION;
    a->autoattack = a->attackPhase = 0;
    a->target = g.living.command[a - g.actors] = -1;
    g.field.playingDead[a - g.actors] = 0;
    ui.screen = SCREEN_WORLD;
    log_msg("Resting in bed.");
    return 1;
}
int building_has_interior(int type) {
    return type <= TOWER || type == FORGE || type == BENCH || type == STORE || type == JAIL;
}
int actor_inside(const Actor *a) { return depth_room(a); }
int interior_props(int building, InteriorProp *p) {
    int type = g.buildings[building].type, n = 0;
    p[n++] = (InteriorProp){5.8f, .9f, .65f, .5f, 21, ROOM_NONE};
    p[n++] = (InteriorProp){6.1f, 5.6f, .25f, .25f, 22, ROOM_NONE};
    if (type == JAIL) {
        p[n++] = (InteriorProp){1.3f, 1.4f, .65f, .55f, 16, ROOM_NONE};
        p[n++] = (InteriorProp){5.5f, 1.4f, .65f, .55f, 16, ROOM_NONE};
    } else if (type == SHOP) {
        p[n++] = (InteriorProp){3.0f, 2.1f, 1.25f, .55f, 17, ROOM_TRADE};
        p[n++] = (InteriorProp){1.1f, 4.4f, .5f, .4f, 20,
                                private_chest(building) ? ROOM_CHEST : ROOM_NONE};
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
        else if (private_chest(building))
            p[n++] = (InteriorProp){1.0f, 4.5f, .5f, .4f, 20, ROOM_CHEST};
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
    if (a->attackPhase == 1 || a->attackPhase == 2)
        return 0;
    if(!depth_build_ready(building)) {log_msg("Finish construction before entering.");return 0;}
    g.interiorBuilding = building + 1;
    g.interiorActor = g.selected;
    g.interiorX = 3.5f;
    g.interiorY = 6.1f;
    a->taskKind = 0;
    a->attackPhase = 0;
    a->target = -1;
    a->movedTimer = 0;
    g.depth.actors[g.selected].trespass = 0;
    g.field.playingDead[g.selected] = 0;
    depth_enter_room(building);
    log_msg("Inside. Walk to a service; \001 use. Return to the door to leave.");
    return 1;
}
void interior_leave(void) {
    if (!g.interiorBuilding)
        return;
    Actor *a = &g.actors[g.interiorActor];
    Building *b = &g.buildings[g.interiorBuilding - 1];
    if (a->prisonTime > 0 && !jail_cell_open(a))
        return;
    jail_exit(a);
    a->x = b->x + 1.45f;
    a->y = b->y + .65f;
    a->taskKind = 0;
    a->target = -1;
    g.depth.actors[g.interiorActor].trespass = 0;
    depth_leave_room(g.interiorBuilding-1);
    g.interiorBuilding = 0;
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
    case ROOM_CHEST:
        return "SEARCH OWNED CHEST";
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
        if (b->type == HOUSE && b->owner < 0) {
            log_msg("Private bed. Try the inn.");
            break;
        }
        rest_at(a, id, p[c].x, p[c].y, id + 1, b->owner < 0 && town >= 0 ? regional_bed_cost(town) : 0);
        break;
    case ROOM_CHEST:
        restock_chest(id);
        ui.storageIndex = id;
        ui.chestKind = CHEST_ROOM;
        ui.screen = SCREEN_CHEST;
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
    if (g.buildings[g.interiorBuilding - 1].type == JAIL &&
        fabsf(y - 3.65f) < .2f && fabsf(x - 3.5f) > .85f)
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
    InteriorProp props[8];
    int count = interior_props(g.interiorBuilding - 1, props);
    for (int i = 0; i < count; i++)
        if (props[i].action == ROOM_REST &&
            distance(g.interiorX, g.interiorY, props[i].x, props[i].y) < .3f)
            g.interiorX = props[i].x + props[i].w + .25f;
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
    walk_motion(a, g.interiorX - (x - wx * speed), g.interiorY - (y - wy * speed));
}
