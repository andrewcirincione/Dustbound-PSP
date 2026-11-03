#include "game.h"
#include <stdio.h>
#include <string.h>

extern const ItemDef baseDefs[BASE_ITEMS];
ItemDef itemDefs[ITEMS];
static char names[ITEMS][40], descriptions[ITEMS][112];
const char *factionNames[FACTIONS] = {"FREE DRIFTERS",  "FRONTIER COMPACT", "RUST CARAVANS", "RED VULTURES",
                                      "IRON SHOGUNATE", "ASH MONASTERY",    "BLACK MESA",    "WILD PACKS"};
const uint32_t factionColors[FACTIONS] = {0xa9b4a2, 0x9c753c, 0x587f75, 0xa44132,
                                          0x823b3b, 0xc0b48b, 0x655679, 0x777758};
const char *regionNames[9] = {"THE CINDER FRONTIER", "COPPER BADLANDS",  "DEAD SALT BASIN",
                              "THE REED FEN",        "ASHEN SCAR",       "IRON MESA",
                              "PALE HIGHLANDS",      "BLACK PINE REACH", "THE RED CANYONS"};
const char *weatherNames[5] = {"CLEAR", "DUST STORM", "RAIN", "ACID MIST", "SNOW"};
const char *originNames[5] = {"WANDERER", "RONIN", "PROSPECTOR", "OUTLAW", "CARAVAN"};
const char *raceNames[3] = {"LOWLANDER", "ASHLANDER", "AUGMENTED"};
const char *materialNames[MATERIALS] = {"Scrap", "Iron", "Bronze", "Steel", "Black", "Relic"};
const char *qualityNames[QUALITIES] = {"Worn", "Field", "Fine", "Master"};
const char *effectNames[12] = {"Serrated",    "Piercing",    "Balanced",     "Staggering",
                               "Guarding",    "Quickdraw",   "Quiet",        "Striding",
                               "Field medic", "Dust sealed", "Load bearing", "Recovery"};
const char *familyNames[FAMILIES] = {
    "Katana",         "Wakizashi",       "Nodachi",        "Tanto",          "Naginata",
    "Yari",           "Nagamaki",        "Kusarigama",     "Frontier sabre", "Bowie knife",
    "Trail machete",  "Butcher cleaver", "War pick",       "Boarding axe",   "Kanabo",
    "Tetsubo",        "Quarterstaff",    "Hook blade",     "Ring sabre",     "Broken falchion",
    "Horse cutter",   "Saw sword",       "Twin sickle",    "Spiked club",    "Scout bow",
    "Repeater bow",   "Heavy arbalest",  "Hand crossbow",  "Longbow",        "Ranger rifle",
    "Dragoon pistol", "Scattergun",      "Trail duster",   "Ronin coat",     "Lamellar do",
    "Patchwork vest", "Brigandine",      "Bone harness",   "Iron cuirass",   "Monk robes",
    "Ash cloak",      "Gambeson",        "Plated duster",  "Reed jacket",    "Scale haori",
    "Bandolier coat", "Scavenger wraps", "Captain armor",  "Sedge kasa",     "Frontier hat",
    "Horned kabuto",  "Menpo hood",      "Dust turban",    "Bowl helm",      "Ranger hood",
    "Oni mask",       "Goggle cap",      "Monk cowl",      "Iron jingasa",   "Crested helm",
    "Riding boots",   "Straw sandals",   "Plated greaves", "Split tabi",     "Wrapped boots",
    "Scout shoes",    "Spurred boots",   "Ash gaiters",    "Hakama",         "Trail trousers",
    "Plated tassets", "Reed leggings",   "Armored chaps",  "Monk skirt",     "Bandit wraps",
    "Scale guards",   "Travel pack",     "Banner frame",   "Medicine roll",  "Ore harness",
    "Ranger cape",    "Sword rack",      "Bedroll pack",   "Pilgrim mantle"};
static int family_slot(int f) { return f < 32 ? 0 : f < 48 ? 1 : f < 60 ? 2 : f < 68 ? 3 : f < 76 ? 4 : 5; }
int gear_id(int f, int m, int q) {
    if (f < 0 || f >= FAMILIES || m < 0 || m >= MATERIALS || q < 0 || q >= QUALITIES)
        return NONE;
    return BASE_ITEMS + (f * MATERIALS + m) * QUALITIES + q;
}
int gear_family(int it) {
    return it >= BASE_ITEMS && it < ITEMS ? (it - BASE_ITEMS) / (MATERIALS * QUALITIES) : -1;
}
int weapon_ranged(int it) { return it == CROSSBOW || (gear_family(it) >= 24 && gear_family(it) < 32); }
int gear_bonus(const Actor *a, int stat) {
    int v = 0;
    for (int i = 0; i < GEARS; i++) {
        const ItemDef *d = &itemDefs[a->gear[i]];
        v += stat == 0   ? d->accuracy
             : stat == 1 ? d->speed
             : stat == 2 ? d->stealth
             : stat == 3 ? d->capacity
             : stat == 4 ? d->protection
                         : (d->effect == (stat == 6 ? 11 : 8) ? d->potency : 0);
    }
    return v;
}
void content_init(void) {
    static int initialized;
    if (initialized)
        return;
    initialized = 1;
    memcpy(itemDefs, baseDefs, sizeof(baseDefs));
    for (int i = 0; i < BASE_ITEMS; i++) {
        itemDefs[i].family = -1;
        itemDefs[i].effect = -1;
        itemDefs[i].range = i == CROSSBOW ? 7 : 1.55f;
        itemDefs[i].cooldown = i == CROSSBOW ? 2.3f : 1.2f + itemDefs[i].weight * .025f;
        itemDefs[i].trim = 0xb8a272;
    }
    static const uint32_t metals[6] = {0x78614a, 0x555250, 0x80613e, 0x858582, 0x292b2c, 0x626977};
    static const uint32_t cloth[6] = {0x77634f, 0x615a4e, 0x8e653f, 0x666058, 0x302e2b, 0x4e535b};
    static const int wpower[32] = {18, 13, 27, 9,  23, 17, 26, 16, 17, 11, 16, 28, 22, 21, 29, 32,
                                   12, 19, 18, 24, 35, 22, 15, 23, 20, 23, 33, 15, 24, 38, 27, 34};
    static const int weights[32] = {5, 3, 10, 1, 8,  6, 9, 5, 5, 2, 4,  9, 7, 7, 11, 13,
                                    3, 5, 5,  8, 14, 7, 3, 8, 4, 6, 10, 3, 4, 9, 4,  8};
    for (int f = 0; f < FAMILIES; f++)
        for (int m = 0; m < MATERIALS; m++)
            for (int q = 0; q < QUALITIES; q++) {
                int id = gear_id(f, m, q), s = family_slot(f);
                ItemDef *d = &itemDefs[id];
                memset(d, 0, sizeof(*d));
                snprintf(names[id], 40, "%s %s %s", qualityNames[q], materialNames[m], familyNames[f]);
                d->name = names[id];
                d->desc = descriptions[id];
                d->slot = s;
                d->family = f;
                d->material = m;
                d->quality = q;
                d->effect = s == 0 ? (f + m + q) % 6 : 4 + (f + m + q) % 8;
                d->potency = 2 + q * 2 + m;
                d->color = s == 0 || f == 34 || f == 38 || s == 2 ? metals[m] : cloth[m];
                d->trim = q == 3 ? 0xc9a452 : q == 2 ? 0x9c978a : q == 1 ? 0x866747 : 0x574839;
                if (s == 0) {
                    d->power = wpower[f] + m * 2 + q * 4 - 5;
                    d->weight = weights[f] + (m == 1 ? 2 : 0) - (m == 4 ? 1 : 0);
                    if (d->weight < 1)
                        d->weight = 1;
                    d->range = f >= 24                                              ? 6 + (f % 4) + m * .25f
                               : (f == 4 || f == 5 || f == 6 || f == 16 || f == 20) ? 2.2f
                                                                                    : 1.55f;
                    d->cooldown = f >= 24 ? 1.8f + (f % 4) * .35f : 1.05f + d->weight * .045f;
                    d->penetration = d->effect == 1 ? 8 + d->potency : 0;
                    d->accuracy = d->effect == 2 ? 3 + d->potency : 0;
                    if (d->effect == 5)
                        d->cooldown *= .79f;
                    d->price = 70 + f * 4 + m * 55 + q * q * 110 + d->power * 5;
                } else {
                    d->armor = 2 + (f % 8) * 2 + m * 2 + q * 3;
                    d->weight = 1 + (f % 5) + (m == 1 ? 2 : 0) + (s == 1 ? 2 : 0);
                    if (s == 5) {
                        d->armor /= 2;
                        d->capacity = 8 + (f - 76) * 2 + m + q * 4;
                    }
                    if (d->effect == 6)
                        d->stealth = 4 + d->potency;
                    if (d->effect == 7)
                        d->speed = 3 + d->potency;
                    if (d->effect == 9)
                        d->protection = 15 + d->potency * 3;
                    if (d->effect == 10)
                        d->capacity += 8 + d->potency * 2;
                    if (m == 4)
                        d->stealth += 3;
                    d->price = 45 + d->armor * 12 + m * 40 + q * q * 95 + (f % 12) * 8;
                }
                snprintf(descriptions[id], 112, "%s +%d. %s %d. Weight %d. %s craft.", effectNames[d->effect],
                         d->potency, s == 0 ? "Damage" : "Armor", s == 0 ? d->power : d->armor, d->weight,
                         materialNames[m]);
            }
}

const char *guideTitles[GUIDE_PAGES] = {
    "CONTROLS",        "YOUR DRIFTER", "THE BORDERLANDS", "COMBAT",   "WOUNDS & RESCUE", "FOOD & SLEEP",
    "EQUIPMENT",       "MERCHANTS",    "FORGING GEAR",    "CRAFTING", "YOUR OUTPOST",    "SQUAD JOBS",
    "CRIME & STEALTH", "FACTIONS",     "CONTRACTS",       "SAVES",    "WEATHER",         "CAPTIVITY"};
const char *guideBodies[GUIDE_PAGES] = {
    "D-pad or analog: move in screen direction. X: nearby interaction. Triangle: pack. Square: "
    "fight/passive. Circle: sneak. L: next squad member. R: tactical pause. Select: world atlas. Start: "
    "expedition menu.\nMenus pause the world. Select inside a menu opens its guide. L/R switch inventory "
    "tabs or guide pages. Move to cancel a work order or travel route.",
    "New expedition opens character creation. Choose a name, origin, ancestry, body, skin, hairstyle, hair "
    "color, beard and scars. Left/right adjusts each choice; X edits your name or begins.\nOrigins change "
    "equipment, supplies and skills. Outlaws begin with a bounty. Caravan starts three companions. Ancestry "
    "changes starting strengths. A town barber can change your appearance later.",
    "The 384 by 384 borderlands contain sixteen settlements and nine terrain regions. Roads connect towns. "
    "Rivers have fords near roads; hills rise into mesas and snowfields.\nSelect opens the atlas. Move the "
    "cursor; X orders travel. L/R selects known towns. Square reveals surveyed geography after buying a "
    "chart. Town ownership, prosperity and unrest change with world events. Camps and depleted deposits "
    "persist.",
    "Square toggles fighting. Move into weapon reach to attack automatically; followers engage nearby "
    "threats. R pauses combat for orders. Heavy weapons hit harder but recover slowly. Guns use cartridges; "
    "bows use bolts.\nArmor protects specific body locations. Piercing weapons ignore some armor; serrated "
    "blades bleed; staggering hits interrupt. Skills improve through use. Retreat from stronger opponents "
    "and use terrain.",
    "Six body parts, blood and fatigue are separate. Head or chest failure knocks a survivor out. Bleeding "
    "can kill an unconscious character. Triangle, Body, X treats the selected survivor; X near an injured "
    "ally treats or carries them.\nDressings stop bleeding and restore some health. Sleeping restores the "
    "rest. A doctor charges for full treatment. Lost limbs require a mechanical replacement. Death is "
    "permanent; dismiss the body from Squad to free its slot.",
    "Rations are eaten automatically below forty hunger. Carry food on each squad member. Water supports "
    "farms; tonics restore blood and treat exposure. Starvation damages the chest.\nAn inn bed costs 25 "
    "silver and starts safe rest. Camp shelters are free. Sleeping reduces fatigue and restores wounds once "
    "bleeding stops. Move to wake. The sun completes a day in thirty minutes of simulation; menus and "
    "tactical pause stop time.",
    "Six visible slots: weapon, torso, head, boots, legs and back. The catalog has 2,016 combinations across "
    "84 silhouettes, six materials and four quality grades. Every variant has visible material/quality "
    "details and mechanical differences.\nPack X equips or uses; Square passes one item to an ally. Triangle "
    "switches between carried items and worn equipment. Inspect shows damage, armor and effects; X on worn "
    "gear removes it if space allows. Weight above capacity slows movement.",
    "Talk to a trader and choose Trade. Left/right switches buying and selling; L/R filters supplies, "
    "weapons or armor. X trades one item. Shops have finite stocks and refresh daily with different "
    "gear.\nPrices depend on quality, faction relations, local prosperity. Hostile factions refuse trade. "
    "Selling stolen goods back to their owner can expose you; Black Mesa fences accept them. The item "
    "details show its stats and effects before purchase.",
    "Visit a forge, then choose Gear forge from the expedition menu or smith dialogue. Choose slot, design, "
    "material and quality with left/right. The preview updates before you spend anything.\nX on Forge makes "
    "the selected design. Metal, cloth and timber costs scale with material and grade; relic gear also needs "
    "a relic. Higher grades require smithing experience and research. Missing materials or a full pack "
    "cancel the transaction without charging you.",
    "Crafting recipes make supplies, refined metal, ammunition and prosthetics. A forge smelts ore and "
    "shapes weapons. A campfire cooks grain or meat with water. A research bench makes dressings.\nStand "
    "close to the station. The recipe screen lists costs and unlocks. Research consumes relics and silver in "
    "three tiers. Skills rise when crafting succeeds. A shared depot supplies worker jobs, but hand crafting "
    "uses the selected survivor's pack.",
    "Build outside settlement boundaries. Choose a structure and place it beside the selected character. "
    "Buildings cannot overlap; materials are charged only after validation.\nShelters heal; farms need water "
    "and yield grain; wind pumps irrigate nearby crops. Forges and benches unlock production. Depots hold "
    "sixteen item stacks of any type. X opens storage; left/right switches deposit/withdraw. Camps attract "
    "raiders, so keep medicine and a retreat route.",
    "Triangle, Squad: choose a survivor with left/right, then assign a job with X. Follow trails the "
    "selected leader; Hold stays put. Mine gathers local deposits; Farm waters and harvests; Medic treats "
    "injured allies.\nSmelt and Cook use nearby stations and can collect ingredients from a nearby depot. "
    "Haul moves pack resources into storage. Each worker needs supplies and food. Select a worker and move "
    "to interrupt their current action. Up to eight living members may join.",
    "Circle sneaks. Detection depends on stealth skill, equipment, distance and weather. Sneak and X near a "
    "civilian to attempt pickpocketing. Owned salvage also counts as theft; locked containers require tools "
    "and time.\nWitnesses create a faction penalty and bounty. A theft can fail, drawing guards. Stolen "
    "stacks retain an owner until fenced. Paying a bounty requires a town official. Guards may imprison a "
    "defeated wanted survivor; breaking out or paying bail is possible.",
    "The Frontier Compact guards the old roads. Rust Caravans trade salvage. The Iron Shogunate rules the "
    "east; Ash monks shelter pilgrims. Black Mesa fences stolen goods. Red Vultures raid; wild packs "
    "hunt.\nCompleted contracts improve standing. Theft worsens it. The Factions ledger shows standing and "
    "controlled towns. Settlement unrest affects prosperity and prices. Fighting a faction can close its "
    "shops; peaceful jobs offer a way back.",
    "Town residents offer delivery, supply, hunt and relic contracts. Accept through dialogue; track "
    "objectives in the Journal. Deliveries require reaching another town. Supplies require the listed items. "
    "Hunts count victories after acceptance.\nReturn to the issuing settlement and claim in the Journal with "
    "X. Rewards grant silver, reputation and experience. Contracts persist in saves. Complete contracts, "
    "research relic technology and survey the frontier to advance the expedition legacy.",
    "Six manual slots plus autosave. Start, Save or Load opens a ledger with name, day, party size and "
    "silver. Choose a slot; X asks for confirmation. Save-and-title uses the selected manual slot.\nSaves "
    "use version, size and checksum validation and preserve a previous .bak file. Invalid files never "
    "replace your running expedition. Version 0.1 saves migrate on load. Back them up before overwriting. "
    "Autosave is slot zero and is never used for manual saving.",
    "Dust, rain, acid mist and snow change visibility, terrain colors and survival pressure. Storms reduce "
    "detection and ranged accuracy. Acid and cold build exposure outdoors; shelters and sealed gear protect "
    "you.\nTonics and rest help recover. Rain irrigates farms. The northern highlands are colder; the fen "
    "produces acid mist. Weather changes as time passes. The world ledger reports storms and settlement "
    "events so you can plan supplies and routes.",
    "Wanted survivors defeated by lawful guards may wake in town custody. They lose stolen cargo and must "
    "serve time, pay bail or attempt an escape. A conscious free companion can still be selected.\nWhen "
    "imprisoned, movement is restricted to the holding area. X opens the custody dialogue. Paying clears the "
    "local bounty. Escape attempts use lockpicking, may fail and raise the bounty. The prison timer, "
    "injuries and inventory persist through saving."};
