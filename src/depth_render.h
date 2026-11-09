/* Included by render.c: shares the bounded PSP software-renderer primitives. */
static void depth_world_draw(void) {
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
    text(31, 237, ui.menuReturn ? "\002 BACK   \010 GUIDE" : "\002 CLOSE   \010 GUIDE", MUTED, 1);
}
static void depth_screen(void) {
    Actor *a = &g.actors[g.selected];
    DepthActor *z = &g.depth.actors[g.selected];
    char buf[140];
    if (ui.screen == SCREEN_ORDERS) {
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
        text(31, 225, "MAP PINS ARE REFERENCES. WALK WITH THE ANALOG STICK.", TEAL, 1);
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
                 "No owned structures yet. Build a camp outside town from Start, World, Build a camp.", TEXT);
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
        int count=item_view_count(), first = start_row(item_view_row(ui.selection), count, 4);
        for (int j = 0; j < 4 && first+j<count; j++) {
            int i = item_view_slot(first+j), it = a->pack[i].item;
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
    }
}
