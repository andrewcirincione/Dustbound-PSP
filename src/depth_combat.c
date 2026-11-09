#include "game.h"
#include <math.h>
#include <stdint.h>
extern int conscious(const Actor *);
extern int hostile(const Actor *, const Actor *);
#define D (g.depth)
static int index_of(const Actor *a) {
    uintptr_t n = (uintptr_t)a, b = (uintptr_t)g.actors;
    return n >= b && n < b + sizeof(g.actors) ? (int)((n - b) / sizeof(Actor)) : -1;
}
int depth_room(const Actor *a) {
    int n = index_of(a);
    if (n < 0)
        return 0;
    return g.interiorBuilding && n == g.interiorActor ? g.interiorBuilding : D.actors[n].room;
}
float depth_x(const Actor *a) {
    int n = index_of(a);
    return depth_room(a) ? n == g.interiorActor && g.interiorBuilding ? g.interiorX : D.actors[n].rx
                         : a->x;
}
float depth_y(const Actor *a) {
    int n = index_of(a);
    return depth_room(a) ? n == g.interiorActor && g.interiorBuilding ? g.interiorY : D.actors[n].ry
                         : a->y;
}
float depth_distance(const Actor *a, const Actor *b) {
    return depth_room(a) != depth_room(b) ? 9999
                                          : distance(depth_x(a), depth_y(a), depth_x(b), depth_y(b));
}
static int room_clear(int room, float x, float y) {
    if (room <= 0 || room > g.buildingCount || x < .4f || y < .4f || x > 6.6f || y > 6.6f)
        return 0;
    InteriorProp props[ROOM_PROPS];
    int n = interior_props(room - 1, props);
    for (int i = 0; i < n; i++)
        if (fabsf(x - props[i].x) < props[i].w + .18f && fabsf(y - props[i].y) < props[i].d + .18f)
            return 0;
    return 1;
}
static int shot_clear(float x, float y) {
    if (x < 0 || y < 0 || x >= WORLD || y >= WORLD)
        return 0;
    /* Water blocks walking, not a projectile traveling above the ground. */
    for (int i = 0; i < g.buildingCount; i++) {
        Building *b = &g.buildings[i];
        if (b->type != HOUSE && b->type != SHOP && b->type != BAR && b->type != TOWER && b->type != WALL && b->type != JAIL)
            continue;
        float r = b->type == TOWER || b->type == JAIL ? .8f : b->type == WALL ? .55f : 1.25f;
        if (fabsf(b->x - x) < r && fabsf(b->y - y) < r)
            return 0;
    }
    return 1;
}
void depth_move_room(Actor *a, float dx, float dy, float dt) {
    int n = index_of(a), room = depth_room(a);
    if (n < 0 || !room || !conscious(a) || D.actors[n].captive || g.hitstop>0 || a->attackPhase==1 || a->attackPhase==2)
        return;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < .01f)
        return;
    float step = movement_speed(a) * dt / len;
    float x = depth_x(a), y = depth_y(a), ox=x, oy=y, nx = x + dx * step, ny = y + dy * step;
    if (room_clear(room, nx, y))
        x = nx;
    if (room_clear(room, x, ny))
        y = ny;
    if (g.interiorBuilding && n == g.interiorActor) {
        g.interiorX = x;
        g.interiorY = y;
    } else {
        D.actors[n].rx = x;
        D.actors[n].ry = y;
    }
    walk_motion(a, x - ox, y - oy);
}
void depth_enter_room(int building) {
    Actor *a = &g.actors[g.selected];
    int room = building + 1;
    for (int i = 0; i < g.actorCount; i++) {
        Actor *b = &g.actors[i];
        if (b == a || !b->squad || !conscious(b) || depth_room(b) || D.actors[i].paused ||
            assigned_job(b) != JOB_FOLLOW || distance(a->x, a->y, b->x, b->y) >= 4)
            continue;
        D.actors[i].room = room;
        D.actors[i].rx = 2.8f + (i % 2) * 1.3f;
        D.actors[i].ry = 5.8f;
    }
}
void actor_leave_room(int n) {
    Actor *a = &g.actors[n];
    int room = depth_room(a);
    if (!room)
        return;
    const Building *b = &g.buildings[room - 1];
    D.actors[n].room = 0;
    D.actors[n].trespass = 0;
    a->x = b->x + 1.45f;
    a->y = b->y + .65f;
    a->routeLen = a->routePos = 0;
    a->target = -1;
    a->attackPhase = 0;
    if (g.interiorBuilding && n == g.interiorActor)
        g.interiorBuilding = 0;
}
void depth_leave_room(int building) {
    for (int i = 0; i < g.actorCount; i++)
        if (g.actors[i].squad && D.actors[i].room == building + 1 && g.actors[i].prisonTime <= 0 &&
            !g.field.intake[i])
            actor_leave_room(i);
}
int depth_room_target(void) {
    Actor *a = &g.actors[g.selected];
    int nearest = -1;
    float reach = TALK_REACH;
    for (int i = 0; i < g.actorCount; i++)
        if (i != g.selected && g.actors[i].active && depth_room(&g.actors[i]) == g.interiorBuilding) {
            Actor *b = &g.actors[i];
            if (conscious(b) && hostile(a, b) && (b->target >= 0 || a->target == i))
                continue;
            float d = depth_distance(a, &g.actors[i]);
            if (d < reach) {
                nearest = i;
                reach = d;
            }
        }
    return nearest;
}
int depth_room_interact(void) {
    int nearest = depth_room_target();
    if (nearest < 0) return 0;
    Actor *b = &g.actors[nearest];
    ui.selection = 0;
    if (b->squad) {
        ui.lootActor = nearest;
        ui.screen = SCREEN_ASSIST;
    } else if (b->dead || b->unconscious > 0) {
        ui.lootActor = nearest;
        ui.screen = SCREEN_LOOT;
    } else if (g.actors[g.selected].sneaking) {
        ui.lootActor = nearest;
        ui.screen = SCREEN_THEFT;
    } else
        dialogue_open(nearest);
    return 1;
}
void depth_room_tick(float dt) {
    for (int i = 0; i < g.actorCount; i++) {
        Actor *a = &g.actors[i];
        int room = depth_room(a);
        if (!room || !conscious(a) || a->prisonTime > 0 || playing_dead(a))
            continue;
        if (a->squad) {
            Building *b = &g.buildings[room - 1];
            DepthActor *z = &D.actors[i];
            int unwelcome = b->owner < 0 && g.rep[b->faction] < -25 &&
                (disguise_score(a, b->faction) < 40 || g.society.recognized[i][b->faction] > 0);
            int privateRoom = b->owner < 0 && (b->type == HOUSE || b->type == TOWER || b->type == JAIL ||
                                               (b->type == SHOP && (g.dayTime < 6 || g.dayTime >= 22)));
            int witness = -1;
            if (unwelcome || privateRoom) {
                for (int j = 0; j < g.actorCount; j++) {
                    Actor *w = &g.actors[j];
                    if (!w->squad && w->faction == b->faction && depth_room(w) == room &&
                        npc_sees(w, a, npc_sight_range(w))) {
                        witness = j;
                        break;
                    }
                }
            }
            float before = z->trespass;
            if (unwelcome && (witness >= 0 || before > 0)) {
                z->trespass += dt;
                if (!before)
                    log_warn("%s: Get out. You've got no business here.", g.actors[witness].name);
                if (before < 7 && z->trespass >= 7) {
                    if (witness >= 0) {
                        g.actors[witness].target = i;
                        g.living.anger[witness] = 1;
                    }
                    log_warn("The occupants are turning on %s.", a->name);
                }
            } else if (privateRoom && witness >= 0) {
                z->trespass += dt;
                if (before < 5 && z->trespass >= 5)
                    log_msg("Private property. Leave before the occupants raise alarm.");
                if (before < 12 && z->trespass >= 12) {
                    crime_charge(a, b->faction, 80, CRIME_TRESPASS);
                    g.rep[b->faction] -= 30;
                    log_warn("The occupants called the watch.");
                }
            } else if (!privateRoom) {
                z->trespass = 0;
            }
        }
        int target = -1;
        float best = 10;
        for (int j = 0; j < g.actorCount; j++)
            if (i != j && conscious(&g.actors[j]) && !playing_dead(&g.actors[j]) && hostile(a, &g.actors[j]) &&
                (!g.actors[j].sneaking || npc_sees(a, &g.actors[j], npc_sight_range(a)))) {
                float d = depth_distance(a, &g.actors[j]);
                if (d < best) {
                    target = j;
                    best = d;
                }
            }
        if (target >= 0 && (!a->squad || a->autoattack)) {
            Actor *b = &g.actors[target];
            int eviction = b->squad && D.actors[target].trespass >= 7 && a->gear[0];
            if (civilian_flees(a) && !eviction) {
                a->target = -1;
                depth_move_room(a, depth_x(a) - depth_x(b), depth_y(a) - depth_y(b), dt);
                continue;
            }
            a->target = target;
            if (depth_tactics(a, target, dt))
                continue;
            float range = a->gear[0] ? itemDefs[a->gear[0]].range : 1.55f;
            if (best <= range)
                strike(i, target);
            else if (i != g.selected)
                depth_move_room(a, depth_x(b) - depth_x(a), depth_y(b) - depth_y(a), dt);
        } else
            a->target = -1;
    }
}
int depth_tactics(Actor *a, int target, float dt) {
    int n = index_of(a);
    if (n < 0 || target < 0 || target >= g.actorCount)
        return 0;
    int order = D.actors[n].order;
    if (a->carrying >= 0 && D.actors[a->carrying].captive == 2)
        return 1;
    if (D.actors[n].captive)
        return 1;
    if (order == ORDER_HOLD_FIRE || order == ORDER_BLOCK)
        return 1;
    Actor *b = &g.actors[target];
    float d = depth_distance(a, b);
    if (order == ORDER_RANGED && weapon_ranged(a->gear[0]) && d < 3.5f && n != g.selected) {
        if (depth_room(a))
            depth_move_room(a, depth_x(a) - depth_x(b), depth_y(a) - depth_y(b), dt);
        else
            move_actor(a, a->x - b->x, a->y - b->y, dt);
        return 1;
    }
    return 0;
}
int depth_fire(int source, int target, int weapon) {
    Actor *a = &g.actors[source], *b = &g.actors[target];
    for (int i = 0; i < PROJECTILES; i++)
        if (!D.shots[i].active) {
            float dx = depth_x(b) - depth_x(a), dy = depth_y(b) - depth_y(a),
                  length = fmaxf(.01f, sqrtf(dx * dx + dy * dy));
            D.shots[i] = (Projectile){1,
                                      source,
                                      weapon,
                                      depth_room(a),
                                      depth_x(a),
                                      depth_y(a),
                                      dx / length * 18,
                                      dy / length * 18,
                                      itemDefs[weapon].range + .6f};
            return 1;
        }
    return 0;
}
void depth_projectiles(float dt) {
    for (int i = 0; i < PROJECTILES; i++) {
        Projectile *p = &D.shots[i];
        if (!p->active)
            continue;
        /* Substeps cap movement at 0.15m, preventing tunnelling through
         * actors/walls. */
        int steps = (int)ceilf(dt * 120);
        if (steps < 1)
            steps = 1;
        float step = dt / steps;
        for (int k = 0; k < steps && p->active; k++) {
            p->x += p->vx * step;
            p->y += p->vy * step;
            p->left -= 18 * step;
            if (p->left <= 0 || (p->room ? !room_clear(p->room, p->x, p->y) : !shot_clear(p->x, p->y))) {
                p->active = 0;
                break;
            }
            for (int j = 0; j < g.actorCount; j++) {
                Actor *a = &g.actors[j];
                if (j == p->source || !conscious(a) || depth_room(a) != p->room)
                    continue;
                if (distance(p->x, p->y, depth_x(a), depth_y(a)) < .34f) {
                    combat_impact(p->source, j, p->weapon);
                    p->active = 0;
                    break;
                }
            }
        }
    }
}
