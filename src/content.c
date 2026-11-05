#include "game.h"
#include <stdio.h>
#include <math.h>
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
    "Ranger cape",    "Sword rack",      "Bedroll pack",   "Pilgrim mantle", "Greatsword",
    "Railway maul",   "Crescent glaive", "Anchor cleaver"};
static int family_slot(int f) {
    return f < 32 || f >= 84 ? 0 : f < 48 ? 1 : f < 60 ? 2 : f < 68 ? 3 : f < 76 ? 4 : 5;
}
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
int weapon_family_index(int i) { return i < 32 ? i : 84 + i - 32; }
int backpack_load_percent(int item) {
    static const int percent[] = {80, 100, 85, 60, 100, 100, 70, 100};
    int f = gear_family(item);
    return f >= 76 && f <= 83 ? percent[f - 76] : 100;
}
int backpack_combat_penalty(int item) {
    static const int penalty[] = {2, 3, 0, 7, 0, 1, 4, 0};
    int f = gear_family(item);
    return f >= 76 && f <= 83 ? penalty[f - 76] : 0;
}
int item_effect_value(int item) {
    const ItemDef *d = &itemDefs[item];
    if (d->effect < 0)
        return 0;
    if (d->slot <= 0)
        return d->potency;
    return d->effect == 6    ? d->stealth
           : d->effect == 7  ? d->speed
           : d->effect == 9  ? d->protection
           : d->effect == 10 ? d->capacity
                             : d->potency;
}
int weapon_strength(int it) {
    if (!it || weapon_ranged(it) || itemDefs[it].slot != 0)
        return 0;
    return itemDefs[it].weight * 2;
}
float weapon_handling(const Actor *a, int it) {
    float effective = level(a, STRENGTH) * (.35f + .65f * fminf(a->health[LARM], a->health[RARM]) / 100.f);
    return 1 + fminf(1.4f, fmaxf(0, weapon_strength(it) - effective) * .035f);
}
/* Garment profiles follow their construction, not their catalog position. */
typedef struct {
    int armor, weight, speed, stealth, effect;
} Garment;
static const Garment garments[44] = {
    {8, 3, 0, 0, 9},    {10, 4, 0, 0, 4},    {24, 10, -9, -12, 4},  {8, 3, 1, 2, 11},
    {20, 8, -6, -8, 4}, {14, 6, -3, -6, 4},  {30, 14, -12, -18, 4}, {3, 1, 3, 3, 8},
    {5, 2, 1, 2, 9},    {12, 5, -1, -1, 11}, {22, 10, -8, -10, 9},  {6, 2, 3, 5, 6},
    {18, 7, -5, -6, 4}, {10, 4, 0, 0, 10},   {4, 1, 4, 7, 6},       {26, 12, -10, -15, 4},
    {2, 1, 0, 0, 9},    {2, 1, 0, 0, 9},     {18, 5, -3, -7, 4},    {10, 2, 0, 3, 6},
    {3, 1, 0, 1, 9},    {16, 4, -2, -5, 4},  {5, 1, 1, 5, 6},       {12, 3, -1, -3, 4},
    {4, 1, 0, 0, 9},    {1, 1, 1, 2, 8},     {12, 3, -1, -3, 4},    {20, 6, -4, -8, 4},
    {6, 2, 3, 0, 7},    {1, 1, 4, 4, 6},     {16, 5, -6, -7, 4},    {3, 1, 3, 6, 6},
    {4, 1, 1, 2, 11},   {3, 1, 7, 5, 7},     {8, 3, 1, -3, 7},      {5, 2, 0, 0, 9},
    {5, 2, 2, 3, 6},    {6, 2, 3, 1, 7},     {22, 8, -8, -12, 4},   {4, 1, 3, 6, 6},
    {16, 6, -5, -8, 4}, {3, 1, 2, 3, 8},     {4, 1, 4, 7, 6},       {18, 7, -6, -10, 4}};
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
    static const int wpower[36] = {18, 13, 27, 9,  23, 17, 26, 16, 17, 11, 16, 28, 22, 21, 29, 32, 12, 19,
                                   18, 24, 35, 22, 15, 23, 20, 23, 33, 15, 24, 38, 27, 34, 39, 34, 27, 44};
    static const int weights[36] = {5, 3, 10, 1, 8, 6, 9, 5, 5,  2, 4, 9, 7, 7, 11, 13, 3,  5,
                                    5, 8, 14, 7, 3, 8, 4, 6, 10, 3, 4, 9, 4, 8, 16, 19, 11, 22};
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
                    int wf = f >= 84 ? f - 52 : f;
                    d->power = wpower[wf] + m * 2 + q * 4 - 5;
                    d->weight = weights[wf] + (m == 1 ? 2 : 0) - (m == 4 ? 1 : 0);
                    if (d->weight < 1)
                        d->weight = 1;
                    d->range = (f >= 24 && f < 32)                                  ? 6 + (f % 4) + m * .25f
                               : (f == 4 || f == 5 || f == 6 || f == 16 || f == 20) ? 2.2f
                                                                                    : 1.55f;
                    d->cooldown = (f >= 24 && f < 32) ? 1.8f + (f % 4) * .35f : 1.05f + d->weight * .045f;
                    d->penetration = d->effect == 1 ? 8 + d->potency : 0;
                    d->accuracy = d->effect == 2 ? 3 + d->potency : 0;
                    if (d->effect == 5)
                        d->cooldown *= .79f;
                    if (f >= 84) {
                        static const float reach[] = {2.25f, 1.7f, 2.65f, 1.95f};
                        static const int effects[] = {2, 3, 1, 0};
                        d->range = reach[f - 84];
                        d->effect = effects[f - 84];
                        d->accuracy = f == 84 ? 3 + d->potency : 0;
                        d->penetration = f == 85 ? 20 + d->potency : f == 86 ? 12 + d->potency : 0;
                        d->cooldown = 1.3f + d->weight * .055f;
                    }
                    d->price = 70 + f * 4 + m * 55 + q * q * 110 + d->power * 5;
                } else {
                    if (s != 5) {
                        const Garment *v = &garments[f - 32];
                        static const int materialArmor[6] = {0, 2, 1, 4, 3, 6};
                        int armorScale = v->armor / 10, gradeScale = v->armor / 12;
                        if (armorScale < 1)
                            armorScale = 1;
                        if (gradeScale < 1)
                            gradeScale = 1;
                        d->armor = v->armor +
                                   (v->armor < 8 ? materialArmor[m] / 2 : materialArmor[m] * armorScale) +
                                   q * gradeScale;
                        d->weight = v->weight + (m == 1 ? (v->weight + 3) / 4 : m == 2 ? v->weight / 5 : 0);
                        if (m == 4)
                            d->weight -= v->weight > 4 ? v->weight / 5 : 0;
                        d->speed = v->speed;
                        d->stealth = v->stealth + (m == 4 ? 2 : 0);
                        d->effect = v->effect;
                        if (d->effect == 9)
                            d->protection = 15 + d->potency * 3;
                        if (d->effect == 10)
                            d->capacity = 6 + q * 2;
                        if (d->effect == 7)
                            d->speed += q;
                        if (d->effect == 6)
                            d->stealth += q;
                        d->price = 45 + d->armor * 12 + m * 40 + q * q * 95 + (f % 12) * 8;
                    }
                    if (s == 5) {
                        static const int carry[] = {16, 0, 10, 36, 0, 4, 24, 0};
                        static const int weight[] = {2, 2, 1, 6, 1, 3, 4, 1};
                        static const int stealth[] = {-3, -12, 0, -15, 6, -4, -8, 3};
                        static const int effect[] = {10, 4, 8, 10, 9, 4, 11, 6};
                        static const int price[] = {120, 90, 180, 230, 150, 170, 200, 120};
                        int b = f - 76;
                        d->weight = weight[b] + (m == 1);
                        d->armor = d->speed = 0;
                        d->capacity = carry[b] ? carry[b] + q * 2 : 0;
                        d->stealth = stealth[b] + (m == 4 ? 2 : 0);
                        d->effect = effect[b];
                        d->protection = f == 80 ? 15 + d->potency * 3 : 0;
                        d->price = price[b] + m * 20 + q * q * 90;
                    }
                }
                snprintf(descriptions[id], 112, "%s +%d. %s %d. Weight %d. %s craft.", effectNames[d->effect],
                         item_effect_value(id), s == 0 ? "Damage" : "Armor", s == 0 ? d->power : d->armor,
                         d->weight, materialNames[m]);
                if (s == 5)
                    snprintf(descriptions[id], 112,
                             "Carried load %d%%; melee/defence -%d; sneak %+d. %s +%d.",
                             backpack_load_percent(id), backpack_combat_penalty(id), d->stealth,
                             effectNames[d->effect], item_effect_value(id));
            }
}

const char *guideTitles[GUIDE_PAGES] = {"CONTROLS",        "YOUR DRIFTER", "THE BORDERLANDS",    "COMBAT",
                                        "WOUNDS & RESCUE", "FOOD & SLEEP", "EQUIPMENT",          "MERCHANTS",
                                        "FORGING GEAR",    "CRAFTING",     "YOUR OUTPOST",       "SQUAD JOBS",
                                        "CRIME & STEALTH", "FACTIONS",     "CONTRACTS",          "SAVES",
                                        "WEATHER",         "CAPTIVITY",    "BUILDING INTERIORS", "BACKPACKS"};
const char *guideBodies[GUIDE_PAGES] = {
    "\011 or analog: move in screen direction. \001: nearby interaction. \004: pack. \003: "
    "fight/passive. \002: sneak. \005: next squad member. \006: tactical pause. \010: world atlas. \007: "
    "expedition menu.\nMenus pause the world. \010 inside a menu opens its guide. \005/\006 switch inventory "
    "tabs or guide pages. Move to cancel a work order or travel route.",
    "New expedition opens character creation. Choose a name, origin, ancestry, body, skin, hairstyle, hair "
    "color, beard and scars. \014 adjusts each choice; \001 edits your name or begins.\nOrigins change "
    "equipment, supplies and skills. Outlaws begin with a bounty. Caravan starts three companions. Ancestry "
    "changes starting strengths. A town barber can change your appearance later.",
    "The 1536 by 1536 borderlands contain sixteen settlements and nine terrain regions. Roads connect towns. "
    "Rivers have fords near roads; hills rise into mesas and snowfields.\n\010 opens the atlas. Move the "
    "cursor; \001 orders travel. \004 switches local/world views. \005/\006 selects known towns. \003 "
    "reveals surveyed geography after buying a "
    "chart. Town ownership, prosperity and unrest change with world events. Camps and depleted deposits "
    "persist.",
    "\003 toggles fighting. Move into weapon reach to attack automatically; followers engage nearby "
    "threats. \006 pauses combat for orders. Heavy steel needs strength equal to twice its weight. Weak or "
    "injured arms lengthen preparation and recovery. Mauls pierce armor; glaives reach farther; greatswords "
    "hit hard; anchor cleavers bleed. Guns use cartridges; "
    "bows use bolts.\nArmor protects specific body locations. Piercing weapons ignore some armor; serrated "
    "blades bleed; staggering hits interrupt. Skills improve through use. Retreat from stronger opponents "
    "and use terrain.",
    "Six body parts, blood and fatigue are separate. Head or chest failure knocks a survivor out. Bleeding "
    "can kill an unconscious character. \004, Body, \001 treats the selected survivor; \001 near an injured "
    "ally treats or carries them.\nDressings stop bleeding and restore some health. Sleeping restores the "
    "rest. A doctor charges for full treatment. Lost limbs require a mechanical replacement. Death is "
    "permanent; dismiss the body from Squad to free its slot.",
    "Rations are eaten below forty hunger: first your pack, then a companion backpack or owned depot within "
    "three meters. Water supports "
    "farms; tonics restore blood and treat exposure. Starvation damages the chest.\nAn inn bed costs 25 "
    "silver and starts safe rest. Camp shelters are free. Sleeping reduces fatigue and restores wounds once "
    "bleeding stops and hunger is above twenty. Move to wake. The sun completes a day in thirty minutes of "
    "simulation; menus and "
    "tactical pause stop time.",
    "Six slots: weapon, torso, head, boots, legs, back. Pack \001 equips; \003 passes one to an ally; \004 "
    "switches carried/worn gear. \001 on worn gear removes it if space allows. WORN / TRY ON compares fit "
    "and actual bonuses.\nPlate protects more but slows movement and hinders stealth. Cloth is light; "
    "travel gear favors weather or movement. Higher grades improve the same role. Torso armor covers "
    "chest/arms; headgear covers head; legs gain leg armor plus half boot armor. MOVE% is the worn speed "
    "bonus. Weight above capacity slows you further.",
    "Talk to a trader and choose Trade. \014 switches buying and selling; \005/\006 filters supplies, "
    "weapons, armor or backpacks. \001 trades one item. Shops have finite stocks and refresh daily with "
    "different "
    "gear.\nPrices depend on quality, faction relations, local prosperity. Hostile factions refuse trade. "
    "Selling stolen goods back to their owner can expose you; Black Mesa fences accept them. The item "
    "details show its stats and effects before purchase.",
    "Visit a forge, then choose Gear forge from the expedition menu or smith dialogue. Choose slot, design, "
    "material and quality with \014. The preview updates before you spend anything.\n\001 on Forge makes "
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
    "sixteen item stacks of any type. \001 opens storage; \014 switches deposit/withdraw. Camps attract "
    "raiders, so keep medicine and a retreat route.",
    "\004, Squad: choose a survivor with \014, then assign a job with \001. Follow trails the "
    "selected leader; Hold stays put. Mine gathers local deposits; Farm waters and harvests; Medic treats "
    "injured allies.\nSmelt and Cook use nearby stations and can collect ingredients from a nearby depot. "
    "Haul moves pack resources into storage. Each worker needs supplies and food. Select a worker and move "
    "to interrupt their current action. Up to eight living members may join.",
    "\002 sneaks. Detection depends on stealth skill, equipment, distance and weather. Sneak and \001 near a "
    "civilian to attempt pickpocketing. Owned salvage also counts as theft; locked containers require tools "
    "and time.\nWitnesses create a faction penalty and bounty. A theft can fail, drawing guards. Stolen "
    "stacks retain an owner until fenced. Paying a bounty requires a town official. Guards may imprison a "
    "defeated wanted survivor; breaking out or paying bail is possible.",
    "Compact guards roads and wells; Rust Caravans trade salvage. Shogunate markets sell lamellar and "
    "glaives. Ash monks sell medicine; Black Mesa fences stolen goods. Vultures raid; wild packs hunt.\n"
    "Contracts raise standing; theft lowers it. The Factions ledger shows specialties and controlled towns. "
    "Hired guards cost 240 silver for escort or 180 for camp duty: one simulated day. They fight and "
    "bandage independently, keep their gear, wait outside interiors and return home on expiry.",
    "Marshals offer delivery, supply, hunt and relic contracts. Accept through dialogue; track "
    "objectives in the Journal. Deliveries require reaching another town. Supplies require the listed items. "
    "Hunts count victories after acceptance.\nReturn to the issuing settlement and claim in the Journal with "
    "\001. Rewards grant silver, reputation and experience. Contracts persist in saves. Complete contracts, "
    "research relic technology and survey the frontier to advance the expedition legacy. \003 locates an "
    "active contract on the atlas; \001 there starts travel when ready.",
    "Six manual slots plus autosave. \007, Save or Load opens a ledger with name, day, party size and "
    "silver. Choose a slot; \001 asks for confirmation. Save-and-title uses the selected manual slot.\nSaves "
    "use version, size and checksum validation and preserve a previous .bak file. Invalid files never "
    "replace your running expedition. Older saves through 0.4.1 migrate on load. Back them up before "
    "overwriting. "
    "Autosave is slot zero and is never used for manual saving.",
    "Dust, rain, acid mist and snow change visibility, terrain colors and survival pressure. Storms reduce "
    "detection and ranged accuracy. Acid and cold build exposure outdoors; shelters and sealed gear protect "
    "you.\nTonics and rest help recover. Rain irrigates farms. The northern highlands are colder; the fen "
    "produces acid mist. Weather changes as time passes. The world ledger reports storms and settlement "
    "events so you can plan supplies and routes.",
    "Wanted survivors defeated by lawful guards may wake in town custody. They lose stolen cargo and must "
    "serve time, pay bail or attempt an escape. A conscious free companion can still be selected.\nWhen "
    "imprisoned, movement is restricted to the holding area. \001 opens the custody dialogue. Paying clears "
    "the "
    "local bounty. Escape attempts use lockpicking, may fail and raise the bounty. The prison timer, "
    "injuries and inventory persist through saving.",
    "Approach a house, inn, shop, tower or workshop door and press \001 to enter. Walk around the room with "
    "\011 or analog. Furniture blocks movement; use the aisles. \001 near the door returns outside.\n"
    "Counters open local trade. Beds cost 25 silver in town; owned beds are free. Workshop stations open "
    "forging, research or storage. Indoor time passes: hunger, bleeding and healing continue. The rest of "
    "your squad remains outside.\nSave games preserve the building and indoor position. Switching squad "
    "members leaves the room. Interiors shelter you from weather; hostile occupants refuse entry.",
    "Buy travel packs, medicine rolls, ore harnesses and bedroll packs from the BACKPACKS shop tab. "
    "Preview before buying. Other back gear rotates daily.\n"
    "CARGO: travel 80%, medicine 85%, ore 60%, bedroll 70% of carried weight. Worn gear and carried "
    "people keep full weight. CARRY adds load allowance; LOAD previews a swap. All packs use the same "
    "24 inventory slots.\n"
    "FIGHT reduces melee attack/defence: travel -2, medicine 0, ore -7, bedroll -4. Bulky packs hurt "
    "stealth. Medicine aids first aid; bedrolls improve rest but are not beds. Unworn packs give no bonus."};
