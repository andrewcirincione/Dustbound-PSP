#include "game.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
extern int conscious(const Actor *);
static int wrapi(int v, int n) { return (v % n + n) % n; }
static void open_view(int screen) {
    ui.screen = screen;
    ui.selection = 0;
    ui.scroll = 0;
}
void open_help(int topic) {
    ui.helpReturn = ui.screen;
    ui.helpSelection = ui.selection;
    ui.helpScroll = ui.scroll;
    ui.screen = SCREEN_HELP;
    ui.selection = topic;
}
void character_begin(int origin) {
    Actor *a = &g.actors[g.selected];
    ui.titleChoice = origin < 0 ? 1 : 0;
    ui.createOrigin = origin < 0 ? a->origin : origin;
    ui.createBody = origin < 0 ? a->body : 0;
    ui.createSkin = origin < 0 ? a->skin : 3;
    ui.createHair = origin < 0 ? a->hair : 2;
    ui.createHairColor = origin < 0 ? a->hairColor : 1;
    ui.createBeard = origin < 0 ? a->beard : 0;
    ui.createScar = origin < 0 ? a->scar : 1;
    ui.createRace = origin < 0 ? a->race : 0;
    snprintf(ui.createName, sizeof(ui.createName), "%s", origin < 0 ? a->name : "ASH");
    open_view(SCREEN_CREATE);
}
void character_finish(void) {
    int origin = ui.createOrigin, body = ui.createBody, skin = ui.createSkin, hair = ui.createHair,
        color = ui.createHairColor, beard = ui.createBeard, scar = ui.createScar, race = ui.createRace,
        editing = ui.titleChoice;
    char name[16];
    snprintf(name, sizeof(name), "%s", ui.createName[0] ? ui.createName : "ASH");
    if (!editing)
        game_new(73819 + (unsigned)(g.time * 37), origin == 4 ? 1 : 0);
    Actor *a = &g.actors[g.selected];
    snprintf(a->name, sizeof(a->name), "%s", name);
    a->body = body;
    a->skin = skin;
    a->hair = hair;
    a->hairColor = color;
    a->beard = beard;
    a->scar = scar;
    a->race = race;
    a->origin = origin;
    if (!editing) {
        /* Starting loadouts should leave room for supplies instead of applying
           an unexplained movement penalty on the first step. */
        const int strength[5] = {8, 11, 15, 2, 6};
        a->xp[STRENGTH] = 12 * strength[origin] * strength[origin];
        const int weapons[5] = {8, 0, 12, 9, 24}, torsos[5] = {32, 34, 35, 46, 45},
                  heads[5] = {49, 48, 56, 51, 54};
        a->gear[0] = gear_id(weapons[origin], origin == 1 ? 1 : 0, 1);
        a->gear[1] = gear_id(torsos[origin], origin == 1 ? 1 : 0, 0);
        a->gear[2] = gear_id(heads[origin], 0, 1);
        a->gear[3] = gear_id(origin == 1 ? 63 : 60, 0, 0);
        a->gear[4] = gear_id(origin == 1 ? 68 : 69, 0, 0);
        a->gear[5] = gear_id(origin == 2 ? 79 : 76, 0, 0);
        if (origin == 1) {
            g.money = 420;
            a->xp[MELEE] = 588;
            a->xp[DEFENCE] = 432;
        }
        if (origin == 2) {
            g.money = 800;
            a->xp[LABOUR] = 1452;
            a->xp[SMITHING] = 588;
            inventory_add(a, IRONBAR, 5);
            inventory_add(a, COAL, 3);
        }
        if (origin == 3) {
            g.money = 180;
            a->xp[STEALTH] = 1452;
            a->xp[THEFT] = 972;
            a->bounty = 160;
            g.rep[UNION] = -20;
        }
        if (origin == 4)
            inventory_add(a, BOLTS, 50);
        if (race == 1) {
            a->xp[TOUGHNESS] += 300;
            a->xp[STEALTH] += 150;
        } else if (race == 2) {
            a->xp[STRENGTH] += 300;
            a->xp[ENGINEERING] += 250;
        }
        g.playOrigin = origin;
    }
    open_view(SCREEN_WORLD);
    log_msg(editing ? "Appearance updated." : "Find work, gather allies, and make the frontier yours.");
}
static int guide_for(int s) {
    switch (s) {
    case SCREEN_CREATE:
    case SCREEN_NAME:
        return 1;
    case SCREEN_MAP:
        return 2;
    case SCREEN_HEALTH:
        return 4;
    case SCREEN_PACK:
    case SCREEN_EQUIPPED:
    case SCREEN_LOOT:
        return 6;
    case SCREEN_TRADE:
        return ui.shopFilter == 4 && !ui.sell ? 19 : 7;
    case SCREEN_FORGE:
        return 8;
    case SCREEN_CRAFT:
    case SCREEN_RESEARCH:
        return 9;
    case SCREEN_BUILD:
    case SCREEN_STORAGE:
        return 10;
    case SCREEN_SQUAD:
        return 11;
    case SCREEN_FACTIONS:
        return 13;
    case SCREEN_JOURNAL:
        return 14;
    case SCREEN_SAVES:
    case SCREEN_CONFIRM:
        return 15;
    default:
        return 0;
    }
}
static void adjust_creator(int d) {
    switch (ui.selection) {
    case 1:
        if (!ui.titleChoice)
            ui.createOrigin = wrapi(ui.createOrigin + d, 5);
        break;
    case 2:
        if (!ui.titleChoice)
            ui.createRace = wrapi(ui.createRace + d, 3);
        break;
    case 3:
        ui.createBody = wrapi(ui.createBody + d, 3);
        break;
    case 4:
        ui.createSkin = wrapi(ui.createSkin + d, 8);
        break;
    case 5:
        ui.createHair = wrapi(ui.createHair + d, 7);
        break;
    case 6:
        ui.createHairColor = wrapi(ui.createHairColor + d, 6);
        break;
    case 7:
        ui.createBeard = wrapi(ui.createBeard + d, 4);
        break;
    case 8:
        ui.createScar = wrapi(ui.createScar + d, 5);
        break;
    }
}
int expansion_input(float dt, Input in) {
    (void)dt;
    int p = in.pressed;
    Actor *a = &g.actors[g.selected];
    if (ui.screen != SCREEN_WORLD && ui.screen != SCREEN_TITLE && ui.screen != SCREEN_HELP &&
        (p & KEY_SELECT)) {
        open_help(guide_for(ui.screen));
        return 1;
    }
    if (ui.screen == SCREEN_HELP) {
        if (p & (KEY_CIRCLE | KEY_START)) {
            ui.screen = ui.helpReturn;
            ui.selection = ui.helpSelection;
            ui.scroll = ui.helpScroll;
            return 1;
        }
        if (p & (KEY_DOWN | KEY_R))
            ui.selection = wrapi(ui.selection + 1, GUIDE_PAGES);
        if (p & (KEY_UP | KEY_L))
            ui.selection = wrapi(ui.selection - 1, GUIDE_PAGES);
        return 1;
    }
    if (ui.screen == SCREEN_CREATE) {
        if (p & KEY_CIRCLE) {
            open_view(ui.titleChoice ? SCREEN_WORLD : SCREEN_TITLE);
            return 1;
        }
        if (p & KEY_UP)
            ui.selection = wrapi(ui.selection - 1, 10);
        if (p & KEY_DOWN)
            ui.selection = wrapi(ui.selection + 1, 10);
        if (p & KEY_LEFT)
            adjust_creator(-1);
        if (p & KEY_RIGHT)
            adjust_creator(1);
        if (p & KEY_CROSS) {
            if (ui.selection == 0) {
                ui.nameCell = 0;
                open_view(SCREEN_NAME);
            } else if (ui.selection == 9)
                character_finish();
            else
                adjust_creator(1);
        }
        return 1;
    }
    if (ui.screen == SCREEN_NAME) {
        if (p & KEY_LEFT)
            ui.nameCell = wrapi(ui.nameCell - 1, 40);
        if (p & KEY_RIGHT)
            ui.nameCell = wrapi(ui.nameCell + 1, 40);
        if (p & KEY_UP)
            ui.nameCell = wrapi(ui.nameCell - 10, 40);
        if (p & KEY_DOWN)
            ui.nameCell = wrapi(ui.nameCell + 10, 40);
        int len = strlen(ui.createName);
        if ((p & KEY_SQUARE) || ((p & KEY_CROSS) && ui.nameCell == 38)) {
            if (len)
                ui.createName[len - 1] = 0;
        } else if ((p & KEY_CROSS) && ui.nameCell < 38) {
            const char *keys = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -";
            if (len < 15) {
                ui.createName[len] = keys[ui.nameCell];
                ui.createName[len + 1] = 0;
            }
        }
        if ((p & (KEY_CIRCLE | KEY_START)) || ((p & KEY_CROSS) && ui.nameCell == 39))
            open_view(SCREEN_CREATE);
        return 1;
    }
    if (ui.screen == SCREEN_WORLD && a->prisonTime > 0) {
        if (p & KEY_CROSS) {
            dialogue_open(g.selected);
            return 1;
        }
        if (!(p & (KEY_L | KEY_START | KEY_SELECT | KEY_TRIANGLE | KEY_R))) {
            if (!ui.paused)
                simulate(dt * ui.speed);
            return 1;
        }
    }
    if (ui.screen == SCREEN_PACK && (p & KEY_TRIANGLE)) {
        open_view(SCREEN_EQUIPPED);
        return 1;
    }
    if (ui.screen == SCREEN_EQUIPPED) {
        if (p & (KEY_CIRCLE | KEY_START)) {
            open_view(SCREEN_WORLD);
            return 1;
        }
        if (p & KEY_TRIANGLE) {
            open_view(SCREEN_PACK);
            return 1;
        }
        if (p & KEY_UP)
            ui.selection = wrapi(ui.selection - 1, GEARS);
        if (p & KEY_DOWN)
            ui.selection = wrapi(ui.selection + 1, GEARS);
        if (p & KEY_CROSS) {
            int it = a->gear[ui.selection];
            if (it && inventory_add_owned(a, it, 1, a->gearStolen[ui.selection])) {
                a->gear[ui.selection] = 0;
                a->gearStolen[ui.selection] = 0;
                log_msg("Removed %s.", itemDefs[it].name);
            } else
                log_msg("No equipment, or pack is full.");
        }
        return 1;
    }
    if (ui.screen == SCREEN_TRADE && (p & (KEY_L | KEY_R))) {
        if (ui.sell)
            return 1;
        ui.shopFilter = wrapi(ui.shopFilter + (p & KEY_L ? -1 : 1), 5);
        ui.selection = 0;
        return 1;
    }
    if (ui.screen == SCREEN_SQUAD && (p & KEY_SQUARE)) {
        if (a->dead && g.squadCount > 1) {
            if (g.interiorBuilding)
                interior_leave();
            a->squad = 0;
            g.squadCount--;
            for (int n = 0; n < g.actorCount; n++)
                if (g.actors[n].squad) {
                    g.selected = n;
                    break;
                }
            for (int n = 0; n < g.actorCount; n++)
                if (g.actors[n].squad && !g.actors[n].dead) {
                    g.selected = n;
                    break;
                }
            open_view(SCREEN_WORLD);
            log_msg("Fallen companion released from the squad roster.");
        } else
            log_msg("\003 dismisses a fallen companion only.");
        return 1;
    }
    if (ui.screen == SCREEN_MAP && (p & (KEY_L | KEY_R | KEY_SQUARE | KEY_TRIANGLE))) {
        if (p & KEY_TRIANGLE)
            ui.mapZoom = !ui.mapZoom;
        else if (p & KEY_SQUARE) {
            if (inventory_remove(a, MAPSCROLL, 1)) {
                for (int i = 0; i < TOWNS; i++)
                    g.discovered[i] = 1;
                memset(g.seen, 255, sizeof(g.seen));
                log_msg("Survey chart added to the atlas.");
            } else
                log_msg("Buy a survey chart from a merchant.");
        } else {
            for (int i = 0; i < TOWNS; i++) {
                ui.mapTown = wrapi(ui.mapTown + (p & KEY_L ? -1 : 1), TOWNS);
                if (g.discovered[ui.mapTown])
                    break;
            }
            ui.mapX = towns[ui.mapTown].x;
            ui.mapY = towns[ui.mapTown].y;
        }
        return 1;
    }
    if (ui.screen == SCREEN_FORGE) {
        static const int first[GEARS] = {0, 32, 48, 60, 68, 76}, counts[GEARS] = {36, 16, 12, 8, 8, 8};
        if (p & (KEY_CIRCLE | KEY_START)) {
            open_view(SCREEN_WORLD);
            return 1;
        }
        if (p & KEY_UP)
            ui.selection = wrapi(ui.selection - 1, 5);
        if (p & KEY_DOWN)
            ui.selection = wrapi(ui.selection + 1, 5);
        if (p & (KEY_LEFT | KEY_RIGHT)) {
            int d = p & KEY_LEFT ? -1 : 1;
            if (ui.selection == 0) {
                ui.forgeSlot = wrapi(ui.forgeSlot + d, GEARS);
                ui.forgeFamily = first[ui.forgeSlot];
            }
            if (ui.selection == 1) {
                int index = ui.forgeFamily >= 84 ? ui.forgeFamily - 52 : ui.forgeFamily - first[ui.forgeSlot];
                index = wrapi(index + d, counts[ui.forgeSlot]);
                ui.forgeFamily = ui.forgeSlot == 0 ? weapon_family_index(index) : first[ui.forgeSlot] + index;
            }
            if (ui.selection == 2)
                ui.forgeMaterial = wrapi(ui.forgeMaterial + d, MATERIALS);
            if (ui.selection == 3)
                ui.forgeQuality = wrapi(ui.forgeQuality + d, QUALITIES);
        }
        if ((p & KEY_CROSS) && ui.selection == 4)
            forge_equipment(gear_id(ui.forgeFamily, ui.forgeMaterial, ui.forgeQuality));
        return 1;
    }
    if (ui.screen == SCREEN_STORAGE) {
        if (p & (KEY_CIRCLE | KEY_START)) {
            open_view(SCREEN_WORLD);
            return 1;
        }
        if (p & (KEY_LEFT | KEY_RIGHT)) {
            ui.storageSide = !ui.storageSide;
            ui.selection = 0;
        }
        int count = ui.storageSide ? 16 : PACK;
        if (p & KEY_UP)
            ui.selection = wrapi(ui.selection - 1, count);
        if (p & KEY_DOWN)
            ui.selection = wrapi(ui.selection + 1, count);
        if (p & (KEY_CROSS | KEY_SQUARE)) {
            int qty = p & KEY_SQUARE
                          ? (ui.storageSide ? g.buildings[ui.storageIndex].storage[ui.selection].qty
                                            : a->pack[ui.selection].qty)
                          : 1;
            int moved = 0;
            while (moved < qty && storage_transfer(ui.storageIndex, ui.storageSide, ui.selection))
                moved++;
            if (moved > 1)
                log_msg("Moved %d items. Remaining goods stay in their source stack.", moved);
        }
        return 1;
    }
    if (ui.screen == SCREEN_SAVES) {
        if (p & (KEY_CIRCLE | KEY_START)) {
            open_view(ui.saveReturn);
            ui.selection = ui.saveReturn == SCREEN_TITLE ? 2 : ui.saveExit ? 9 : ui.saveMode ? 4 : 5;
            ui.saveExit = 0;
            return 1;
        }
        if (p & KEY_UP)
            ui.selection = wrapi(ui.selection - 1, SAVE_SLOTS);
        if (p & KEY_DOWN)
            ui.selection = wrapi(ui.selection + 1, SAVE_SLOTS);
        if (p & KEY_CROSS) {
            if (ui.saveMode && ui.selection == 0) {
                log_msg("Slot zero is reserved for autosave.");
                return 1;
            }
            if (!ui.saveMode && !ui.saves[ui.selection].valid) {
                log_msg("This slot is empty or incompatible.");
                return 1;
            }
            ui.slot = ui.selection;
            ui.confirmAction = ui.saveMode ? 3 : 2;
            open_view(SCREEN_CONFIRM);
        }
        return 1;
    }
    if (ui.screen == SCREEN_JOURNAL || ui.screen == SCREEN_FACTIONS) {
        if (p & (KEY_CIRCLE | KEY_START)) {
            open_view(SCREEN_WORLD);
            return 1;
        }
        int n = ui.screen == SCREEN_JOURNAL ? CONTRACTS : FACTIONS;
        if (p & KEY_UP)
            ui.selection = wrapi(ui.selection - 1, n);
        if (p & KEY_DOWN)
            ui.selection = wrapi(ui.selection + 1, n);
        if (ui.screen == SCREEN_JOURNAL && (p & KEY_CROSS))
            complete_contract(ui.selection);
        if (ui.screen == SCREEN_JOURNAL && (p & KEY_SQUARE)) {
            Contract *c = &g.contracts[ui.selection];
            if (c->active == 1) {
                int town = c->type == 0 && c->progress < c->needed ? c->target : c->town;
                open_view(SCREEN_MAP);
                ui.mapTown = town;
                ui.mapX = towns[town].x;
                ui.mapY = towns[town].y;
                ui.mapZoom = 0;
                log_msg("Contract: %s. Choose travel on the map when ready.", towns[town].name);
            } else
                log_msg("Select an active contract to locate its destination.");
        }
        return 1;
    }
    if (ui.screen == SCREEN_LOOT) {
        if (p & (KEY_CIRCLE | KEY_START)) {
            open_view(SCREEN_WORLD);
            return 1;
        }
        int n = ui.lootActor;
        if (n < 0 || n >= g.actorCount) {
            open_view(SCREEN_WORLD);
            return 1;
        }
        int count = PACK + GEARS;
        if (p & KEY_UP)
            ui.selection = wrapi(ui.selection - 1, count);
        if (p & KEY_DOWN)
            ui.selection = wrapi(ui.selection + 1, count);
        if (p & (KEY_CROSS | KEY_SQUARE))
            loot_take(ui.selection, !!(p & KEY_SQUARE));
        return 1;
    }
    if (ui.screen == SCREEN_ASSIST) {
        int n = ui.lootActor;
        if (p & (KEY_CIRCLE | KEY_START)) {
            open_view(SCREEN_WORLD);
            return 1;
        }
        if (n < 0 || n >= g.actorCount) {
            open_view(SCREEN_WORLD);
            return 1;
        }
        Actor *b = &g.actors[n];
        if(p&KEY_SQUARE){depth_repair(g.selected,n);return 1;}
        if (p & KEY_UP)
            ui.selection = wrapi(ui.selection - 1, 4);
        if (p & KEY_DOWN)
            ui.selection = wrapi(ui.selection + 1, 4);
        if (p & KEY_CROSS) {
            if (!conscious(a) || distance(a->x, a->y, b->x, b->y) > TALK_REACH ||
                actor_inside(a) != actor_inside(b)) {
                log_msg("Choose a conscious survivor within reach.");
                return 1;
            }
            if (ui.selection == 0) {
                if (!heal_actor(g.selected, n))
                    log_msg(b->dead ? "Too late for first aid." : "Needs an injury and a field dressing.");
            } else if (ui.selection == 1) {
                if (b->dead || b->unconscious <= 0 || b->carriedBy >= 0 || a->carrying >= 0)
                    log_msg("Carry needs an unconscious survivor and free arms.");
                else {
                    a->carrying = n;
                    b->carriedBy = g.selected;
                    a->taskKind = 0;
                    ui.waypoint = 0;
                    open_view(SCREEN_WORLD);
                    log_msg("Carrying %s. \001 puts them down; speed is reduced.", b->name);
                }
            } else if (ui.selection == 2) {
                if (!b->dead && b->unconscious <= 0)
                    log_msg("Use their pack directly while they are conscious.");
                else
                    open_view(SCREEN_LOOT);
            } else {
                g.selected = n;
                open_view(SCREEN_WORLD);
            }
        }
        return 1;
    }
    if (ui.screen == SCREEN_LOG) {
        if (p & (KEY_CIRCLE | KEY_START))
            open_view(SCREEN_WORLD);
        return 1;
    }
    if (ui.screen == SCREEN_DIALOG) {
        int n = ui.dialogActor;
        if (n < 0 || n >= g.actorCount) {
            open_view(SCREEN_WORLD);
            return 1;
        }
        Actor *b = &g.actors[n];
        int town = town_near(a->x, a->y, 14), prison = a->prisonTime > 0 && n == g.selected,
            count = prison               ? 3
                    : ui.dialogPage == 1 ? 5
                                         : dialogue_count(n);
        if (p & (KEY_CIRCLE | KEY_START)) {
            open_view(SCREEN_WORLD);
            return 1;
        }
        if (p & KEY_UP)
            ui.selection = wrapi(ui.selection - 1, count);
        if (p & KEY_DOWN)
            ui.selection = wrapi(ui.selection + 1, count);
        if (p & KEY_CROSS) {
            int choice = ui.selection;
            if (prison) {
                if (choice == 0) {
                    if (g.money >= a->bounty) {
                        g.money -= a->bounty;
                        a->bounty = 0;
                        a->prisonTime = 0;
                        g.rep[g.townOwner[a->prisonTown]] = 0;
                        open_view(SCREEN_WORLD);
                    } else
                        log_msg("Insufficient silver for bail.");
                } else if (choice == 1) {
                    gain(a, LOCKPICK, 30);
                    if (inventory_count(a, TOOLS) && ((g.rng = g.rng * 1664525u + 1013904223u) % 100) <
                                                         (unsigned)(25 + level(a, LOCKPICK))) {
                        a->prisonTime = 0;
                        a->bounty += 100;
                        g.rep[g.townOwner[a->prisonTown]] -= 18;
                        open_view(SCREEN_WORLD);
                        log_msg("Lock opened. Escape before the guards return.");
                    } else {
                        a->prisonTime += 20;
                        a->bounty += 20;
                        log_msg("Escape failed. Sentence and bounty increased.");
                    }
                } else
                    open_view(SCREEN_WORLD);
            } else if (ui.dialogPage == 1) {
                if (choice < 4)
                    accept_contract(town, choice);
                else {
                    ui.dialogPage = 0;
                    ui.selection = 0;
                }
            } else
                switch (dialogue_action(n, choice)) {
                case 0:
                    if (town >= 0 && depth_trade_open(town)) {
                        ui.tradeTown = town;
                        ui.sell = 0;
                        ui.shopFilter = 0;
                        open_view(SCREEN_TRADE);
                    } else
                        log_msg("No market on this stretch of road.");
                    break;
                case 1:
                    ui.dialogPage = 1;
                    ui.selection = 0;
                    break;
                case 2:
                    start_sleep(town);
                    break;
                case 3:
                    doctor_heal(town);
                    break;
                case 4:
                    if (town >= 0 && a->bounty > 0 && g.money >= a->bounty) {
                        g.money -= a->bounty;
                        a->bounty = 0;
                        g.rep[g.townOwner[town]] = 0;
                        log_msg("Bounty paid to the local authority.");
                    } else {
                        ui.dialogPage = 2;
                        log_msg("%s", g.worldNews);
                    }
                    break;
                case 5:
                    if (town >= 0)
                        character_begin(-1);
                    break;
                case 6:
                    if (b->kind == RECRUIT) {
                        if (recruit_actor(n))
                            open_view(SCREEN_WORLD);
                    } else {
                        ui.dialogPage = 2;
                        log_msg("%s", g.worldNews);
                    }
                    break;
                case 7:
                    open_view(SCREEN_FORGE);
                    break;
                case 9:
                    merc_hire(n, 0);
                    break;
                case 10:
                    merc_hire(n, 1);
                    break;
                case 11:
                    merc_dismiss(n);
                    break;
                case 12:
                    ui.dialogPage = 3;
                    break;
                case 8:
                    open_view(SCREEN_WORLD);
                    break;
                }
        }
        return 1;
    }
    return 0;
}
