/* Included by render.c: shares the bounded PSP software-renderer primitives. */
static void depth_world_draw(void) {
    for (int i = 0; !g.interiorBuilding && i < LANDMARKS; i++) {
        int x, y;
        project(g.depth.siteX[i], g.depth.siteY[i], &x, &y);
        if (x < 10 || x > 470 || y < 48 || y > 175)
            continue;
        int nearby = distance(g.actors[g.selected].x, g.actors[g.selected].y, g.depth.siteX[i],
                              g.depth.siteY[i]) < 5;
        if (nearby) {
            diamond(x, y, 8, 4, GOLD);
            if (ui.screen == SCREEN_WORLD && ui.contextKind == 4 && ui.contextIndex == i)
                center(x, y - 59, landmarkNames[i], GOLD, 1);
        }
    }
    for (int i = 0; i < PROJECTILES; i++) {
        Projectile *p = &g.depth.shots[i];
        if (!p->active || p->room != g.interiorBuilding)
            continue;
        int x, y, xx, yy;
        if (p->room) {
            room_project(p->x, p->y, &x, &y);
            room_project(p->x - p->vx * .035f, p->y - p->vy * .035f, &xx, &yy);
        } else {
            project(p->x, p->y, &x, &y);
            project(p->x - p->vx * .035f, p->y - p->vy * .035f, &xx, &yy);
        }
        line(xx, yy - 21, x, y - 21, 0xc7b88c);
    }
}
static void depth_header(const char *name, const char *sub) {
    overlay_header(name, sub);
    rect(25, 233, 425, 15, PANEL);
    text(31, 237, "\002 CLOSE   \010 GUIDE", MUTED, 1);
}
static void depth_screen(void) {
    Actor *a = &g.actors[g.selected];
    DepthActor *z = &g.depth.actors[g.selected];
    char buf[140];
    if (ui.screen == SCREEN_EXPEDITION) {
        depth_header("THE COPPER ROAD", "\015 LEAD  \001 LOCATE  \003 DETAILS  \010 GUIDE");
        clipped(31, 84, depth_objective(), 68, TEXT);
        int first = start_row(ui.selection, LANDMARKS, 8);
        for (int j = 0; j < 8; j++) {
            int i = first + j;
            snprintf(buf, sizeof(buf), "%s / %.0fm", g.depth.visited & (1 << i) ? "SEEN" : "RUMOR",
                     distance(a->x, a->y, g.depth.siteX[i], g.depth.siteY[i]));
            row(107 + j * 14, ui.selection == i, landmarkNames[i], buf);
        }
        clipped(31, 226, depth_outcome(), 68, TEAL);
    } else if (ui.screen == SCREEN_ORDERS) {
        depth_header("SQUAD ORDERS", "\015 ROW  \014 CHANGE  \001 SET  \010 GUIDE");
        const char *names[] = {"Combat role",      "Expedition group", "Make this survivor leader",
                               "Job queue status", "Priority 1",       "Priority 2",
                               "Priority 3",       "Priority 4",       "Use priority queue"};
        for (int i = 0; i < 9; i++) {
            const char *v = "";
            if (i == 0)
                v = orderNames[z->order];
            if (i == 1) {
                snprintf(buf, sizeof(buf), "%d / 3", z->group + 1);
                v = buf;
            }
            if (i == 2)
                v = g.depth.groupLeader[z->group] == g.selected ? "LEADER" : "SET";
            if (i == 3)
                v = z->paused ? "PAUSED" : "ACTIVE";
            if (i >= 4 && i < 8)
                v = jobNames[z->queue[i - 4]];
            if (i == 8)
                v = z->queued ? "ON" : "OFF";
            row(86 + i * 16, ui.selection == i, names[i], v);
        }
        text(31, 225, "ATLAS: HOLD \006 + \001 TO ROUTE THIS GROUP", TEAL, 1);
    } else if (ui.screen == SCREEN_CAMP) {
        depth_header("CAMP WORK & FUEL", "\015 STRUCTURE  \001 BUILD/REPAIR  \003 FUEL  \010 GUIDE");
        int total = depth_camp_count();
        int visible = total < 8 ? total : 8, first = start_row(ui.selection, total, visible);
        for (int j = 0; j < visible; j++) {
            int selectedRow = first + j;
            int i = depth_camp_build(selectedRow);
            Building *b = &g.buildings[i];
            DepthBuilding *s = &g.depth.buildings[i];
            snprintf(buf, sizeof(buf), "%s %.0fm",
                     b->type == CAMPFIRE ? "CAMPFIRE"
                     : b->type == FORGE  ? "FORGE"
                     : b->type == BED    ? "BED"
                     : b->type == STORE  ? "DEPOT"
                     : b->type == FARM   ? "FARM"
                     : b->type == BENCH  ? "BENCH"
                                         : "STRUCTURE",
                     distance(a->x, a->y, b->x, b->y));
            char val[70];
            snprintf(val, sizeof(val), "BUILD %.0f  FUEL %.0f  HP %.0f", s->labor, s->fuel,
                     s->condition);
            row(87 + j * 16, ui.selection == selectedRow, buf, val);
        }
        if (!total)
            wrap(31, 100, 60,
                 "No owned structures yet. Build a camp outside town from the expedition menu.", TEXT);
        text(31, 225, "TIMBER: 4 CHARGES / COAL: 10 / WORK WITHIN REACH", GOLD, 1);
    } else if (ui.screen == SCREEN_CARE) {
        depth_header(z->captive ? "SHACKLED / RECOVERY" : "TREATMENT & RECOVERY",
                     "\001 BANDAGE/ESCAPE  \003 REPAIR  \004 MAINTAIN  \010 GUIDE");
        snprintf(buf, sizeof(buf), "BLOOD %.0f  BLEED %.1f  ARMOR CONDITION %.0f%%", a->blood, a->bleed,
                 z->upkeep);
        text(31, 86, buf, a->bleed > 0 ? RED : TEAL, 1);
        for (int i = 0; i < LIMBS; i++) {
            snprintf(buf, sizeof(buf), "%s  HP %.0f  TRAUMA %.0f",
                     depth_organic(a, i) ? "FLESH" : "MECH", a->health[i], z->trauma[i]);
            row(109 + i * 16, ui.selection == i, limbNames[i], buf);
        }
        wrap(31, 211, 68,
             z->captive ? "Time passes outside this screen. Work the shackles away "
                          "from guards. A free companion can help."
                        : "Bandages stop bleeding. Rest heals trauma slowly. "
                          "Mechanical limbs require parts, not medicine.",
             MUTED);
    } else if (ui.screen == SCREEN_LAYERS) {
        depth_header("ARMOR LAYERS", "\015 ITEM  \001 FIT UNDERLAYER  \003 REMOVE  \010 GUIDE");
        snprintf(buf, sizeof(buf), "UNDER: %.48s", z->layer ? itemDefs[z->layer].name : "NONE");
        text(31, 85, buf, TEAL, 1);
        int first = start_row(ui.selection, PACK, 4);
        for (int j = 0; j < 4; j++) {
            int i = first + j, it = a->pack[i].item;
            char value[40];
            snprintf(value, sizeof(value), "WT %d / ARM %d", itemDefs[it].weight, itemDefs[it].armor);
            row(103 + j * 16, ui.selection == i, it ? itemDefs[it].name : "EMPTY", it ? value : "");
        }
        int candidate = a->pack[ui.selection].item, limb = CHEST;
        if (candidate && itemDefs[candidate].slot == 1 && itemDefs[candidate].weight <= 5)
            snprintf(buf, sizeof(buf), "CHEST COVERAGE %d%% > %d%% / CUT ARMOR %.1f > %.1f",
                     depth_coverage(z->layer, limb), depth_coverage(candidate, limb),
                     itemDefs[z->layer].armor * .55f, itemDefs[candidate].armor * .55f);
        else
            snprintf(buf, sizeof(buf), "CHOOSE LIGHT BODY CLOTHING / \003 REMOVES WORN LAYER");
        clipped(31, 178, buf, 68, GOLD);
        text(31, 195, "CLOTH ONLY: BODY SLOT, WEIGHT 5 OR LESS.", MUTED, 1);
        text(31, 210, "COVERAGE: BODY 95%, ARMS 65%, HEAD 85%.", MUTED, 1);
        text(31, 225, "BLUNT IMPACT REDUCES OUTER ARMOR TO 72%.", MUTED, 1);
    } else {
        int site = ui.depthSite;
        if (site < 0 || site >= LANDMARKS)
            site = 0;
        depth_header(landmarkNames[site], "\015 CHOOSE  \001 ACT  \002 LEAVE  \010 GUIDE");
        const char *stories[] = {"A split wagon, drag marks and a trail toward the chain yard. Sora is "
                                 "stranded here. Stop the bleeding before asking her to move.",
                                 "The old drainage way bypasses Rell's road. Bring Sora home, then "
                                 "brace the crossing with 3 timber, 1 iron bar and reusable tools.",
                                 "Rell controls this stretch. Talk, pay for a truce, or challenge his "
                                 "scouts. Bringing him alive to Nell earns more than killing him.",
                                 "Orin and Tamsin wait in shackles. Approach each captive to unlock "
                                 "them. A guard within four meters interrupts lock work.",
                                 "A quiet refuge beside the road. Old medical supplies are wrapped "
                                 "under the shrine. The bed offers shelter for injured travelers.",
                                 "A dead workshop still holds repair parts. Its forge can service an "
                                 "expedition, but the surrounding wastes are dangerous.",
                                 "From the ridge, distant routes become clear. Salvage the old signal "
                                 "relics and use the atlas to choose your next destination.",
                                 "Copper stores depend on the road. Nell buys a lasting solution. Sora "
                                 "will remember the escort; the shelves reflect your choice."};
        wrap(31, 88, 68, stories[site], TEXT);
        char info[90];
        snprintf(info, sizeof(info), "%.1fm AWAY / NEED 1.7m / %s",
                 distance(a->x, a->y, g.depth.siteX[site], g.depth.siteY[site]),
                 g.depth.salvage & (1 << site) ? "CACHE EMPTY" : "CACHE AVAILABLE");
        text(31, 169, info, GOLD, 1);
        row(194, ui.selection == 0,
            site == SITE_CULVERT ? "Repair the alternate crossing" : "Search the landmark", "");
        row(213, ui.selection == 1, "Locate the chain yard", "");
        clipped(31, 225, depth_outcome(), 68, TEAL);
    }
}
