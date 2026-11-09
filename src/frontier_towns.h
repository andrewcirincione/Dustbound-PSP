/* Settlement plans are keyed by the original town, so occupation changes the flag
   without rebuilding the native architecture or replacing a town's services. */
static const TownDesign townDesigns[TOWNS] = {
    /* inn, market, beds, forge, custody, market tier, plan, turn */
    {"Copper Lantern", "Dusk Provisions",   3, 1, 1, 1, 0, 0},
    {"Sprocket Rest",   "Crossing Supplies", 2, 1, 0, 0, 1, 0},
    {"Brine House",     "Salt Exchange",     2, 0, 0, 0, 2, 1},
    {"Canvas & Salt",   "Dry Market Bazaar", 5, 0, 1, 1, 3, 0},
    {"Riven Shelter",   "Spire Scavengers",  1, 0, 0, 0, 5, 2},
    {"The Split Rock",  "Gulch Outfitters",  3, 1, 0, 0, 4, 0},
    {"Three Shields",   "Iron Gate Supply", 4, 1, 2, 2, 2, 0},
    {"Ash Pilgrim",     "Ash Herb Room",     4, 0, 1, 1, 5, 1},
    {"Smokestack",      "Mesa Fence",        3, 1, 1, 2, 4, 1},
    {"Reed & Rope",     "Reed Provisioner",  2, 0, 0, 0, 1, 3},
    {"The Drift Net",   "Cinder Exchange",   5, 1, 2, 2, 0, 3},
    {"The Red Banner",  "Rest Quartermaster",2, 1, 2, 1, 2, 2},
    {"Snowmelt Lodge",  "Pass Waystation",   2, 0, 0, 0, 5, 3},
    {"Raft House",      "Ferry Freight",     3, 0, 1, 0, 1, 2},
    {"Vulture Roost",   "Nest Salvage",      1, 0, 0, 0, 4, 3},
    {"Pine Sanctuary",  "Pine Pantry",       3, 0, 1, 0, 5, 0},
    {"Pump House Inn",  "Rail Exchange",     6, 1, 2, 2, 3, 3},
    {"Court Inn",       "Iron Tribute Hall", 5, 1, 2, 2, 2, 3},
    {"Pilgrim Hospice", "Herbal Dispensary", 6, 0, 1, 2, 5, 2},
};
const TownDesign *town_design(int town) {
    return &townDesigns[town >= 0 && town < TOWNS ? town : 0];
}
int building_town(int id) {
    const Building *b = &g.buildings[id];
    if (b->owner >= 0 || g.frontier.variant[id] == BUILD_LODGE)
        return -1;
    return town_near(b->x, b->y, 24);
}
const char *building_name(int id) {
    const Building *b = &g.buildings[id];
    int t = building_town(id), v = g.frontier.variant[id];
    const TownDesign *plan = town_design(t);
    if (v == BUILD_LODGE) return "Dustward Lodge";
    if (v == BUILD_ARMORER) return "Armorer";
    if (b->type == BAR && t >= 0) return plan->inn;
    if (b->type == SHOP && t >= 0) return plan->market;
    if (b->type == JAIL)
        return plan->custody == 0 ? "Watch cages" : plan->custody == 2 ? "Town stockade" : "Jail";
    static const char *labels[] = {
        "Frontier home", "Trading post", "Inn", "Watch house", "Windpump", "Well", "Campfire",
        "Bedroll", "Crop field", "Smithy", "Research bench", "Palisade", "Depot", "Jail", "Slave camp"
    };
    return labels[b->type];
}
int building_sprite(int id) {
    const Building *b = &g.buildings[id];
    int town = building_town(id), style = town < 0 ? -1 : civil_style(town);
    const TownDesign *plan = town_design(town);
    if (g.frontier.variant[id] == BUILD_LODGE) return ART_DUSTWARD;
    if (g.frontier.variant[id] == BUILD_ARMORER) return ART_ARMORER;
    static const int base[] = {0, 1, 2, 3, 24, 25, 27, 26, 28, 6, 29, 30, 7, 3, 31};
    switch (b->type) {
    case HOUSE:
        if (town == 9 || town == 13 || town == 15) return id % 3 ? ART_REED_HOME : ART_ASH_HOME;
        if (town == 12) return id % 2 ? ART_SNOW_LODGE : ART_ASH_HOME;
        if (style == 1) return id % 3 ? 4 : 0;
        if (style == 2) return id % 2 ? ART_ASH_HOME : ART_HERB_SHOP;
        if (style == 3 || style == 4) return id % 3 ? ART_RUST_HOME : 0;
        if (style == 5) return id % 2 ? 5 : ART_RUST_HOME;
        return id % 3 == 0 ? ART_RUST_HOME : 0;
    case SHOP:
        if (style == 2) return ART_HERB_SHOP;
        if (town == 3 || town == 8 || town == 16) return ART_DRY_SHOP;
        return 1;
    case BAR:
        if (town == 12) return ART_SNOW_LODGE;
        if (town == 9 || town == 13) return ART_REED_HOME;
        if (style == 1) return plan->beds >= 4 ? ART_COURT_INN : 2;
        if (style == 2) return plan->beds >= 4 ? ART_ASH_HOME : ART_SNOW_LODGE;
        if (town == 3 || town == 16) return ART_DRY_INN;
        return plan->beds <= 3 ? ART_ROAD_INN : 2;
    case JAIL:
        return plan->custody == 0 ? ART_WATCH_CAGE : plan->custody == 2 ? ART_STOCKADE : 3;
    default:
        return base[b->type];
    }
}
PaintedDoor painted_door(int sprite) {
    static const PaintedDoor original[] = {
        {.39f,.87f,0}, {.45f,.89f,0}, {.38f,.90f,0}, {.80f,.85f,1},
        {.42f,.88f,0}, {.48f,.90f,0}, {.53f,.91f,0}, {.75f,.87f,1}
    };
    static const PaintedDoor generated[] = {
        {.71f,.89f,1}, {.78f,.88f,1}, {.81f,.84f,1}, {.73f,.88f,1},
        {.81f,.84f,1}, {.67f,.89f,1}, {.78f,.88f,1}, {.81f,.87f,1},
        {.82f,.84f,1}, {.77f,.91f,1}, {.73f,.87f,1}, {.66f,.89f,1}
    };
    if (sprite < 8) return original[sprite];
    if (sprite == ART_WATCH_CAGE) return (PaintedDoor){.81f,.88f,1};
    return generated[sprite - SETTLEMENT_ART_FIRST];
}
void building_footprint(int id, float *w, float *d, int *height) {
    const Building *b = &g.buildings[id];
    int t = building_town(id);
    const TownDesign *plan = town_design(t);
    unsigned v = (unsigned)id + (unsigned)(t + 1) * 11;
    *w = 1.05f + (v % 3) * .16f;
    *d = .83f + (v / 3 % 3) * .17f;
    *height = 29 + 5 * (v % 3);
    switch (b->type) {
    case SHOP:
        *w = 1.25f + plan->marketTier * .24f;
        *d = 1.03f + (v % 3) * .17f;
        *height = 30 + (v % 3) * 5;
        break;
    case BAR:
        *w = 1.0f + plan->beds * .17f;
        *d = .9f + plan->beds * .14f;
        *height = 29 + plan->beds * 4;
        break;
    case TOWER:
        *w = .83f; *d = .81f; *height = 46 + (v % 3) * 7;
        break;
    case FORGE:
        *w = 1.22f + (v % 2) * .29f; *d = 1.24f; *height = 31 + (v % 3) * 4;
        break;
    case STORE:
        *w = 1.38f; *d = 1.20f + (v % 3) * .22f; *height = 29 + (v % 2) * 8;
        break;
    case JAIL:
        *w = 1.26f + plan->custody * .35f; *d = 1.10f + plan->custody * .39f;
        *height = plan->custody == 0 ? 24 : 36 + plan->custody * 6;
        break;
    }
    if (g.frontier.variant[id] == BUILD_LODGE) {
        *w = 1.48f; *d = 1.45f; *height = 39;
    }
}
static void door_offset(int id, float *x, float *y, int *rightWall) {
    float w, d;
    int height;
    building_footprint(id, &w, &d, &height);
    PaintedDoor door = painted_door(building_sprite(id));
    float projected = (door.x - .5f) * (w + d) * 2.1f;
    *rightWall = door.rightWall;
    *x = door.rightWall ? w : fmaxf(-.76f*w, fminf(.76f*w, d + projected));
    *y = door.rightWall ? fmaxf(-.76f*d, fminf(.76f*d, w - projected)) : d;
}
void building_threshold(int id, float *x, float *y) {
    int rightWall;
    door_offset(id, x, y, &rightWall);
    *x += g.buildings[id].x;
    *y += g.buildings[id].y;
}
void building_door(int id, float *x, float *y) {
    int rightWall;
    door_offset(id, x, y, &rightWall);
    *x += g.buildings[id].x + (rightWall ? .34f : 0);
    *y += g.buildings[id].y + (rightWall ? 0 : .34f);
}
float building_door_distance(int id, float x, float y) {
    float bx, by;
    building_door(id, &bx, &by);
    return distance(x, y, bx, by);
}

/* In order: market, inn, smith, lockup, depot, well, bench, watch, first home. */
static const signed char streetPlans[6][9][2] = {
    {{-4,-5},{5,0},{-5,5},{4,-7},{9,5},{0,0},{-9,0},{0,-11},{-4,10}},
    {{-5,-1},{3,7},{7,-6},{2,-5},{-8,6},{0,1},{-9,-7},{10,2},{-4,-8}},
    {{-6,3},{6,2},{-6,-6},{6,-6},{0,-9},{0,2},{-10,8},{0,-3},{8,9}},
    {{3,-5},{-6,4},{8,4},{-6,-6},{1,9},{0,0},{-11,1},{9,-6},{5,7}},
    {{-7,-5},{-2,5},{5,5},{9,-1},{-7,5},{0,-2},{4,-8},{-11,0},{9,9}},
    {{5,1},{-2,-7},{7,7},{-7,0},{-7,8},{0,1},{4,-7},{-10,-5},{1,9}},
};
static int street_service(int type) {
    switch (type) {
    case SHOP: return 0;
    case BAR: return 1;
    case FORGE: return 2;
    case JAIL: return 3;
    case STORE: return 4;
    case WELL: return 5;
    case BENCH: return 6;
    case TOWER: return 7;
    case HOUSE: return 8;
    default: return -1;
    }
}
static int structure_site(int id, float x, float y, const unsigned char *pending) {
    float w = .85f, d = .85f;
    int height = 0, solid = building_has_interior(g.buildings[id].type);
    if (solid) building_footprint(id, &w, &d, &height);
    float dx = 0, dy = 0;
    int side = 0;
    if (solid) door_offset(id, &dx, &dy, &side);
    if (terrain_water(x, y) || terrain_water(x + dx + (side ? .34f : 0), y + dy + (side ? 0 : .34f)) || trail_blocked(x, y))
        return 0;
    for (int j = 0; j < g.buildingCount; j++) {
        const Building *b = &g.buildings[j];
        if (j == id || (pending && pending[j]) || portable_bedroll(j) < 0 ||
            fabsf(b->x - x) > 7 || fabsf(b->y - y) > 7)
            continue;
        float bw = .85f, bd = .85f;
        int h, bsolid = building_has_interior(b->type);
        if (bsolid) building_footprint(j, &bw, &bd, &h);
        if (fabsf(b->x - x) < w + bw + .82f && fabsf(b->y - y) < d + bd + .82f)
            return 0;
        if (solid && bsolid &&
            fabsf((b->x - x) - (b->y - y)) < (w + d + bw + bd) * .63f &&
            fabsf((b->x - x) + (b->y - y)) < 3.2f)
            return 0;
    }
    return 1;
}
static void site_building(int id, float x, float y, const unsigned char *pending) {
    for (int ring = 0; ring <= 9; ring++) {
        int steps = ring ? 12 : 1;
        for (int i = 0; i < steps; i++) {
            float angle = i * 6.2831853f / steps + id * .31f;
            float bx = x + cosf(angle) * ring * 1.05f, by = y + sinf(angle) * ring * 1.05f;
            if (structure_site(id, bx, by, pending)) {
                g.buildings[id].x = bx;
                g.buildings[id].y = by;
                return;
            }
        }
    }
}
static void settlement_rebuild(void) {
    unsigned char pending[MAX_BUILDINGS] = {0};
    signed char district[MAX_BUILDINGS];
    int original = g.buildingCount;
    memset(district, -1, sizeof(district));
    for (int i = 0; i < original; i++) {
        Building *b = &g.buildings[i];
        if (b->owner >= 0 || b->type == WALL || b->type == SLAVE_YARD || portable_bedroll(i) != 0)
            continue;
        int t = town_near(b->x, b->y, 19);
        if (t < 0) continue;
        if (b->type == BED && t != 7 && t != 12 && t != 18 && !trail_bed(i)) {
            int occupied = 0;
            for (int n = 0; n < g.actorCount; n++)
                if (g.actors[n].taskKind == 3 && g.actors[n].taskIndex == i)
                    occupied = 1;
            if (!occupied) {
                g.field.rolls[i] = -1;
                continue;
            }
        }
        int landmark = 0;
        for (int k = 0; k < LANDMARKS; k++)
            if (g.depth.siteBuilding[k] == i) landmark = 1;
        if (landmark) continue;
        district[i] = t;
        pending[i] = 1;
    }
    for (int t = 0; t < TOWNS; t++) {
        int counts[SLAVE_YARD + 1] = {0};
        const TownDesign *plan = town_design(t);
        for (int i = 0; i < original; i++) {
            if (district[i] != t) continue;
            Building *b = &g.buildings[i];
            if (b->type == FORGE && !plan->smith)
                b->type = HOUSE;
            int seq = counts[b->type]++, service = street_service(b->type);
            float x, y;
            if (service >= 0 && !seq) {
                x = streetPlans[plan->plan][service][0];
                y = streetPlans[plan->plan][service][1];
            } else {
                float angle = (i * 2.39996f + t * .78f);
                float radius = (b->type == BED ? 7.8f : 10.6f) + seq % 3 * 1.8f;
                x = cosf(angle) * radius;
                y = sinf(angle) * radius;
            }
            if (b->type == WELL && t % 3 != 0) {
                x += t % 2 ? 5 : -5;
                y += t % 3 == 1 ? -2 : 4;
            }
            for (int spin = 0; spin < plan->turn; spin++) {
                float swap = x;
                x = -y;
                y = swap;
            }
            pending[i] = 0;
            site_building(i, towns[t].x + x, towns[t].y + y, pending);
        }
        if (plan->marketTier == 2 && t != 8 && t != 18) {
            int b = add_build(towns[t].x, towns[t].y, SHOP, g.townOwner[t], -1);
            if (b >= 0) {
                g.frontier.variant[b] = BUILD_ARMORER;
                site_building(b, towns[t].x - 8, towns[t].y + 9, pending);
            }
        }
    }
    for (int n = 0; n < g.actorCount; n++) {
        Actor *a = &g.actors[n];
        int t = a->homeTown, room = depth_room(a);
        if (room) {
            building_door(room - 1, &a->x, &a->y);
            continue;
        }
        if (a->squad || t < 0 || t >= TOWNS || a->dead || a->prisonTime > 0 ||
            population_owned(n) || regional_named(n) || civil_elite_index(n) >= 0 ||
            living_owned(n) || merc_slot(a) >= 0 || g.war.prisonTown[n]) {
            if (a->squad && !can_walk(a->x, a->y)) civil_clear_position(&a->x, &a->y);
            continue;
        }
        int role = npc_role(a), type = role == ROLE_MERCHANT ? SHOP : role == ROLE_INNKEEPER ? BAR :
            role == ROLE_SMITH ? FORGE : a->kind == GUARD && g.field.jail[t] ? JAIL : -1;
        int best = -1;
        for (int i = 0; i < g.buildingCount; i++)
            if (g.buildings[i].type == type && building_town(i) == t && !g.frontier.variant[i]) {
                best = i;
                break;
            }
        if (best >= 0) {
            if (type == JAIL) {
                int already = 0;
                for (int j = 0; j < n; j++)
                    if (g.actors[j].homeTown == t && !strcmp(g.actors[j].name, "Jailkeeper")) already = 1;
                if (already) continue;
                snprintf(a->name, sizeof(a->name), "Jailkeeper");
            }
            building_door(best, &a->homeX, &a->homeY);
            if (distance(a->x, a->y, towns[t].x, towns[t].y) < 22 && !a->prisonTime) {
                a->x = a->tx = a->homeX;
                a->y = a->ty = a->homeY;
                a->routeLen = a->routePos = 0;
            }
        } else if (role == ROLE_SMITH && !town_design(t)->smith) {
            a->voice = ROLE_CITIZEN;
            snprintf(a->name, sizeof(a->name), "Town resident");
        }
    }
    for (int n = 0; n < g.actorCount; n++) {
        Actor *a = &g.actors[n];
        if (!a->active || a->dead || depth_room(a) || can_walk(a->x,a->y))
            continue;
        civil_clear_position(&a->x,&a->y);
        if (!a->squad) {
            a->homeX = a->tx = a->x;
            a->homeY = a->ty = a->y;
        }
    }
}
static void frontier_resources(void) {
    /* Seams are outside town limits, a short detour from each approach. */
    for (int i = 0; i < g.nodeCount; i++) {
        Node *n = &g.nodes[i];
        if (n->type <= NODE_COPPER && town_near(n->x,n->y,18) >= 0) {
            n->regen = -1;
            n->remaining = 0;
        }
    }
    for (int t = 0; t < TOWNS; t++) {
        for (int k = 0; k < 3; k++) {
            int type = k == 0 ? NODE_ORE : k == 1 ? NODE_COPPER : NODE_WOOD;
            float x = towns[t].x, y = towns[t].y;
            for (int i = 0; i < 24; i++) {
                float angle = (t * 2.39996f + k * 1.66f) + i * .262f;
                float radius = 21.5f + (t + k) % 4 + i / 8 * 1.5f;
                float nx = x + cosf(angle) * radius, ny = y + sinf(angle) * radius;
                if (town_near(nx, ny, 18) >= 0 || terrain_road(nx, ny) || !can_walk(nx, ny) ||
                    !can_walk(nx + .5f, ny) || !can_walk(nx, ny + .5f))
                    continue;
                int slot = -1, near = 0;
                for (int n = 0; n < g.nodeCount; n++) {
                    Node *node = &g.nodes[n];
                    if (node->regen < 0 && slot < 0) slot = n;
                    if (node->regen >= 0 && fabsf(node->x - nx) < 4 && fabsf(node->y - ny) < 4) near = 1;
                }
                if (near) continue;
                if (slot >= 0)
                    g.nodes[slot] = (Node){nx, ny, type, 20, 0, FREE, NONE, 0, 0};
                else
                    add_node(nx, ny, type, FREE, NONE, 0);
                break;
            }
        }
    }
}
static void lodge_sites(void) {
    static const int hubs[LODGES] = {0, 6, 10};
    for (int i = 0; i < LODGES; i++) {
        const Town *town = &towns[hubs[i]];
        for (int k = 0; k < 24; k++) {
            float angle = (i * 2.39996f + 1.1f) + k * .262f;
            float x = town->x + cosf(angle) * (34 + k / 8 * 2), y = town->y + sinf(angle) * (34 + k / 8 * 2);
            if (!clear_structure_site(x, y, 6) || terrain_road(x, y) || town_near(x, y, 25) >= 0)
                continue;
            int id = add_build(x, y, HOUSE, FREE, -1);
            if (id >= 0) {
                g.frontier.lodge[i] = id + 1;
                g.frontier.variant[id] = BUILD_LODGE;
            }
            break;
        }
    }
}
int starter_country(float x, float y) {
    static const int origins[] = {0, 1, 3, 5};
    for (unsigned i = 0; i < sizeof(origins)/sizeof(origins[0]); i++) {
        const Town *t = &towns[origins[i]];
        if (fabsf(x-t->x) < 115 && fabsf(y-t->y) < 115 && distance(x,y,t->x,t->y) < 115)
            return 1;
    }
    return 0;
}
static void starter_bandits(void) {
    for (int n = 0; n < g.actorCount; n++) {
        Actor *a = &g.actors[n];
        if (a->squad || !a->active || a->dead || (a->kind != RAIDER && a->kind != BEAST) ||
            a->prisonTime > 0 || civil_elite_index(n) >= 0 || !starter_country(a->x, a->y))
            continue;
        for (int s = 0; s < SKILLS; s++) a->xp[s] = fminf(a->xp[s], 12 * 12 * 12);
        if (a->kind == RAIDER && itemDefs[a->gear[0]].power > 27) a->gear[0] = n % 2 ? SABRE : RUSTSWORD;
        if (a->kind == RAIDER && itemDefs[a->gear[1]].armor > 10) a->gear[1] = RAGS;
    }
}
static int clear_loot_site(float x, float y, int node) {
    if (!can_walk(x, y)) return 0;
    for (int i = 0; i < g.buildingCount; i++) {
        const Building *b = &g.buildings[i];
        if (!building_has_interior(b->type) || fabsf(b->x-x) > 4 || fabsf(b->y-y) > 4)
            continue;
        float w, d;
        int height;
        building_footprint(i, &w, &d, &height);
        if (fabsf(b->x-x) < w + .95f && fabsf(b->y-y) < d + .95f)
            return 0;
        if (building_door_distance(i, x, y) < 1.3f)
            return 0;
    }
    for (int i = 0; i < g.nodeCount; i++)
        if (i != node && g.nodes[i].remaining && fabsf(g.nodes[i].x-x) < .85f && fabsf(g.nodes[i].y-y) < .85f)
            return 0;
    return 1;
}
static void move_overlapping_loot(void) {
    for (int i = 0; i < g.nodeCount; i++) {
        Node *n = &g.nodes[i];
        if (!n->remaining || n->type < NODE_LOOT || town_near(n->x, n->y, 22) < 0 ||
            clear_loot_site(n->x, n->y, i))
            continue;
        int moved = 0;
        for (int ring = 1; ring <= 12 && !moved; ring++)
            for (int j = 0; j < 16; j++) {
                float angle = j * .39269908f;
                float x = n->x + cosf(angle) * ring * .65f, y = n->y + sinf(angle) * ring * .65f;
                if (clear_loot_site(x, y, i)) {
                    n->x = x;
                    n->y = y;
                    moved = 1;
                    break;
                }
            }
    }
}
static void place_roadside_ore(void) {
    for (int t = 0; t < TOWNS; t++) {
        const Town *town = &towns[t];
        int node = nearest_ore(town->x,town->y,32,NODE_ORE);
        if (node < 0) continue;
        float best = 1e9f, ox = g.nodes[node].x, oy = g.nodes[node].y;
        for (int i = 0; i < 64; i++) {
            float angle = i * .09817477f, dx = cosf(angle), dy = sinf(angle);
            for (int radius = 19; radius <= 25; radius += 2) {
                float roadX = town->x + dx*radius, roadY = town->y + dy*radius;
                if (!terrain_road(roadX,roadY) || !can_walk(roadX,roadY))
                    continue;
                for (int side = -1; side <= 1; side += 2) {
                    float x = roadX - dy*side*3.4f, y = roadY + dx*side*3.4f;
                    float score = distance(x,y,town->x,town->y);
                    if (score >= best || town_near(x,y,18.5f) >= 0 || terrain_road(x,y) ||
                        !clear_loot_site(x,y,node) || !can_walk((roadX+x)*.5f,(roadY+y)*.5f))
                        continue;
                    best = score;
                    ox = x;
                    oy = y;
                }
            }
        }
        g.nodes[node].x = ox;
        g.nodes[node].y = oy;
    }
}
void frontier_world_init(void) {
    if (!g.frontier.version) {
        settlement_rebuild();
        frontier_resources();
        lodge_sites();
        starter_bandits();
        for (int i = 0; i < CONTRACTS; i++)
            if (g.contracts[i].active == 2)
                remember_contract(&g.contracts[i]);
        restock();
    }
    if (g.frontier.version < 2) {
        move_overlapping_loot();
        place_roadside_ore();
        for (int n = 0; n < g.actorCount; n++) {
            Actor *a = &g.actors[n];
            int room = depth_room(a);
            if (room) building_door(room - 1, &a->x, &a->y);
        }
        g.frontier.version = 2;
    }
}
