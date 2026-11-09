#include "game.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
extern int conscious(const Actor *);
static int wrapi(int v, int n) { return (v % n + n) % n; }
/* Named world destinations. Local view retains free cursor travel. */
int atlas_item(int row){if(row==0)return -1;for(int t=0;t<TOWNS;t++)if(g.discovered[t]&&--row==0)return t;return -2;}
int atlas_count(void){int n=1;for(int t=0;t<TOWNS;t++)n+=!!g.discovered[t];return n;}
void atlas_select(int row){ui.mapCategory=0;ui.mapChoice=wrapi(row,atlas_count());int id=atlas_item(ui.mapChoice);if(id<0){ui.mapX=g.actors[g.selected].x;ui.mapY=g.actors[g.selected].y;}else{ui.mapTown=id;ui.mapX=towns[id].x;ui.mapY=towns[id].y;}}
void atlas_sync(void){float best=distance(ui.mapX,ui.mapY,g.actors[g.selected].x,g.actors[g.selected].y);int row=0;for(int n=1;n<atlas_count();n++){int id=atlas_item(n);float d=distance(ui.mapX,ui.mapY,towns[id].x,towns[id].y);if(d<best){best=d;row=n;}}atlas_select(row);}
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
    ui.scroll=ui.helpTopic=0;
}
const StartScenario startScenarios[START_SCENARIOS] = {
    {3, "Standard", "Start alone in the Dry Market, a Scavenger trading town. You have field dressings and enough silver to buy supplies or hire help."},
    {11, "Hard", "Start in Shogun's Rest with a blade and a Shogunate uniform. The garrison watches outsiders."},
    {16, "Standard", "Start at Brass Junction with metal, coal, and a heavy pack. Labour and smithing give you a head start. Work the nearby deposits or trade your stock."},
    {8, "Hardest", "Start outside Black Mesa with 180 silver and a Union theft bounty. You know how to sneak and steal. Union patrols will look for you beyond the enclave."},
    {10, "Easy", "Start at Cinder Port with Reed and Vale. You have more silver, a travel pack, a crossbow and dressings. Three people need food and care."}
};
void scenario_menu(int origin, int from) {
    ui.scenarioReturn = from;
    open_view(SCREEN_SCENARIOS);
    ui.selection = origin;
}
static void place_starting_squad(int origin) {
    int town = startScenarios[origin].town, row = 0;
    const Town *start = &towns[town];
    float cx = start->x, cy = start->y + (origin == 3 ? 12 : 3);
    for (int n = 0; n < g.actorCount; n++) {
        Actor *a = &g.actors[n];
        if (!a->squad)
            continue;
        float ox = cx + (row ? (row == 1 ? -1.8f : 1.8f) : 0), oy = cy + (row ? 1.5f : 0);
        float best = 1e9f;
        for (int dy = -10; dy <= 10; dy++)
            for (int dx = -10; dx <= 10; dx++) {
                float x = ox + dx * .75f, y = oy + dy * .75f, score = dx * dx + dy * dy;
                if (score >= best || !can_walk(x, y) || !can_walk(x + .35f, y + .35f))
                    continue;
                int clear = 1;
                for (int j = 0; j < g.actorCount && clear; j++) {
                    Actor *b = &g.actors[j];
                    if (b == a || !b->active || b->dead || (b->squad && j > n))
                        continue;
                    float reach = b->squad ? 1.25f : hostile(a, b) ? 9 : 1;
                    clear = distance(x, y, b->x, b->y) >= reach;
                }
                if (!clear)
                    continue;
                a->x = x;
                a->y = y;
                best = score;
            }
        a->homeX = a->tx = a->x;
        a->homeY = a->ty = a->y;
        a->homeTown = town;
        row++;
    }
    Actor *a = &g.actors[g.selected];
    ui.mapX = ui.camX = a->x;
    ui.mapY = ui.camY = a->y;
    ui.contextChoice = ui.feedCount = ui.logCount = 0;
    g.discovered[town] = 1;
    reveal_area(a);
    find_context();
    log_msg("%s near %s.", originNames[origin], start->name);
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
    ui.createRace = origin < 0 ? a->race : 0;
    snprintf(ui.createName, sizeof(ui.createName), "%s", origin < 0 ? a->name : "ASH");
    open_view(SCREEN_CREATE);
}
void starting_gear(Actor *a, int origin) {
    static const int weapons[] = {8, 0, 12, 9, 24}, torsos[] = {32, 34, 35, 46, 45},
                     heads[] = {49, 50, 56, 51, 54};
    a->gear[0] = gear_id(weapons[origin], origin == 1 ? 1 : 0, 1);
    a->gear[1] = gear_id(torsos[origin], origin == 1 ? 1 : 0, 0);
    a->gear[2] = gear_id(heads[origin], 0, 1);
    a->gear[3] = gear_id(origin == 1 ? 63 : 60, 0, 0);
    a->gear[4] = gear_id(origin == 1 ? 68 : 69, 0, 0);
    a->gear[5] = origin == 2 ? gear_id(79, 0, 0) : origin == 4 ? gear_id(76, 0, 0) : NONE;
}
void character_finish(void) {
    int origin = ui.createOrigin, body = ui.createBody, skin = ui.createSkin, hair = ui.createHair,
        color = ui.createHairColor, beard = ui.createBeard, race = ui.createRace,
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
    a->race = race;
    a->origin = origin;
    if (!editing) {
        starting_gear(a, origin);
        a->autoattack = 0;
        if (origin == 1) {
            g.money = 420;
            inventory_remove(a, WOOD, inventory_count(a, WOOD));
        }
        if (origin == 2) {
            g.money = 800;
            inventory_add(a, IRONBAR, 5);
            inventory_add(a, COAL, 3);
        }
        if (origin == 3) {
            g.money = 180;
            crime_charge(a, UNION, 160, CRIME_THEFT);
            g.rep[UNION] = -20;
        }
        if (origin == 4)
            inventory_add(a, BOLTS, 50);
        if (race == 2) {
            inventory_remove(a, MEDKIT, inventory_count(a, MEDKIT));
            inventory_add(a, PARTS, 4);
        }
        a->scar = 0;
        species_start(a,origin,race);
        g.playOrigin = origin;
        place_starting_squad(origin);
    }
    open_view(SCREEN_WORLD);
    if (editing) log_msg("Appearance updated.");
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
    case SCREEN_CHEST:
    case SCREEN_THEFT:
        return 12;
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
        if (!ui.titleChoice) ui.createRace = wrapi(ui.createRace + d, SPECIES);
        break;
    case 2:
        ui.createBody = wrapi(ui.createBody + d, 3);
        break;
    case 3:
        ui.createSkin = wrapi(ui.createSkin + d, 8);
        break;
    case 4:
        if (ui.createRace != 2) ui.createHair = wrapi(ui.createHair + d, 7);
        break;
    case 5:
        if (ui.createRace != 2) ui.createHairColor = wrapi(ui.createHairColor + d, 6);
        break;
    case 6:
        if (ui.createRace != 2) ui.createBeard = wrapi(ui.createBeard + d, 4);
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
        if (p & (KEY_DOWN | KEY_R)){ui.selection = wrapi(ui.selection + 1, GUIDE_PAGES);ui.scroll=0;}
        if (p & (KEY_UP | KEY_L)){ui.selection = wrapi(ui.selection - 1, GUIDE_PAGES);ui.scroll=0;}
        if(p&KEY_RIGHT)ui.scroll=(int)fminf(ui.helpTopic,ui.scroll+110);
        if(p&KEY_LEFT)ui.scroll=(int)fmaxf(0,ui.scroll-110);
        return 1;
    }
    if (ui.screen == SCREEN_PASS) {
        int count = 0;
        while (pass_recipient(count) >= 0)
            count++;
        int done = p & (KEY_CIRCLE | KEY_START);
        if (count) {
            if (p & KEY_UP) ui.selection = wrapi(ui.selection - 1, count);
            if (p & KEY_DOWN) ui.selection = wrapi(ui.selection + 1, count);
            if (p & KEY_CROSS) {
                int n = pass_recipient(ui.selection);
                const char *blocked = pass_blocked(n);
                if (blocked)
                    log_msg("%s.", blocked);
                else {
                    int item = a->pack[ui.passSlot].item;
                    if (inventory_transfer(a, &g.actors[n], ui.passSlot, 1)) {
                        log_msg("Passed %s to %s.", itemDefs[item].name, g.actors[n].name);
                        done = 1;
                    }
                }
            }
        }
        if (done) {
            open_view(SCREEN_PACK);
            ui.selection = ui.passSlot;
            item_view_normalize();
        }
        return 1;
    }
    if(ui.screen==SCREEN_ACTIONS){
        int count=nearby_actions();
        if(p&(KEY_CIRCLE|KEY_START)){open_view(SCREEN_WORLD);return 1;}
        if(p&KEY_R){ui.paused=!ui.paused;if(!ui.paused){open_view(SCREEN_WORLD);return 1;}}
        if(p&KEY_SQUARE){g.society.queueCount[g.selected]=0;log_msg("Queued actions cleared.");}
        if(count){if(p&KEY_UP)ui.selection=wrapi(ui.selection-1,count);if(p&KEY_DOWN)ui.selection=wrapi(ui.selection+1,count);if(ui.selection>=count)ui.selection=count-1;if(p&KEY_CROSS)nearby_action_choose(ui.selection);}
        return 1;
    }
    if(ui.screen==SCREEN_ENGAGE) {
        int count=combat_candidates();
        if(p&(KEY_CIRCLE|KEY_START)){open_view(SCREEN_WORLD);return 1;}
        if(count){if(p&KEY_UP)ui.selection=wrapi(ui.selection-1,count);if(p&KEY_DOWN)ui.selection=wrapi(ui.selection+1,count);if(ui.selection>=count)ui.selection=count-1;}
        ui.attackFocus=combat_candidate(ui.selection);
        if((p&KEY_CROSS)&&ui.attackFocus>=0){command_attack(ui.attackFocus);open_view(SCREEN_WORLD);}
        return 1;
    }
    if (ui.screen == SCREEN_HEALTH && (p & (KEY_CROSS|KEY_SQUARE|KEY_TRIANGLE))) {
        DepthActor *z=&g.depth.actors[g.selected];
        if(p&KEY_CROSS) {if(z->captive)depth_capture_action(g.selected);else heal_actor(g.selected,g.selected);}
        if(p&KEY_SQUARE)depth_repair(g.selected,g.selected);
        if(p&KEY_TRIANGLE) {
            if(z->upkeep>=100)log_msg("Equipment is already maintained.");
            else if(inventory_remove(a,CLOTH,1)){z->upkeep=fminf(100,z->upkeep+40);log_msg("Equipment maintained.");}
            else log_msg("Maintenance needs one cloth.");
        }
        return 1;
    }
    if(ui.screen==SCREEN_SQUAD && !(p&KEY_SQUARE)) {
        if(p&(KEY_CIRCLE|KEY_START)){open_view(SCREEN_WORLD);return 1;}
        if (p & KEY_TRIANGLE) {
            ui.squadPage = !ui.squadPage;
            ui.selection = 0;
            return 1;
        }
        if(p&KEY_L)cycle_squad(-1);
        if(p&KEY_R)cycle_squad(1);
        a=&g.actors[g.selected];DepthActor *z=&g.depth.actors[g.selected];
        if (slave_in_camp(a)) return 1;
        int rows = ui.squadPage ? 5 : 3;
        if (p & KEY_UP) ui.selection = wrapi(ui.selection - 1, rows);
        if (p & KEY_DOWN) ui.selection = wrapi(ui.selection + 1, rows);
        if(p&(KEY_LEFT|KEY_RIGHT|KEY_CROSS)) {
            int d=p&KEY_LEFT?-1:1;
            if (ui.squadPage) {
                if (ui.selection == 0) {
                    int mode = wrapi((!z->queued ? 0 : z->paused ? 2 : 1) + d, 3);
                    z->queued = mode != 0;
                    z->paused = mode == 2;
                    a->taskKind = 0;
                } else
                    z->queue[ui.selection - 1] = wrapi(z->queue[ui.selection - 1] + d, 8);
            } else if (ui.selection == 0) {
                a->job = wrapi(a->job + d, 8);
                z->queued = z->paused = a->taskKind = 0;
            } else if (ui.selection == 1) {
                z->order = wrapi(z->order + d, 4);
                a->autoattack = z->order != ORDER_HOLD_FIRE;
                if (!a->autoattack) {
                    g.living.command[g.selected] = -1;
                    a->target = -1;
                    a->attackPhase = 0;
                }
            } else
                z->group = wrapi(z->group + d, 3);
        }
        return 1;
    }
    if (ui.screen == SCREEN_THEFT) {
        if (p & (KEY_CIRCLE | KEY_START)) {
            open_view(SCREEN_WORLD);
        } else if (p & KEY_CROSS) {
            pickpocket_slot(ui.lootActor, ui.selection);
            open_view(SCREEN_WORLD);
        }
        return 1;
    }
    if (ui.screen == SCREEN_SCENARIOS) {
        if (p & KEY_CIRCLE)
            open_view(ui.scenarioReturn);
        else if (p & KEY_CROSS)
            character_begin(ui.selection);
        else if (p & (KEY_UP | KEY_DOWN))
            ui.selection = wrapi(ui.selection + (p & KEY_UP ? -1 : 1), START_SCENARIOS);
        return 1;
    }
    if (ui.screen == SCREEN_CREATE) {
        if (p & KEY_CIRCLE) {
            if (ui.titleChoice) open_view(SCREEN_WORLD);
            else scenario_menu(ui.createOrigin, ui.scenarioReturn);
            return 1;
        }
        if (p & KEY_UP)
            ui.selection = wrapi(ui.selection - 1, 8);
        if (p & KEY_DOWN)
            ui.selection = wrapi(ui.selection + 1, 8);
        if (p & KEY_LEFT)
            adjust_creator(-1);
        if (p & KEY_RIGHT)
            adjust_creator(1);
        if (p & KEY_CROSS) {
            if (ui.selection == 0) {
                ui.nameCell = 0;
                open_view(SCREEN_NAME);
            } else if (ui.selection == 7)
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
        if (ui.sell || (ui.tradeSmith && ui.tradeSmith != TRADE_SMITH))
            return 1;
        if (ui.tradeSmith == TRADE_SMITH) {
            static const int filters[] = {0, 2, 3};
            int index = ui.shopFilter == 2 ? 1 : ui.shopFilter == 3 ? 2 : 0;
            ui.shopFilter = filters[wrapi(index + (p & KEY_L ? -1 : 1), 3)];
        } else
            ui.shopFilter = wrapi(ui.shopFilter + (p & KEY_L ? -1 : 1), 6);
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
    if (ui.screen == SCREEN_MAP && !ui.mapZoom && (p & (KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT|KEY_L|KEY_R))) {
        if(p&(KEY_L|KEY_R|KEY_LEFT|KEY_RIGHT))atlas_select(ui.mapChoice+(p&(KEY_L|KEY_LEFT)?-7:7));
        else atlas_select(ui.mapChoice+(p&KEY_UP?-1:1));
        return 1;
    }
    if (ui.screen == SCREEN_MAP && (p & (KEY_L | KEY_R | KEY_SQUARE | KEY_TRIANGLE))) {
        if (p & KEY_TRIANGLE) {
            ui.mapZoom = !ui.mapZoom;
            if(!ui.mapZoom)atlas_sync();
        }
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
    if(ui.screen==SCREEN_CRAFT&&(p&KEY_SQUARE)){open_view(SCREEN_FORGE);return 1;}
    if (ui.screen == SCREEN_FORGE) {
        if(p&KEY_SQUARE){open_view(SCREEN_CRAFT);return 1;}
        static const int first[GEARS] = {0, 32, 48, 60, 68, 76}, counts[GEARS] = {38, 20, 12, 8, 8, 8};
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
                int index = ui.forgeFamily >= 90 ? ui.forgeFamily-90+16 : ui.forgeFamily >= 84 ? ui.forgeFamily - 52 : ui.forgeFamily - first[ui.forgeSlot];
                index = wrapi(index + d, counts[ui.forgeSlot]);
                ui.forgeFamily = ui.forgeSlot == 0 ? (index>=36?index+52:weapon_family_index(index)) : ui.forgeSlot==1&&index>=16?index-16+90:first[ui.forgeSlot]+index;
            }
            if (ui.selection == 2)
                ui.forgeMaterial = wrapi(ui.forgeMaterial + d, MATERIALS);
            if (ui.selection == 3)
                ui.forgeQuality = wrapi(ui.forgeQuality + d, QUALITIES);
        }
        if ((p & KEY_CROSS) && ui.selection == 4)
            forge_equipment(forge_selection_item());
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
            ui.selection = ui.saveReturn == SCREEN_TITLE ? 1 : ui.saveExit ? 5 : ui.saveMode ? 1 : 2;
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
        if (ui.screen == SCREEN_JOURNAL && (p & (KEY_LEFT | KEY_RIGHT | KEY_L | KEY_R))) {
            ui.journalTab ^= 1;
            ui.selection = 0;
        }
        int n = ui.screen == SCREEN_JOURNAL ? journal_count() : FACTIONS;
        if (!n) return 1;
        if (p & KEY_UP)
            ui.selection = wrapi(ui.selection - 1, n);
        if (p & KEY_DOWN)
            ui.selection = wrapi(ui.selection + 1, n);
        if (ui.screen == SCREEN_JOURNAL && !ui.journalTab && (p & KEY_SQUARE)) {
            float x, y;
            int town = contract_location(journal_slot(ui.selection), &x, &y);
            if (town >= 0) {
                open_view(SCREEN_MAP);
                ui.mapTown = town;
                ui.mapX = x;
                ui.mapY = y;
                ui.mapZoom = 1;
            } else
                log_msg("Select an active contract to locate its destination.");
        }
        return 1;
    }
    if (ui.screen == SCREEN_CHEST) {
        if (p & (KEY_CIRCLE | KEY_START)) {
            open_view(SCREEN_WORLD);
            return 1;
        }
        int count = interior_chest_count();
        if (count) {
            if (p & KEY_UP) ui.selection = wrapi(ui.selection - 1, count);
            if (p & KEY_DOWN) ui.selection = wrapi(ui.selection + 1, count);
            if (p & KEY_CROSS) interior_chest_take(ui.selection);
        }
        if (!interior_chest_item(ui.selection))
            ui.selection = 0;
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
        int count = PACK + GEARS + 4;
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
                    open_view(SCREEN_WORLD);
                    log_msg("Carrying %s. \001 puts them down; speed is reduced.", b->name);
                }
            } else if (ui.selection == 2) {
                if (!b->dead && b->unconscious <= 0)
                    log_msg("Use their pack directly while they are conscious.");
                else
                    open_view(SCREEN_LOOT);
            } else {
                select_squad_member(n);
                open_view(SCREEN_WORLD);
            }
        }
        return 1;
    }
    if (ui.screen == SCREEN_LOG) {
        if (p & KEY_UP && ui.scroll > 0) ui.scroll--;
        if (p & KEY_DOWN && ui.scroll + 6 < ui.logCount && ui.scroll + 6 < LOGS) ui.scroll++;
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
            count = prison ? 3 : ui.dialogPage == DIALOG_WORK ? work_dialogue_count(town)
                    : ui.dialogPage == DIALOG_BOUNTY ? 3
                    : (ui.dialogPage == DIALOG_INSPECTION || ui.dialogPage == DIALOG_SUMMONS) ? 2
                    : (ui.dialogPage == DIALOG_DISMISS || ui.dialogPage == DIALOG_WORK_HINT || ui.dialogPage == DIALOG_IRON) ? 1 : dialogue_count(n);
        if (p & (KEY_CIRCLE | KEY_START)) {
            if (ui.dialogPage == DIALOG_SUMMONS) return 1;
            if (ui.dialogPage == DIALOG_INSPECTION)
                pocket_inspection(n, 0);
            open_view(SCREEN_WORLD);
            return 1;
        }
        if (p & KEY_UP)
            ui.selection = wrapi(ui.selection - 1, count);
        if (p & KEY_DOWN)
            ui.selection = wrapi(ui.selection + 1, count);
        if (p & KEY_CROSS) {
            int choice = ui.selection;
            if (ui.dialogPage == DIALOG_SUMMONS) {
                if ((choice == 0 && crime_pay(a, b->faction)) || (choice == 1 && watch_surrender(n)))
                    open_view(SCREEN_WORLD);
            } else if (ui.dialogPage == DIALOG_BOUNTY) {
                if (choice == 0 && crime_pay(a, b->faction))
                    open_view(SCREEN_WORLD);
                if (choice > 0) {
                    ui.dialogPage = choice == 1 ? 5 : 0;
                    ui.selection = 0;
                }
            } else if (ui.dialogPage == DIALOG_IRON) {
                int node = town_ore(town);
                if (node >= 0) {
                    open_view(SCREEN_MAP);
                    ui.mapTown = town;
                    ui.mapZoom = 1;
                    ui.mapX = ui.travelX = g.nodes[node].x;
                    ui.mapY = ui.travelY = g.nodes[node].y;
                    ui.waypoint = 1;
                } else {
                    ui.dialogPage = 0;
                    ui.selection = 0;
                }
            } else if (ui.dialogPage == DIALOG_WORK_HINT) {
                ui.dialogPage = 0;
                ui.selection = 0;
            } else if (ui.dialogPage == DIALOG_INSPECTION || ui.dialogPage == DIALOG_DISMISS) {
                if (ui.dialogPage == DIALOG_INSPECTION)
                    pocket_inspection(n, choice == 0);
                open_view(SCREEN_WORLD);
            } else if (prison) {
                if (choice == 2 || (choice == 0 && jail_bail(n)) || (choice == 1 && jail_pick_start(n)))
                    open_view(SCREEN_WORLD);
            } else if (ui.dialogPage == DIALOG_WORK) {
                int turnIn = work_turn_in(n, choice), offset = 0;
                while (work_turn_in(n, offset) >= 0)
                    offset++;
                int kind = n == work_contact(town) && choice >= offset ? contract_offer(town, choice - offset) : -1;
                if (turnIn >= 0 && depth_distance(a, b) <= TALK_REACH && conscious(b)) {
                    if (complete_contract(turnIn))
                        open_view(SCREEN_WORLD);
                } else if (kind >= 0 && depth_distance(a, b) <= TALK_REACH && conscious(b)) {
                    if (accept_contract(town, kind))
                        open_view(SCREEN_WORLD);
                } else {
                    ui.dialogPage = 0;
                    ui.selection = 0;
                }
            } else
                switch (dialogue_action(n, choice)) {
                case 16:ui.limbReturn=SCREEN_DIALOG;limb_open(SCREEN_SURGERY);break;
                case 17:if(town>=0&&depth_trade_open(town)){ui.tradeTown=town;ui.tradeSmith=TRADE_APOTHECARY;ui.shopFilter=0;ui.sell=0;open_view(SCREEN_TRADE);}break;
                case 14:ui.dialogPage=4;ui.selection=0;break;
                case 15:open_view(SCREEN_FACTIONS);break;
                case 0:
                    if (population_vendor(n) && conscious(b) && depth_distance(a, b) <= TALK_REACH) {
                        ui.roadTrader = n + 1;
                        ui.tradeSmith = 0;
                        ui.tradeTown = -1;
                        ui.sell = ui.shopFilter = 0;
                        open_view(SCREEN_TRADE);
                    } else if (town >= 0 && depth_trade_open(town)) {
                        ui.tradeTown = town;
                        ui.tradeSmith = npc_role(b) == ROLE_SMITH ? TRADE_SMITH :
                                        npc_role(b) == ROLE_INNKEEPER ? TRADE_INN : TRADE_GENERAL;
                        ui.sell = 0;
                        ui.shopFilter = 0;
                        open_view(SCREEN_TRADE);
                    } else
                        log_msg("No market on this stretch of road.");
                    break;
                case 1:
                    ui.dialogPage = n == work_contact(town) ? DIALOG_WORK : DIALOG_WORK_HINT;
                    ui.selection = 0;
                    break;
                case 2:
                    start_sleep(town);
                    break;
                case 3:
                    doctor_heal(town);
                    break;
                case 4:
                    ui.dialogPage = watch_town(b) >= 0 && wanted_amount(a, b->faction) ? DIALOG_BOUNTY : 5;
                    if (ui.dialogPage == 5 && town == 8 && b->faction == g.townOwner[8])
                        g.frontier.tollKnown = 1;
                    ui.selection = 0;
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
                        ui.dialogPage = 5;
                        ui.selection = 0;
                    }
                    break;
                case 19:
                    ui.dialogPage = DIALOG_IRON;
                    ui.selection = 0;
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
                case 18:
                    ui.dialogPage = 5;
                    ui.selection = 0;
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
