#include "game.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
extern int conscious(const Actor *);
extern int add_actor(const char *, int, int, float, float, int);
extern int add_build(float, float, int, int, int);
extern int hostile(const Actor *, const Actor *);
#define D (g.depth)
const char *landmarkNames[LANDMARKS] = {"SALT ROAD WRECK", "DRY CULVERT",  "RELL'S TOLL CAMP",
                                        "CHAIN YARD",      "ASH SHRINE",   "OLD FOUNDRY",
                                        "WATCH BEACON",    "COPPER STORES"};
const char *orderNames[4] = {"Balanced", "Defensive", "Keep range", "Hold fire"};
static int member(const Actor *a) {
    return (uintptr_t)a >= (uintptr_t)g.actors && (uintptr_t)a < (uintptr_t)(g.actors + MAX_ACTORS)
               ? (int)(a - g.actors)
               : -1;
}
static int alive(int n) { return n >= 0 && n < g.actorCount && g.actors[n].active && !g.actors[n].dead; }
/* Distance feedback must not conceal the actual reason an action is blocked. */
static int ready_to_act(const Actor *a) {
    if (conscious(a))
        return 1;
    if (!a->active || a->dead)
        log_msg("This survivor cannot act. Select a living ally.");
    else if (a->carriedBy >= 0)
        log_msg("Set this survivor down before acting.");
    else if (a->unconscious > 0)
        log_msg("Unconscious: recover or send an ally to help.");
    else
        log_msg("Stunned: resume time to recover before acting.");
    return 0;
}
int depth_named(int actor) {
    for (int i = 0; i < DEPTH_NPCS; i++)
        if (D.npc[i] == actor && actor >= 0)
            return i;
    return -1;
}
void depth_open(int screen) {
    ui.screen = screen == SCREEN_ORDERS ? SCREEN_SQUAD : screen == SCREEN_CARE ? SCREEN_HEALTH : screen;
    ui.selection = ui.scroll = 0;
}
static void clear_spot(float *x, float *y) {
    float ox = *x, oy = *y;
    for (int r = 0; r < 10; r++)
        for (int k = 0; k < 16; k++) {
            float a = k * .392699f, xx = ox + cosf(a) * r, yy = oy + sinf(a) * r;
            if (can_walk(xx, yy) && can_walk(xx + .7f, yy) && can_walk(xx, yy + .7f)) {
                *x = xx;
                *y = yy;
                return;
            }
        }
}
void depth_init(void) {
    if (D.version == 1)
        return;
    memset(&D, 0, sizeof(D));
    D.version = 1;
    D.lastDay = g.day;
    D.retaliation = 360;
    for (int i = 0; i < MAX_ACTORS; i++) {
        D.actors[i].upkeep = 100;
        for (int j = 0; j < 4; j++)
            D.actors[i].queue[j] = JOB_HOLD;
    }
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        D.buildings[i].condition = 100;
        D.buildings[i].fuel = 20;
    }
    for (int i = 0; i < DEPTH_NPCS; i++)
        D.npc[i] = -1;
    for (int i = 0; i < 3; i++)
        D.groupLeader[i] = -1;
    D.groupLeader[0] = g.selected;
    const float coords[LANDMARKS][2] = {{100, 43.4f}, {144, 19.8f}, {162, 47.6f}, {173, 57.6f},
                                        {72, 14.6f},  {258, 289},   {513, 73},    {225, 53}};
    for (int i = 0; i < LANDMARKS; i++) {
        D.siteX[i] = coords[i][0];
        D.siteY[i] = coords[i][1];
        clear_spot(&D.siteX[i], &D.siteY[i]);
        D.siteBuilding[i] = -1;
        if (g.buildingCount < MAX_BUILDINGS) {
            int type = i == SITE_SHRINE    ? BED
                       : i == SITE_FOUNDRY ? FORGE
                       : i == SITE_CAGES   ? WALL
                       : i == SITE_STORES  ? STORE
                                           : CAMPFIRE;
            D.siteBuilding[i] = add_build(D.siteX[i] - 2.5f, D.siteY[i] - 1.8f, type,
                                          i == SITE_CAMP || i == SITE_CAGES ? OUTLAWS : FREE, -1);
        }
    }
    /* Never recycle somebody's corpse or overwrite an old save's full actor pool.
     */
    if (g.actorCount + DEPTH_NPCS > MAX_ACTORS) {
        log_msg("Older expedition is full: landmark salvage is available.");
        return;
    }
    const char *names[DEPTH_NPCS] = {"SORA",   "NELL",        "RELL",       "ORIN",
                                     "TAMSIN", "ROAD WARDEN", "ROAD SCOUT", "COPPER CONVOY"};
    const int sites[DEPTH_NPCS] = {0, 7, 2, 3, 3, 2, 2, 7};
    for (int i = 0; i < DEPTH_NPCS; i++) {
        int site = sites[i];
        float x = D.siteX[site] + (i == NPC_TAMSIN ? 1.2f : 0), y = D.siteY[site];
        clear_spot(&x, &y);
        int n = add_actor(names[i],
                          i == NPC_RELL                            ? RAIDER
                          : i == NPC_PATROL_A || i == NPC_PATROL_B ? GUARD
                                                                   : SETTLER,
                          i == NPC_RELL || i == NPC_PATROL_A || i == NPC_PATROL_B ? OUTLAWS : FREE, x, y,
                          i == NPC_RELL ? 18 : 8);
        D.npc[i] = n;
        Actor *a = &g.actors[n];
        a->voice = i == NPC_NELL ? ROLE_SCOUT : ROLE_CITIZEN;
        a->gear[1] = gear_id(i == NPC_RELL ? 34 : 45, 0, 1);
        a->gear[2] = gear_id(i == NPC_RELL ? 56 : 49, 0, 0);
        a->gear[0] = i == NPC_RELL ? gear_id(84, 0, 1) : STAFF;
        if (i == NPC_SORA) {
            a->health[LLEG] = 12;
            a->health[CHEST] = 42;
            a->bleed = .8f;
            D.actors[n].trauma[LLEG] = 30;
        }
        if (i == NPC_ORIN || i == NPC_TAMSIN) {
            D.actors[n].captive = 1;
            D.actors[n].confiscated = a->gear[0];
            a->gear[0] = 0;
        }
        if (i == NPC_PATROL_A || i == NPC_PATROL_B) {
            a->x += 4 + (i - NPC_PATROL_A) * 3;
            a->homeX = a->x;
            a->tx = a->x;
        }
    }
}
int depth_validate(const Game *s) {
    const FrontierDepth *d = &s->depth;
    if (!d->version)
        return 1; /* legacy prefix */
    if (d->version != 1 || d->route < 0 || d->route > ROUTE_FREED || d->rescued < 0 || d->rescued > 2 ||
        d->reported < 0 || d->reported > 1 || d->visited < 0 || d->visited >= 256)
        return 0;
    if (!isfinite(d->retaliation) || !isfinite(d->convoyClock))
        return 0;
    for (int i = 0; i < DEPTH_NPCS; i++)
        if (d->npc[i] < -1 || d->npc[i] >= s->actorCount)
            return 0;
    for (int i = 0; i < LANDMARKS; i++)
        if (!isfinite(d->siteX[i]) || !isfinite(d->siteY[i]) || d->siteX[i] < 0 || d->siteY[i] < 0 ||
            d->siteX[i] > WORLD || d->siteY[i] > WORLD || d->siteBuilding[i] < -1 ||
            d->siteBuilding[i] >= s->buildingCount)
            return 0;
    for (int i = 0; i < 3; i++)
        if (d->groupLeader[i] < -1 || d->groupLeader[i] >= s->actorCount || !isfinite(d->groupX[i]) ||
            !isfinite(d->groupY[i]) || d->groupX[i] < 0 || d->groupY[i] < 0 || d->groupX[i] > WORLD ||
            d->groupY[i] > WORLD)
            return 0;
    for (int i = 0; i < MAX_ACTORS; i++) {
        const DepthActor *a = &d->actors[i];
        if (a->order < 0 || a->order > 3 || a->group < 0 || a->group > 2 || a->room < 0 ||
            a->room > s->buildingCount || a->layer < 0 || a->layer >= ITEMS || a->confiscated < 0 ||
            a->confiscated >= ITEMS || a->captive < 0 || a->captive > 3 || !isfinite(a->upkeep) ||
            a->upkeep < 0 || a->upkeep > 100 || !isfinite(a->escape) || !isfinite(a->recognized) ||
            !isfinite(a->trespass) || !isfinite(a->rx) || !isfinite(a->ry))
            return 0;
        for (int j = 0; j < 4; j++)
            if (a->queue[j] < 0 || a->queue[j] > JOB_HAUL)
                return 0;
        for (int j = 0; j < LIMBS; j++)
            if (!isfinite(a->trauma[j]) || a->trauma[j] < 0 || a->trauma[j] > 80)
                return 0;
    }
    for (int i = 0; i < MAX_BUILDINGS; i++)
        if (!isfinite(d->buildings[i].labor) || d->buildings[i].labor < 0 ||
            !isfinite(d->buildings[i].fuel) || d->buildings[i].fuel < 0 || d->buildings[i].fuel > 1000 ||
            !isfinite(d->buildings[i].condition) || d->buildings[i].condition < 0 ||
            d->buildings[i].condition > 100)
            return 0;
    for (int i = 0; i < PROJECTILES; i++) {
        const Projectile *p = &d->shots[i];
        if (p->active &&
            (p->source < 0 || p->source >= s->actorCount || p->weapon < 0 || p->weapon >= ITEMS ||
             p->room < 0 || p->room > s->buildingCount || !isfinite(p->x) || !isfinite(p->y) ||
             !isfinite(p->vx) || !isfinite(p->vy) || !isfinite(p->left) || p->left < 0 || p->left > 100))
            return 0;
    }
    return 1;
}
int depth_near_site(const Actor *a, float reach) {
    if (actor_inside(a))
        return -1;
    int best = -1;
    for (int i = 0; i < LANDMARKS; i++) {
        float d = distance(a->x, a->y, D.siteX[i], D.siteY[i]);
        if (d < reach) {
            reach = d;
            best = i;
        }
    }
    return best;
}
const char *depth_outcome(void) {
    static const char *s[] = {"BLOCKADE / Copper short of supplies",
                              "BYPASS / Independent trade restored",
                              "TRUCE / Supplies under Rell's toll", "CAPTURE / Road under town patrol",
                              "FREED / Leader dead; reprisals likely"};
    return s[D.route];
}
const char *depth_objective(void) {
    if (D.reported)
        return "Return to Copper: see how your choice changed trade.";
    if (D.route)
        return "Report the open road to Nell at Copper stores.";
    if (D.rescued == 1)
        return "Escort Sora to Copper; keep her alive.";
    if (D.rescued == 2)
        return "Repair the culvert, bargain with Rell, or capture him.";
    return "Find Sora at the Salt Road wreck. Bring dressings.";
}
void depth_locate(int site) {
    if (site < 0 || site >= LANDMARKS)
        return;
    ui.mapX = D.siteX[site];
    ui.mapY = D.siteY[site];
    ui.mapZoom = 0;
    depth_open(SCREEN_MAP);
    log_msg("%s marked. \001 places a manual map pin.", landmarkNames[site]);
}
static void route_state(int state) {
    if (D.route)
        return;
    D.route = state;
    D.retaliation = 360;
    g.townOwner[1] = state == ROUTE_TRUCE ? OUTLAWS : towns[1].faction;
    g.prosperity[1] = state == ROUTE_TRUCE ? 48 : 68;
    for (int i = 0; i < g.actorCount; i++)
        if (!g.actors[i].squad && g.actors[i].homeTown == 1)
            g.actors[i].faction = g.townOwner[1];
    for (int i = 0; i < g.buildingCount; i++)
        if (g.buildings[i].owner < 0 && town_near(g.buildings[i].x, g.buildings[i].y, 12) == 1)
            g.buildings[i].faction = g.townOwner[1];
    for (int i = NPC_PATROL_A; i <= NPC_PATROL_B; i++)
        if (alive(D.npc[i]))
            g.actors[D.npc[i]].faction = state == ROUTE_TRUCE ? OUTLAWS : FREE;
    g.stock[1][RATION] += 8;
    g.stock[1][MEDKIT] += 3;
    g.stock[1][IRONBAR] += 4;
    snprintf(g.worldNews, sizeof(g.worldNews), "Copper Cross: %s", depth_outcome());
    log_msg("%s", g.worldNews);
}
const char *depth_line(int actor) {
    switch (depth_named(actor)) {
    case NPC_SORA:
        return D.rescued == 2 ? "The dry culvert can carry wagons. Three timber, one iron bar "
                                "and tools will brace it. I left the route on your atlas."
               : D.rescued    ? "Stay close. I cannot fight on this leg. Carry me if needed. "
                                "Get me to the stores at Copper Cross."
                              : "Rell's men broke the wagon. My leg is bleeding. A "
                                "dressing, then an escort to Copper. There is another "
                                "way around his toll.";
    case NPC_NELL:
        return D.reported
                   ? (D.route == ROUTE_TRUCE ? "The food is back, but every wagon pays Rell. The toll "
                                               "is in our prices. You chose the cost."
                                             : "The road is open. Our shelves and patrols remember "
                                               "what you did. Sora is rebuilding the wagon line.")
                   : "Copper is running short. Sora went missing on the Salt Road. "
                     "Open a supply route and I can pay you. Bring her home if you "
                     "can.";
    case NPC_RELL:
        return D.route == ROUTE_TRUCE ? "You paid for peace. My road, my toll. Copper will eat."
                                      : "Four hundred and fifty silver buys a truce. Or try your "
                                        "steel. My scouts watch the road; the chain yard holds those "
                                        "who fail.";
    case NPC_ORIN:
    case NPC_TAMSIN:
        return D.actors[actor].captive ? "Wait until the guards move away. Tools make the lock faster. "
                                         "Don't leave me in this yard."
                                       : "I remember who opened the lock. If there is room, I'll carry "
                                         "my share.";
    default:
        return NULL;
    }
}
const char *depth_option(int actor, int row) {
    static const char *sora[] = {"Treat the wound and escort her.", "Show Copper stores on the atlas.",
                                 "Carry Sora to safety.", "Leave."};
    static const char *nell[] = {"Mark the missing wagon.", "Report the route. Claim payment.",
                                 "Buy relief cargo. 150 silver", "Mark Rell's toll camp.", "Leave."};
    static const char *rell[] = {"Pay 450 silver for a toll truce.", "Challenge Rell and his scouts.",
                                 "Bind and carry fallen Rell.", "Leave."};
    static const char *captive[] = {"Unlock shackles / recruit.", "Recover confiscated weapon.",
                                    "Leave."};
    int n = depth_named(actor);
    if (n == NPC_SORA) {
        if (row == 0 && D.rescued == 1)
            return "Treat her wounds again. One dressing.";
        return sora[row % 4];
    }
    if (n == NPC_NELL)
        return nell[row % 5];
    if (n == NPC_RELL)
        return rell[row % 4];
    if (n == NPC_ORIN || n == NPC_TAMSIN)
        return captive[row % 3];
    return "Leave.";
}
int depth_dialog(int actor, int row) {
    if (actor < 0 || actor >= g.actorCount)
        return 0;
    Actor *a = &g.actors[g.selected], *b = &g.actors[actor];
    if (!ready_to_act(a))
        return 0;
    if (depth_distance(a, b) > TALK_REACH) {
        log_msg("Move within talking reach.");
        return 0;
    }
    int n = depth_named(actor);
    if (n == NPC_SORA) {
        if (row == 0 && D.rescued < 2) {
            if (!heal_actor(g.selected, actor)) {
                log_msg("Bring a dressing and treat Sora.");
                return 0;
            }
            D.rescued = 1;
            b->bleed = 0;
            log_msg("Sora follows while you stay within 18m.");
            depth_open(SCREEN_WORLD);
        } else if (row == 1)
            depth_locate(SITE_STORES);
        else if (row == 2) {
            if (D.rescued != 1 || b->dead || b->carriedBy >= 0 || a->carrying >= 0) {
                log_msg("Treat Sora first. Carry needs free arms and a living escort.");
                return 0;
            }
            a->carrying = actor;
            b->carriedBy = g.selected;
            a->taskKind = 0;
            ui.waypoint = 0;
            depth_open(SCREEN_WORLD);
            log_msg("Carrying Sora. Travel is slower; \001 puts her down.");
        } else if (row == 3)
            depth_open(SCREEN_WORLD);
    } else if (n == NPC_NELL) {
        if (row == 0)
            depth_locate(SITE_WRECK);
        if (row == 1) {
            int rell = D.npc[NPC_RELL];
            if (!D.route && rell >= 0 && a->carrying == rell) {
                g.actors[rell].carriedBy = -1;
                a->carrying = -1;
                D.actors[rell].captive = 1;
                g.actors[rell].x = D.siteX[7];
                g.actors[rell].y = D.siteY[7] + 2;
                route_state(ROUTE_CAPTURED);
            }
            if (!D.route)
                log_msg("Open a route first: culvert, truce, or Rell.");
            else if (D.reported)
                log_msg("You have already claimed this reward.");
            else {
                int reward = D.route == ROUTE_TRUCE ? 120 : D.route == ROUTE_CAPTURED ? 550 : 350;
                g.money += reward;
                D.reported = 1;
                g.rep[g.townOwner[1]] += 8;
                log_msg("Route payment: %d silver. Trade will remember.", reward);
            }
        }
        if (row == 2) {
            Actor old = *a;
            if (g.money < 150)
                log_msg("Relief cargo costs 150 silver.");
            else if (!inventory_add(a, RATION, 6) || !inventory_add(a, MEDKIT, 2)) {
                *a = old;
                log_msg("Make room for rations and dressings.");
            } else {
                g.money -= 150;
                D.supplied++;
                log_msg("Six rations and two dressings packed.");
            }
        }
        if (row == 3)
            depth_locate(SITE_CAMP);
        if (row == 4)
            depth_open(SCREEN_WORLD);
    } else if (n == NPC_RELL) {
        if (row == 0) {
            if (D.route)
                log_msg("The road already has an outcome.");
            else if (!conscious(b))
                log_msg("Rell cannot negotiate while unconscious.");
            else if (g.money < 450)
                log_msg("Rell demands 450 silver.");
            else {
                g.money -= 450;
                route_state(ROUTE_TRUCE);
                depth_open(SCREEN_WORLD);
            }
        }
        if (row == 1) {
            if (D.route)
                log_msg("The road outcome is already settled.");
            else {
                b->faction = HOSTILE;
                for (int j = NPC_PATROL_A; j <= NPC_PATROL_B; j++)
                    if (alive(D.npc[j]))
                        g.actors[D.npc[j]].faction = HOSTILE;
                depth_open(SCREEN_WORLD);
                a->autoattack = 1;
                log_msg("Rell's scouts draw steel.");
            }
        }
        if (row == 2) {
            if (b->dead || b->unconscious <= 0 || a->carrying >= 0 || b->carriedBy >= 0)
                log_msg("Rell must be alive and unconscious. Free your arms.");
            else {
                a->carrying = actor;
                b->carriedBy = g.selected;
                depth_open(SCREEN_WORLD);
                log_msg("Carry Rell to Nell at Copper stores.");
            }
        }
        if (row == 3)
            depth_open(SCREEN_WORLD);
    } else if (n == NPC_ORIN || n == NPC_TAMSIN) {
        if (row == 0)
            depth_capture_action(actor);
        if (row == 1) {
            DepthActor *z = &D.actors[actor];
            if (z->captive)
                log_msg("Unlock the captive first.");
            else if (z->confiscated && inventory_add_owned(b, z->confiscated, 1, z->confiscatedOwner)) {
                z->confiscated = 0;
                log_msg("Confiscated weapon recovered.");
            }
        }
        if (row == 2)
            depth_open(SCREEN_WORLD);
    }
    return 1;
}
int depth_action(int site, int option) {
    Actor *a = &g.actors[g.selected];
    if (!ready_to_act(a))
        return 0;
    if (site < 0 || site >= LANDMARKS || depth_near_site(a, 1.7f) != site) {
        log_msg("Walk within 1.7m of the landmark.");
        return 0;
    }
    if (site == SITE_CULVERT) {
        if (D.route) {
            log_msg("The supply road is already open.");
            return 0;
        }
        if (D.rescued < 2) {
            log_msg("Sora must reach Copper before the repair is safe.");
            return 0;
        }
        if (inventory_count(a, WOOD) < 3 || inventory_count(a, IRONBAR) < 1 ||
            !inventory_count(a, TOOLS)) {
            log_msg("Need 3 timber, 1 iron bar and reusable tools.");
            return 0;
        }
        inventory_remove(a, WOOD, 3);
        inventory_remove(a, IRONBAR, 1);
        gain(a, ENGINEERING, 80);
        route_state(ROUTE_BYPASS);
        return 1;
    }
    if (option == 1) {
        depth_locate(SITE_CAGES);
        return 1;
    }
    if (D.salvage & (1 << site)) {
        log_msg("This cache is empty. Its story remains.");
        return 0;
    }
    int item = site == SITE_FOUNDRY  ? PARTS
               : site == SITE_BEACON ? RELIC
               : site == SITE_SHRINE ? MEDKIT
                                     : IRONBAR;
    int qty = site == SITE_FOUNDRY ? 5 : site == SITE_BEACON ? 2 : 3;
    if (!inventory_add(a, item, qty)) {
        log_msg("Pack full.");
        return 0;
    }
    D.salvage |= 1 << site;
    gain(a, PERCEPTION, 40);
    log_msg("Recovered %d %s. Check expedition leads.", qty, itemDefs[item].name);
    return 1;
}
int depth_trade_open(int town) {
    if (town < 0)
        return 0;
    if (g.dayTime < 6 || g.dayTime >= 22) {
        log_msg("Markets open at 06:00. Find an inn until dawn.");
        return 0;
    }
    return 1;
}
int depth_camp_count(void) {
    int n = 0;
    for (int i = 0; i < g.buildingCount; i++)
        if (g.buildings[i].owner >= 0)
            n++;
    return n;
}
int depth_camp_build(int row) {
    for (int i = 0; i < g.buildingCount; i++)
        if (g.buildings[i].owner >= 0 && row-- == 0)
            return i;
    return -1;
}
int depth_build_ready(int b) {
    return b >= 0 && b < g.buildingCount && D.buildings[b].labor <= 0 && D.buildings[b].condition > 0;
}
int depth_build_action(int b) {
    Actor *a = &g.actors[g.selected];
    if (b < 0 || b >= g.buildingCount || !conscious(a) ||
        distance(a->x, a->y, g.buildings[b].x, g.buildings[b].y) > 2.5f)
        return 0;
    if (D.buildings[b].labor > 0) {
        a->taskKind = 4;
        a->taskIndex = b;
        a->action = D.buildings[b].labor;
        depth_open(SCREEN_WORLD);
        log_msg("Construction started. Stay nearby; movement cancels.");
        return 1;
    }
    if (D.buildings[b].condition < 100) {
        if (!inventory_remove(a, WOOD, 1)) {
            log_msg("One timber repairs this structure.");
            return 0;
        }
        D.buildings[b].condition = fminf(100, D.buildings[b].condition + 35);
        gain(a, ENGINEERING, 12);
        log_msg("Structure repaired.");
        return 1;
    }
    return depth_fuel(b);
}
int depth_fuel(int b) {
    Actor *a = &g.actors[g.selected];
    if (b < 0 || b >= g.buildingCount || !depth_build_ready(b) || g.buildings[b].owner < 0 ||
        (g.buildings[b].type != FORGE && g.buildings[b].type != CAMPFIRE))
        return 0;
    if (!actor_inside(a) && distance(a->x, a->y, g.buildings[b].x, g.buildings[b].y) > 1.7f) {
        log_msg("Stand beside the fire or forge.");
        return 0;
    }
    if (D.buildings[b].fuel > 990) {
        log_msg("Fuel store is full.");
        return 0;
    }
    int fuel = 0;
    for (int i = 0; i < PACK; i++)
        if (!a->stolen[i] && (a->pack[i].item == COAL || a->pack[i].item == WOOD)) {
            fuel = a->pack[i].item;
            inventory_remove_slot(a, i, 1);
            break;
        }
    if (!fuel) {
        log_msg("Bring clean coal or timber for fuel.");
        return 0;
    }
    D.buildings[b].fuel += fuel == COAL ? 10 : 4;
    log_msg("Fuel loaded: %.0f production charges.", D.buildings[b].fuel);
    return 1;
}
int depth_use_fuel(int b, float amount) {
    if (!depth_build_ready(b)) {
        log_msg("Finish construction or repair the station.");
        return 0;
    }
    if (g.buildings[b].owner < 0)
        return 1;
    if (D.buildings[b].fuel < amount) {
        log_msg("Station needs fuel. Camp ledger: add coal or timber.");
        return 0;
    }
    D.buildings[b].fuel -= amount;
    D.buildings[b].condition = fmaxf(1, D.buildings[b].condition - .1f);
    return 1;
}
int depth_organic(const Actor *a, int limb) {
    return a->race != 2 && !(a->prostheticMask & (1 << limb));
}
int depth_can_heal(const Actor *a, int limb) {
    return depth_organic(a, limb) && (!(a->lostMask & (1 << limb)) || (a->prostheticMask & (1 << limb)));
}
void depth_wound(Actor *a, int limb, float damage) {
    int n = member(a);
    if (n < 0)
        return;
    a->treatedMask &= ~(1 << limb);
    D.actors[n].trauma[limb] = fminf(80, D.actors[n].trauma[limb] + damage * .18f);
    D.actors[n].upkeep = fmaxf(20, D.actors[n].upkeep - damage * .025f);
}
int depth_needs_repair(const Actor *a) {
    int n = member(a);
    if (n < 0 || a->dead)
        return 0;
    for (int j = 0; j < LIMBS; j++)
        if (!limb_missing(a, j) && !depth_organic(a, j) &&
            (a->health[j] < limb_max_health(a, j) - 1 || D.actors[n].trauma[j] > 0))
            return 1;
    return 0;
}
int depth_repair(int mi, int pi) {
    if (mi < 0 || pi < 0 || mi >= g.actorCount || pi >= g.actorCount)
        return 0;
    Actor *m = &g.actors[mi], *a = &g.actors[pi];
    if (!conscious(m) || a->dead || depth_distance(m, a) > TALK_REACH)
        return 0;
    if (!depth_needs_repair(a)) {
        log_msg("No mechanical damage to repair.");
        return 0;
    }
    if (!inventory_remove(m, PARTS, 1)) {
        log_msg("Mechanical repairs need spare parts.");
        return 0;
    }
    for (int j = 0; j < LIMBS; j++)
        if (!limb_missing(a,j) && !depth_organic(a, j)) {
            D.actors[pi].trauma[j] = fmaxf(0, D.actors[pi].trauma[j] - 20);
            a->health[j] =
                fminf(limb_max_health(a,j) - D.actors[pi].trauma[j], a->health[j] + 30 + level(m, ENGINEERING) * .5f);
        }
    if (a->race == 2)
        a->bleed = 0;
    gain(m, ENGINEERING, 25);
    log_msg("Repaired mechanical limbs with spare parts.");
    return 1;
}
int depth_layer(int slot) {
    Actor *a = &g.actors[g.selected];
    DepthActor *z = &D.actors[g.selected];
    if (slot < 0 || slot >= PACK)
        return 0;
    int item = a->pack[slot].item;
    if (!item || itemDefs[item].slot != 1 || itemDefs[item].weight > 5) {
        log_msg("Underlayer needs light body clothing (weight 5 or less).");
        return 0;
    }
    Actor backup = *a;
    int owner = a->stolen[slot];
    inventory_remove_slot(a, slot, 1);
    if (z->layer && !inventory_add_owned(a, z->layer, 1, z->layerOwner)) {
        *a = backup;
        log_msg("Pack full. Layer swap cancelled.");
        return 0;
    }
    z->layer = item;
    z->layerOwner = owner;
    log_msg("Underlayer fitted. Inspect coverage in Armor Layers.");
    return 1;
}
int depth_coverage(int item, int limb) {
    if (item <= 0 || item >= ITEMS)
        return 0;
    int slot = itemDefs[item].slot;
    if (slot == 2)
        return limb == HEAD ? 85 : 0;
    if (slot == 1)
        return limb == CHEST ? 95 : limb == LARM || limb == RARM ? 65 : 0;
    if (slot == 4)
        return limb >= LLEG ? 90 : 0;
    if (slot == 3)
        return limb >= LLEG ? 35 : 0;
    return 0;
}
float depth_armor(const Actor *a, int limb, int weapon, int roll) {
    int n = member(a), family = weapon_family_index(weapon);
    int blunt = !weapon || weapon == STAFF || (family >= 14 && family <= 16) || family == 33;
    float armor = 0, condition = n >= 0 ? .65f + D.actors[n].upkeep * .0035f : 1;
    for (int j = 1; j < GEARS; j++)
        if (roll < depth_coverage(a->gear[j], limb))
            armor += itemDefs[a->gear[j]].armor * (blunt ? .72f : 1);
    if (n >= 0 && roll < depth_coverage(D.actors[n].layer, limb))
        armor += itemDefs[D.actors[n].layer].armor * (blunt ? .8f : .55f);
    return armor * condition;
}
int depth_capture_action(int n) {
    if (n < 0 || n >= g.actorCount)
        return 0;
    Actor *a = &g.actors[g.selected], *b = &g.actors[n];
    DepthActor *z = &D.actors[n];
    if (!conscious(a) || b->dead || depth_distance(a, b) > TALK_REACH)
        return 0;
    if (z->captive || b->prisonTime > 0)
        return society_rescue_start(n);
    if (!b->squad && g.squadCount < SQUAD) {
        b->squad = 1;
        b->kind = PLAYER;
        b->faction = FREE;
        b->job = JOB_FOLLOW;
        b->autoattack = 1;
        g.squadCount++;
        D.actors[n].group = D.actors[g.selected].group;
        log_msg("%s joins your expedition.", b->name);
        return 1;
    }
    log_msg("No room in the squad, or already recruited.");
    return 0;
}
static void captivity(float dt) {
    for (int i = 0; i < g.actorCount; i++) {
        Actor *a = &g.actors[i];
        DepthActor *z = &D.actors[i];
        if (!a->active || a->dead)
            continue;
        if (z->captive == 2) { /* physical capture, no teleport */
            if (a->carriedBy < 0) {
                z->captive = 0;
                continue;
            }
            Actor *captor = &g.actors[a->carriedBy];
            route_toward(captor, D.siteX[3], D.siteY[3], dt);
            if (distance(captor->x, captor->y, D.siteX[3], D.siteY[3]) < 1.6f) {
                captor->carrying = -1;
                a->carriedBy = -1;
                z->captive = 1;
                z->confiscated = a->gear[0];
                z->confiscatedOwner = a->gearStolen[0];
                a->gear[0] = 0;
                a->gearStolen[0] = 0;
                log_msg("%s is shackled at the chain yard.", a->name);
            }
        } else if (z->captive == 1) {
            a->attackPhase = 0;
            a->target = -1;
            a->taskKind = 0;
            a->bleed = 0;
            a->blood = fmaxf(35, a->blood);
            a->hunger = fmaxf(25, a->hunger);
            if (a->unconscious > 0) {
                a->health[HEAD] = fmaxf(5, a->health[HEAD]);
                a->health[CHEST] = fmaxf(5, a->health[CHEST]);
            }
        } else if (a->squad && a->unconscious > 0 && a->carriedBy < 0 && !actor_inside(a) &&
                   distance(a->x, a->y, D.siteX[3], D.siteY[3]) < 24) {
            for (int j = 0; j < g.actorCount; j++) {
                Actor *b = &g.actors[j];
                if (j != i && !b->squad && b->faction == HOSTILE && conscious(b) && b->carrying < 0 &&
                    depth_distance(a, b) < TALK_REACH) {
                    a->carriedBy = j;
                    b->carrying = i;
                    z->captive = 2;
                    a->bleed = 0;
                    log_msg("%s is being carried to the chain yard. Rescue them!", a->name);
                    break;
                }
            }
        }
    }
}
static int stock_has(int type, int item, const Actor *a) {
    for (int i = 0; i < g.buildingCount; i++) {
        Building *b = &g.buildings[i];
        if (b->type != type || b->owner < 0 || !depth_build_ready(i) ||
            distance(a->x, a->y, b->x, b->y) > 18)
            continue;
        if (!item)
            return 1;
        for (int j = 0; j < 16; j++)
            if (b->storage[j].item == item && b->storage[j].qty > 0)
                return b->storage[j].qty;
    }
    return 0;
}
static int ready_job(const Actor *a, int job) {
    if (job == JOB_HOLD || job == JOB_FOLLOW)
        return 1;
    if (job == JOB_MEDIC) {
        if (!inventory_count(a, MEDKIT))
            return 0;
        for (int j = 0; j < g.actorCount; j++)
            if (g.actors[j].squad && needs_aid(&g.actors[j]) && wound_needs_dressing(&g.actors[j]) &&
                depth_distance(a, &g.actors[j]) < 18)
                return 1;
        return 0;
    }
    if (job == JOB_MINE) {
        for (int j = 0; j < g.nodeCount; j++)
            if (g.nodes[j].remaining && g.nodes[j].type <= NODE_COPPER &&
                distance(a->x, a->y, g.nodes[j].x, g.nodes[j].y) < 12) {
                Actor test = *a;
                if (inventory_add(&test, g.nodes[j].type == NODE_ORE ? ORE : COPPER, 1))
                    return 1;
            }
        return 0;
    }
    if (job == JOB_FARM)
        return stock_has(FARM, 0, a);
    if (job == JOB_HAUL) {
        if (!stock_has(STORE, 0, a))
            return 0;
        return inventory_count(a, ORE) || inventory_count(a, COPPER) || inventory_count(a, WOOD) ||
               inventory_count(a, GRAIN) || inventory_count(a, IRONBAR);
    }
    int station = job == JOB_SMELT ? FORGE : CAMPFIRE;
    if (!stock_has(station, 0, a))
        return 0;
    if (stock_has(STORE, 0, a) &&
        inventory_count(a, job == JOB_SMELT ? IRONBAR : RATION) > (job == JOB_SMELT ? 0 : 2))
        return 1;
    int input = job == JOB_SMELT ? ORE : GRAIN;
    if (inventory_count(a, input) + stock_has(STORE, input, a) < 3 &&
        !(job == JOB_COOK && inventory_count(a, MEAT) + stock_has(STORE, MEAT, a) >= 2))
        return 0;
    return job == JOB_SMELT || inventory_count(a, WATER) + stock_has(STORE, WATER, a) > 0;
}
int assigned_job(const Actor *a) {
    int n = member(a);
    if (n < 0 || !D.actors[n].queued)
        return a->job;
    return ui.priorityJob[n] ? ui.priorityJob[n] - 1 : JOB_HOLD;
}
void depth_queue(Actor *a) {
    int n = member(a);
    if (n < 0 || !automatic_work(a) || !D.actors[n].queued || D.actors[n].paused || a->taskKind || a->target >= 0)
        return;
    ui.priorityJob[n] = JOB_HOLD + 1;
    for (int j = 0; j < 4; j++)
        if (ready_job(a, D.actors[n].queue[j])) {
            ui.priorityJob[n] = D.actors[n].queue[j] + 1;
            break;
        }
}
int depth_group_job(Actor *a, float dt) {
    int n = member(a);
    if (n < 0 || !a->squad)
        return 0;
    DepthActor *z = &D.actors[n];
    if (z->paused)
        return 1;
    if (assigned_job(a) != JOB_FOLLOW)
        return 0;
    Actor *lead = &g.actors[g.selected];
    if (a == lead || !conscious(lead))
        return 1;
    int room = depth_room(a), leadRoom = depth_room(lead);
    if (room && room == leadRoom) {
        if (depth_distance(a, lead) > 2.2f)
            depth_move_room(a, depth_x(lead) - depth_x(a), depth_y(lead) - depth_y(a), dt);
    } else if (!room && leadRoom) {
        Building *door = &g.buildings[leadRoom - 1];
        float x = door->x + 1.45f, y = door->y + .65f;
        if (distance(a->x, a->y, x, y) > 1.7f)
            route_toward(a, x, y, dt);
        else {
            z->room = leadRoom;
            z->rx = 2.6f + (n % 3) * .65f;
            z->ry = 5.8f;
        }
    } else if (!room && distance(a->x, a->y, lead->x, lead->y) > 2.4f + z->group * .35f) {
        route_toward(a, lead->x + (n % 3 - 1) * .75f, lead->y + .8f + z->group * .45f, dt);
    }
    return 1;
}
static void world_consequences(float dt) {
    int sora = D.npc[NPC_SORA], rell = D.npc[NPC_RELL];
    Actor *p = &g.actors[g.selected];
    if (D.rescued == 1 && alive(sora)) {
        Actor *a = &g.actors[sora];
        if (conscious(a) && !actor_inside(p) && distance(a->x, a->y, p->x, p->y) < 18 &&
            distance(a->x, a->y, p->x, p->y) > 2.1f)
            route_toward(a, p->x, p->y, dt);
        if (distance(a->x, a->y, D.siteX[7], D.siteY[7]) < 5) {
            if (a->carriedBy >= 0 && a->carriedBy < g.actorCount) {
                Actor *carrier = &g.actors[a->carriedBy];
                if (carrier->carrying == sora)
                    carrier->carrying = -1;
                a->carriedBy = -1;
            }
            D.rescued = 2;
            g.money += 100;
            log_msg("Sora is home. +100 silver. The culvert route is now known.");
        }
    }
    if (rell >= 0 && g.actors[rell].dead && !D.route)
        route_state(ROUTE_FREED);
    for (int i = 0; i < LANDMARKS; i++)
        if (distance(p->x, p->y, D.siteX[i], D.siteY[i]) < 12 && !(D.visited & (1 << i))) {
            D.visited |= 1 << i;
            gain(p, PERCEPTION, 20);
            log_msg("Discovered %s. Expedition ledger updated.", landmarkNames[i]);
        }
    if (D.lastDay != g.day) {
        D.lastDay = g.day;
        if (!D.route) {
            g.stock[1][RATION] = g.stock[1][RATION] > 4 ? 4 : g.stock[1][RATION];
            g.prosperity[1] = 30;
        } else {
            g.stock[1][RATION] = g.stock[1][RATION] > 999 ? 999 : g.stock[1][RATION] + 6;
            g.stock[1][MEDKIT] = g.stock[1][MEDKIT] > 995 ? 999 : g.stock[1][MEDKIT] + 4;
            g.prosperity[1] = D.route == ROUTE_TRUCE ? 48 : 68;
        }
    }
    if (D.route && alive(D.npc[NPC_CONVOY])) {
        Actor *a = &g.actors[D.npc[NPC_CONVOY]];
        int dest = D.convoyTrips % 2 ? SITE_STORES : SITE_CULVERT;
        route_toward(a, D.siteX[dest], D.siteY[dest], dt);
        if (distance(a->x, a->y, D.siteX[dest], D.siteY[dest]) < 2) {
            D.convoyTrips++;
            if (dest == SITE_STORES) {
                g.stock[1][RATION] = g.stock[1][RATION] > 995 ? 999 : g.stock[1][RATION] + 4;
                g.prosperity[1] = (int)fminf(85, g.prosperity[1] + 1);
            }
        }
    }
    if (D.route == ROUTE_CAPTURED || D.route == ROUTE_FREED) {
        D.retaliation -= dt;
        if (D.retaliation <= 0 && !D.retal) {
            D.retal = 1;
            for (int j = NPC_PATROL_A; j <= NPC_PATROL_B; j++)
                if (alive(D.npc[j])) {
                    Actor *a = &g.actors[D.npc[j]];
                    a->faction = HOSTILE;
                    a->homeX = D.siteX[7] - 14;
                    a->homeY = D.siteY[7];
                    a->tx = a->homeX;
                    a->ty = a->homeY;
                }
            snprintf(g.worldNews, sizeof(g.worldNews),
                     "Rell's remaining scouts are moving on Copper stores.");
            log_msg("%s", g.worldNews);
        }
    }
}
void depth_tick(float dt) {
    if (!D.version)
        return;
    world_consequences(dt);
    captivity(dt);
    for (int i = 0; i < g.actorCount; i++) {
        Actor *a = &g.actors[i];
        DepthActor *z = &D.actors[i];
        if (!a->active || a->dead)
            continue;
        z->recognized = fmaxf(0, z->recognized - dt);
        if (a->race == 2) {
            a->bleed = 0;
            a->blood = 100;
            a->hunger = 100;
        }
        if (a->squad)
            depth_queue(a);
        if (a->taskKind == 4) {
            int b = a->taskIndex;
            if (!conscious(a) || b < 0 || b >= g.buildingCount ||
                distance(a->x, a->y, g.buildings[b].x, g.buildings[b].y) > 2.5f) {
                a->taskKind = 0;
                continue;
            }
            float power = 1 + level(a, ENGINEERING) * .025f;
            D.buildings[b].labor = fmaxf(0, D.buildings[b].labor - dt * power);
            a->action = D.buildings[b].labor + 1;
            gain(a, ENGINEERING, dt * 3);
            if (!D.buildings[b].labor) {
                a->taskKind = 0;
                log_msg("Construction complete: %s.", context_name());
            }
        }
        for (int j = 0; j < LIMBS; j++)
            if (depth_organic(a, j) && a->taskKind == 3 && a->bleed <= 0 && a->hunger >= 20)
                z->trauma[j] = fmaxf(0, z->trauma[j] - dt * .12f);
    }
    depth_projectiles(dt);
}
void depth_after_tick(float dt) {
    for (int i = 0; i < g.actorCount; i++) {
        Actor *a = &g.actors[i];
        DepthActor *z = &D.actors[i];
        if (!a->active)
            continue;
        for (int j = 0; j < LIMBS; j++)
            a->health[j] = fminf(a->health[j], limb_max_health(a,j) - z->trauma[j]);
        /* Named NPCs retain authored positions; escorts and convoy move in
         * depth_tick. */
        int n = depth_named(i);
        if (n >= 0 && n <= NPC_TAMSIN && !a->squad && a->carriedBy < 0 && a->target < 0) {
            if (n == NPC_NELL || (n == NPC_SORA && D.rescued == 2)) {
                a->x = D.siteX[7] + (n == NPC_SORA ? 2 : 0);
                a->y = D.siteY[7];
            }
        }
        if (z->captive == 1 && n != NPC_RELL && !g.war.prisonTown[i]) {
            a->x = D.siteX[3] + (n == NPC_TAMSIN ? 1.2f : 0);
            a->y = D.siteY[3];
        }
    }
    depth_room_tick(dt);
}
int depth_input(float dt, Input in) {
    int p = in.pressed;
    Actor *a = &g.actors[g.selected];
    DepthActor *z = &D.actors[g.selected];
    if (ui.screen >= SCREEN_EXPEDITION && ui.screen <= SCREEN_LAYERS) {
        if (p & KEY_SELECT) {
            open_help(ui.screen == SCREEN_EXPEDITION || ui.screen == SCREEN_ENCOUNTER ? 20
                      : ui.screen == SCREEN_ORDERS                                    ? 21
                      : ui.screen == SCREEN_CAMP                                      ? 22
                      : ui.screen == SCREEN_CARE                                      ? 23
                                                                                      : 24);
            return 1;
        }
        if (p & (KEY_CIRCLE | KEY_START)) {
            depth_open(SCREEN_WORLD);
            return 1;
        }
        int count = ui.screen == SCREEN_EXPEDITION ? LANDMARKS
                    : ui.screen == SCREEN_ORDERS   ? 9
                    : ui.screen == SCREEN_CAMP     ? depth_camp_count()
                    : ui.screen == SCREEN_LAYERS   ? PACK
                    : ui.screen == SCREEN_CARE     ? LIMBS
                                                   : 2;
        if (count < 1)
            count = 1;
        if (p & KEY_DOWN)
            ui.selection = (ui.selection + 1) % count;
        if (p & KEY_UP)
            ui.selection = (ui.selection + count - 1) % count;
        if (ui.screen == SCREEN_EXPEDITION) {
            if (p & KEY_CROSS)
                depth_locate(ui.selection);
            if (p & KEY_SQUARE) {
                ui.depthSite = ui.selection;
                depth_open(SCREEN_ENCOUNTER);
            }
        } else if (ui.screen == SCREEN_ORDERS) {
            int delta = p & KEY_LEFT ? -1 : 1;
            if (p & (KEY_CROSS | KEY_LEFT | KEY_RIGHT))
                switch (ui.selection) {
                case 0:
                    z->order = (z->order + delta + 4) % 4;
                    a->autoattack = z->order != ORDER_HOLD_FIRE;
                    break;
                case 1:
                    z->group = (z->group + delta + 3) % 3;
                    break;
                case 2:
                    D.groupLeader[z->group] = g.selected;
                    log_msg("%s leads group %d.", a->name, z->group + 1);
                    break;
                case 3:
                    z->paused = !z->paused;
                    a->taskKind = 0;
                    log_msg(z->paused ? "Jobs paused. Manual movement is safe." : "Jobs resumed.");
                    break;
                case 8:
                    z->queued = !z->queued;
                    break;
                default:
                    z->queue[ui.selection - 4] = (z->queue[ui.selection - 4] + delta + 8) % 8;
                    break;
                }
        } else if (ui.screen == SCREEN_CAMP) {
            if (p & KEY_CROSS)
                depth_build_action(depth_camp_build(ui.selection));
            if (p & KEY_SQUARE)
                depth_fuel(depth_camp_build(ui.selection));
        } else if (ui.screen == SCREEN_CARE) {
            if (p & KEY_CROSS) {
                if (z->captive)
                    depth_capture_action(g.selected);
                else
                    heal_actor(g.selected, g.selected);
            }
            if (p & KEY_SQUARE)
                depth_repair(g.selected, g.selected);
            if (p & KEY_TRIANGLE) {
                if (z->upkeep >= 100)
                    log_msg("Equipment is already maintained.");
                else if (inventory_remove(a, CLOTH, 1)) {
                    z->upkeep = fminf(100, z->upkeep + 40);
                    log_msg("Clothing and armor maintained.");
                } else
                    log_msg("Maintenance needs one cloth.");
            }
        } else if (ui.screen == SCREEN_LAYERS) {
            if (p & KEY_CROSS)
                depth_layer(ui.selection);
            if ((p & KEY_SQUARE) && z->layer) {
                if (inventory_add_owned(a, z->layer, 1, z->layerOwner)) {
                    z->layer = 0;
                    z->layerOwner = 0;
                } else
                    log_msg("Pack full. Cannot remove layer.");
            }
        } else if (p & KEY_CROSS)
            depth_action(ui.depthSite, ui.selection);
        return 1;
    }
    if (ui.screen == SCREEN_DIALOG && depth_named(ui.dialogActor) >= 0 &&
        depth_named(ui.dialogActor) <= NPC_TAMSIN) {
        int count = dialogue_count(ui.dialogActor);
        if (p & KEY_SELECT) {
            open_help(20);
            return 1;
        }
        if (p & (KEY_CIRCLE | KEY_START)) {
            depth_open(SCREEN_WORLD);
            return 1;
        }
        if (p & KEY_DOWN)
            ui.selection = (ui.selection + 1) % count;
        if (p & KEY_UP)
            ui.selection = (ui.selection + count - 1) % count;
        if (p & KEY_CROSS)
            depth_dialog(ui.dialogActor, ui.selection);
        return 1;
    }
    if (ui.screen == SCREEN_WORLD && z->captive) {
        if (p & KEY_CROSS) {
            depth_open(SCREEN_CARE);
            return 1;
        }
        if (!(p & (KEY_L | KEY_R | KEY_START | KEY_SELECT | KEY_TRIANGLE))) {
            if (!ui.paused)
                simulate(dt * ui.speed);
            return 1;
        }
    }
    if (ui.screen == SCREEN_WORLD && fabsf(in.ax) + fabsf(in.ay) > .15f) {
        if (z->queued && !z->paused) {
            z->paused = 1;
            log_msg("Job queue paused by movement. Resume in Orders.");
        }
        D.groupTravel[z->group] = 0;
    }
    return 0;
}
