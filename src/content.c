#include "game.h"
#include "combat_pose.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "weapon_design.h"

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
    return it >= CATALOG_ITEMS && it < ITEMS ? itemDefs[it].family : it >= BASE_ITEMS && it < CATALOG_ITEMS ? (it - BASE_ITEMS) / (MATERIALS * QUALITIES) : -1;
}
int weapon_rapier(int it) { return (it>=WEAPON_ITEMS&&arsenal_type(it)==37)||(it>=RAPIER_ITEMS&&it<RAPIER_ITEMS+24)||it==UNIQUE_ITEMS+1; }
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
    float effective = effective_level(a, STRENGTH) * (.35f + .65f * limb_arm_condition(a,it));
    return (1 + fminf(1.4f, fmaxf(0, weapon_strength(it) - effective) * .035f))*(1-(weapon_class_level(a,weapon_class(it))-1)*.0015f)*(limb_arms(a)==1&&combat_two_hands(it)?1.65f:1);
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
    itemDefs[BEDROLL_ITEM] = (ItemDef){"Bedroll", "Equip / Use to lay it out. Pick it up when done.",
                                     85, 3, -1, 0, 0, 0x9d8469, .family = -1, .effect = -1};
    itemDefs[SLAVE_GARB] = (ItemDef){"Slave garb", "Rough clothing issued in the work camp.",
                                    4, 1, 1, 0, 1, 0xa99b79, .family = -1, .effect = -1};
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
    static const char *special[24]={"Duelist's needle","Salt reaper","Gatebreaker","Pilgrim's reach","Trench hook","Ash longrifle","Caravan repeater","Raider's falx",
        "Stormskin coat","Scout's reedweave","Siege cuirass","Surgeon's mantle","Gutter brigandine","Frostwatch haori","Runner's wraps","Marshal's harness",
        "Vulture King's cleaver","Shogun's oathblade","Mesa executioner","Ashkeeper's staff","Iron captain's carapace","Reed stalker's shroud","Frostfang trophy","Caravan master's coat"};
    static const int family[24]={3,86,85,5,17,29,25,87,32,43,38,39,36,44,46,45,87,0,84,16,47,40,55,42};
    for(int i=0;i<24;i++) {
        int id=CATALOG_ITEMS+i,f=family[i];ItemDef*d=&itemDefs[id];
        *d=itemDefs[gear_id(f,i>=16?5:3,i>=16?3:2)];
        d->name=special[i];d->desc=descriptions[id];d->trim=i>=16?0xe4bd70:0x81b2b3;
        if(i<8){d->power+=4;d->penetration+=i==2?18:3;d->range+=i==3?.8f:0;if(i==0){d->cooldown=.62f;d->weight=2;d->power-=8;}}
        if(i>=8&&i<16){d->armor+=i==10?16:3;d->protection+=i==8||i==13?30:0;d->stealth+=i==9||i==14?12:0;d->potency+=i==11?12:0;d->capacity+=i==15?20:0;}
        if(i>=16){d->power+=d->slot==0?10:0;d->armor+=d->slot?10:0;d->potency+=5;}
        d->price+=i>=16?1400:350;
        snprintf(descriptions[id],112,"%s. %s %d; weight %d. %s %d.",i>=16?"Signature trophy":"Specialist equipment",d->slot==0?"Damage":"Armor",d->slot==0?d->power:d->armor,d->weight,effectNames[d->effect],item_effect_value(id));
    }

    /* Two additional weapon types, each with all 24 material/quality variants. */
    for(int type=0;type<2;type++)for(int m=0;m<MATERIALS;m++)for(int q=0;q<QUALITIES;q++) {
        int id=OLD_ITEMS+type*24+m*4+q;ItemDef*d=&itemDefs[id];
        *d=itemDefs[gear_id(type?0:18,m,q)];
        snprintf(names[id],40,"%s %s %s",qualityNames[q],materialNames[m],type?"Rapier":"Scimitar");d->name=names[id];d->desc=descriptions[id];
        d->power=(type?13:21)+m*2+q*4;d->weight=type?3:6;d->range=type?1.95f:1.7f;d->cooldown=type?.7f:1.1f;
        d->effect=type?1:0;d->potency=4+q*2+m;d->penetration=type?12+q*3:0;d->accuracy=type?8:2;d->price=100+m*60+q*q*100+d->power*8;
        snprintf(descriptions[id],112,type?"Thrusting blade. Fast, accurate, piercing; weight %d.":"Curved cutting blade. Serrated edge; weight %d.",d->weight);
    }
    static const char*unique[16]={"Saint's flensing knife","Gate commander's rapier","Red feast scimitar","Ferry watch greatsword","Cinder prospector's pick","Saltwind longbow","Black warrant rifle","Dusk warden's maul","Saint's ash vestments","Feastmaster's bone mantle","Gatekeeper's lamellar","Smuggler's reed coat","Saltwalker's plated coat","Caravan surgeon's robes","Prospector's ore frame","Pilgrim's veil"};
    static const int uf[16]={9,0,18,84,12,28,29,85,39,37,34,43,42,39,79,57};
    for(int i=0;i<16;i++){int id=UNIQUE_ITEMS+i;ItemDef*d=&itemDefs[id];*d=itemDefs[gear_id(uf[i],4,3)];d->name=unique[i];d->desc=descriptions[id];d->trim=0xd5ba78;d->power+=i<8?9:0;d->armor+=i>=8&&i!=14?8:0;d->stealth+=i==11||i==15?14:0;d->capacity+=i==14?12:0;d->price+=1200;snprintf(descriptions[id],112,"Named frontier equipment. %s +%d; weight %d.",effectNames[d->effect],item_effect_value(id),d->weight);}

    civil_content();
    armor_content();
    limbs_content();
    portion_content();
    itemDefs[DISPATCH_ITEM] = (ItemDef){"Sealed dispatch", "Road reports for the destination watch.", 0, 0, -1, 0, 0, 0xd5bb87, .family=-1, .effect=-1};
    for(int i=BASE_ITEMS;i<ITEMS;i++)if(itemDefs[i].slot==0){
        WeaponDesign d=weapon_design(i);
        if(!weapon_ranged(i))itemDefs[i].range*=1+(d.length-1)*.6f;
        itemDefs[i].cooldown*=1+(d.length-1)*.18f+(d.width-1)*.08f;
    }
    arsenal_content();
}

const char *guideTitles[GUIDE_PAGES] = {"CONTROLS",        "YOUR DRIFTER", "THE BORDERLANDS",    "COMBAT",
                                        "WOUNDS & RESCUE", "FOOD & SLEEP", "EQUIPMENT",          "MERCHANTS",
                                        "FORGING GEAR",    "RESEARCH",     "CAMP & HOME",       "SQUAD JOBS",
                                        "CRIME & STEALTH", "FACTIONS",     "CONTRACTS",          "SAVES",
                                        "WEATHER",         "CAPTIVITY",    "BUILDING INTERIORS", "BACKPACKS", "FINDING WORK", "ORDERS & GROUPS", "CAMP COOKING", "TRAUMA & REPAIR", "ARMOR LAYERS"};
const char *guideBodies[GUIDE_PAGES] = {
    "Analog: move. D-pad Up: toggle limb health, blood, food, fatigue and load. Down: skills. Left: worn gear. Right: squad management.\n\001: interact. \004: pack. \003: "
    "fight/passive. Hold L + Square: choose someone to attack (also Start > Squad). \002: sneak. \005: next squad member. \006: tactical pause. \010: world atlas. \007: "
    "pause menu: \005/\006 change Pause, Squad, World and Camp & Home tabs. D-pad navigates menus.\nMenus pause time. \010 opens help. \005/\006 switch members or guide pages. Cross opens nearby actions. Pause with R, choose an action to queue it, then R resumes. Four actions can wait; moving cancels them. Analog movement cancels work. Map pins never move you.",
    "New expedition opens character creation. Choose a name, origin, species, body, skin, hairstyle, hair "
    "color and beard. \014 adjusts each choice. \001 edits your name or begins.\nOrigins change "
    "equipment, supplies and skills. Outlaws begin with a bounty. Species "
    "changes health, carry, speed, appetite and starting skills. The preview shows your starting values. Clockworks need spare parts, not food or bandages. A town barber can change your appearance later.",
    "The 1536 by 1536 borderlands contain nineteen settlements and nine terrain regions. Roads connect towns. "
    "Rivers have fords near roads; hills rise into mesas and snowfields.\n\010 opens the atlas. "
    "D-pad selects a known town; \005/\006 changes pages. The blue triangle is you; "
    "white squares are towns. World activity records last-seen sightings, not live tracking. "
    "\001 places a reference pin. \004 opens the local view. \005/\006 selects known towns. \003 "
    "reveals surveyed geography after buying a "
    "chart. Town ownership, prosperity and unrest change with world events. Depleted deposits persist. Player camps expire after a day away.",
    "\003 toggles fighting. L + Square opens opponents; Cross orders an attack, including neutrals (assault adds bounty). Small feet marks: teal line friendly, gold dot neutral, red chevron hostile. Red corners mark your opponent. Fight stance closes to weapon reach and attacks at cooldown. The analog stick overrides pursuit. Followers engage nearby "
    "threats. \006 pauses combat for orders. Heavy steel handles best with strength equal to twice its weight. Weak or "
    "injured arms lengthen preparation and recovery. Mauls pierce armor; glaives reach farther; greatswords "
    "hit hard; anchor cleavers bleed. Guns use cartridges; "
    "bows use bolts.\nArmor protects specific body locations. Piercing weapons ignore some armor; serrated "
    "blades bleed; staggering hits interrupt. Skills improve through use. Retreat from stronger opponents "
    "and use terrain. Gold windup marks show incoming strikes. Enemy combat bars show remaining knockout reserve from head, chest and blood. Square to passive cancels your attack; move away to disengage.",
    "Head or chest injuries and blood loss can knock a survivor out. Severe injuries can cause a recovery coma. After stabilizing, a survivor wakes prone and can give themselves first aid. Critical injuries may leave them crawling. Severe head trauma or blood loss can kill.\n"
    "Dress an injured body part before it can heal. Clotting stops blood loss. It still takes a dressing to start healing. Severe wounds take longer to bandage and recover. Beds and clinic care speed up recovery. Health shows healthy tissue in teal, untreated damage in red and dressed damage in gold. Nearby Actions offers first aid and emergency dressings. First aid stops bleeding before treating each wound. Check the remaining charge on a field dressing. An ally can treat someone who is down or carry them to an unoccupied bed. A rare passerby may help or carry you to a safe bed. Lost limbs never regrow. A bad leg injury makes you limp; a missing leg or two disabled legs makes you crawl. One missing arm penalizes two-handed weapons. Health: R opens individual limb slots. Fit replacements for 2 machine parts. Surgeon Venn in Dusk Haven removes limbs for 400 silver. Copper Cross sells reinforced limbs; Iron Gate precision limbs; Black Mesa relic limbs. Prosthetics are uncommon on ordinary travelers; high-grade parts are rarer. Actual fitted limbs on fallen wanderers can be looted. Death is "
    "permanent; dismiss the body from Squad to free its slot.",
    "Food is eaten below forty hunger. You eat what you need and save the remainder. Supplies come from your pack, then a nearby companion or owned depot within "
    "three meters. Water supports "
    "farms; tonics restore blood and treat exposure. Starvation damages the chest.\nAn inn bed charges its listed lodging price. Camp shelters are free. Six roadside wayfarer beds have finite caches. Rock passes provide cover and outer bypasses. Sleeping reduces fatigue and helps dressed wounds heal. Severe hunger slows recovery. Move to wake. The sun completes a day in thirty minutes of "
    "simulation; menus and "
    "tactical pause stop time.",
    "Six slots: weapon, torso, head, boots, legs, back. Pack \001 equips; \003 passes one to an ally; \004 "
    "switches carried/worn gear. \001 on worn gear removes it if space allows. Hovering equipment changes the single character preview and compares stats. WORN DISGUISE shows your current outfit; PREVIEW shows the hovered outfit. Equipping commits it.\nPlate protects more but slows movement and hinders stealth. Cloth is light; "
    "travel gear favors weather or movement. Higher grades improve the same role. Torso armor covers "
    "chest/arms; headgear covers head; leg armor and boots have separate coverage chances. MOVE% is the worn speed "
    "bonus. Weight above capacity slows you further.",
    "Talk to a trader and choose Trade. \014 switches buying and selling; \005/\006 filters supplies, "
    "weapons, armor or backpacks. \001 trades one item. Shops have finite stocks and refresh daily with "
    "different "
    "gear.\nRegional exports are cheap; imports and shortages cost more. Green percentages mean a good buy or sale; red means poor, relative to ordinary buy/resale value. Ask merchants for trading tips. Hostile factions refuse trade unless fooled by a disguise. "
    "Stolen goods appear in red. The selling list shows the chance a trader accepts the item without reporting you. Stealth and Thievery improve the odds. Black Mesa fences most outside goods. The item "
    "details show its stats and effects before purchase.",
    "Enter a smithy and walk to the forge. Choose slot, design, "
    "material and quality with \014. The preview updates before you spend anything.\n\001 on Forge makes "
    "the selected design. Metal, cloth and timber costs scale with material and grade; relic gear also needs "
    "a relic. Higher grades require smithing experience and research. Missing materials or a full pack "
    "cancel the transaction without charging you.",
    "Start > Camp & Home > Research shows three forging tiers. Stand near a research bench with a relic and silver to research. Furniture needs no research.\nEnter a smithy to forge equipment. Mechanical limbs must be bought or found. Cooking remains available at campfires, including with enemies nearby.",
    "Start > Camp & Home > Build camp offers a campfire and an inventory bedroll. Move the placement cursor with the analog stick or D-pad, then Cross to place. Red spots are blocked. Placement clears foliage. Camps disappear after a day away.\nBuy a house at its door, enter it and choose Place furniture. Beds, chests, tables, shelves and stoves cost silver. Move / remove repositions existing furniture. All purchased-home chests share a free stash.\nTalk to a depot clerk or use the counter for a separate store shared across towns. Fees are quoted for deposits and withdrawals. \014 switches sides; \001 moves one item; \003 moves a stack.",
    "D-pad Right opens Squad management. \005/\006 chooses a member; \015 selects a row; \014 changes its value. Follow trails the "
    "assigned group leader; Hold stays put. Mine gathers local deposits; Farm waters and harvests; Medic treats "
    "injured allies.\nSmelt and Cook use nearby stations and can collect ingredients from a nearby depot. "
    "Workers fetch water and ingredients, walk to stations, and store output; cooks keep two rations. Haul deposits raw cargo. Job status explains delays. Select a worker and move "
    "to interrupt their current action. Up to eight living members may join.",
    "\002 sneaks. Detection depends on stealth skill, equipment, distance and weather. Sneak and \001 near a "
    "civilian to inspect a pickpocket attempt. Select an item with \015. Risk reflects weight, value, approach and visible witnesses. Getting behind the target improves odds. \001 steals one; \002 cancels. Owned salvage says STEAL. Locks take time to pick; tools are optional.\nShop and house chests are private. Repeated theft can bring pocket checks for two days. Witnesses can warn you or raise a bounty. A theft can fail, drawing guards. Stolen "
    "stacks retain an owner until fenced. Review the charges with a watch officer before confirming a payment. Jail staff treat injuries while you serve time. Failed lockpicks train your skill. Nearby Actions frees other prisoners or slaves. Work takes time and guards can catch you. Some freed people flee; others follow independently. Only Recruit adds them to your squad. Leave me alone dismisses a follower.",
    "Compact guards roads and wells; Rust Caravans trade salvage. Shogunate markets sell lamellar and "
    "glaives. Ash monks sell medicine; Black Mesa fences stolen goods. Vultures raid; wild packs hunt.\n"
    "Contracts raise standing; theft lowers it. The Factions ledger shows each faction’s outstanding amount and charges. Ash Monastery, Shogun Rest and Vulture Nest require matching body armor and headwear; close inspections can expose a disguise. "
    "Hired guards cost 240 silver for escort or 180 for camp duty: one simulated day. They fight and "
    "bandage independently, keep their gear, wait outside interiors and return home on expiry.",
    "Ask merchants or marshals for work. Freight needs a refundable deposit and four goods. Bring the cargo to the receiving contact. Caravans wait for you outside town. Square on the Journal entry locates the caravan. Escort or patrol work pays 20 silver per 30 seconds of nearby movement, plus a completion bonus. Stay within 8m. Check Health / Stats has Health, Stats and Bounty tabs. The Bounty tab lists payouts and accepting factions. Carry the target inside a paying faction's jail and choose Environment > Turn in carried bounty. Live targets pay double.\nDispatch jobs give a sealed packet. It is delivered when you reach the destination; return to your contact for payment. The Journal tracks the current objective and completed work.",
    "Six manual slots plus autosave. \007, Save or Load opens a ledger with name, day, party size and "
    "silver. Choose a slot; \001 asks for confirmation. Save-and-title opens the ledger; choose and confirm a slot.\nSaves "
    "use version, size and checksum validation and preserve a previous .bak file. Invalid files never "
    "replace your running expedition. Older saves through 0.11.0 remain loadable. Back them up before "
    "overwriting. "
    "Autosave is slot zero and is never used for manual saving.",
    "Dust, rain, acid mist and snow change visibility, terrain colors and survival pressure. Storms reduce "
    "detection and ranged accuracy. Acid and cold build exposure outdoors; shelters and sealed gear protect "
    "you.\nTonics and rest help recover. Rain irrigates farms. The northern highlands are colder; the fen "
    "produces acid mist. Weather changes as time passes. The world ledger reports storms and settlement "
    "events so you can plan supplies and routes.",
    "For a minor bounty the watch asks you to pay or serve time. At 100 silver or more they pursue and arrest you when you fall. Jail staff treat injuries and seize stolen goods. In the cell, \001 offers bail or lockpicking. The watch can relock an open cell if you linger. Move to the exit and press \001 to leave. Escaping adds a charge and the watch can pursue you.\nSlavers carry fallen survivors to a work camp. Shackled prisoners work in issued garb. The camp chest holds their seized equipment and supplies. Press \001 to pick your shackles. Former captors can recognize escapees.",
    "Approach a house, inn, shop, tower or workshop door and press \001 to enter. Walk around the room with "
    "the analog stick. Furniture blocks movement; use the aisles. \001 near the door returns outside.\n"
    "Counters open local trade and smithies sell weapons and armor. Inns offer beds for 25 silver each. Owned beds are free. Join the Dustward Clan for 280 silver to use beds and training dummies at any of its lodges. Dummies train your equipped weapon skill or Martial Arts up to level 20. Use a research bench directly from outside. Indoor time passes: hunger, bleeding and healing continue. Nearby followers in your group enter with you. Other "
    "groups and hired guards remain outside.\nSave games preserve the building and indoor position. Switching squad "
    "members leaves the room. Interiors shelter you from weather; occupants who distrust you may demand you leave and attack if you stay.",
    "Buy travel packs, medicine rolls, ore harnesses and bedroll packs from the BACKPACKS shop tab. "
    "Preview before buying. Other back gear rotates daily.\n"
    "CARGO: travel 80%, medicine 85%, ore 60%, bedroll 70% of carried weight. Worn gear and carried "
    "people keep full weight. CARRY adds load allowance; LOAD previews a swap. All packs use the same "
    "24 inventory slots.\n"
    "FIGHT reduces melee attack/defence: travel -2, medicine 0, ore -7, bedroll -4. Bulky packs hurt "
    "stealth. Medicine rolls speed up first aid. Bedroll packs help you recover while resting. Buy a Bedroll in Supplies to lay out a portable bed.",

    "Work is posted intermittently. Ask a merchant, marshal, innkeeper or smith, or ask locals what they have heard. If someone is hiring, the local contact will name the job. A town can have up to three postings. After accepting one, allow time for new work. The Journal lists the objective, payout and the person who pays.",
    "Squad management sets Balanced, Defensive, Keep range or Hold fire. Defensive gives +18 defence but never attacks. Ranged allies retreat if a foe closes within 3.5m. Projectiles travel and hit walls or allies: clear your firing line.\nAssign each survivor to one of three groups and choose its leader. A pin marks the destination on both maps. Follow its line on the minimap. Walk with the analog stick; group members follow their leader. Four job priorities choose the first available task. Put Follow or Hold last. Manual movement pauses a queue; resume the Priority queue in Squad management.",
    "Wilderness groups travel, camp, hunt and fight. Camp bedrolls are free when unoccupied, and some camps have tents. Ore and cuttable timber have distinct minimap dots outside settlements. Road traders carry their own stock.\nUse a campfire to cook food; nearby enemies do not block cooking. A bedroll from your pack opens the placement view. Use Nearby Actions to rest or pack it up. Player camps disappear after a day away.",
    "Cuts can worsen while bleeding. Bandaging starts gradual recovery from organic damage. Deep wounds take longer. Rest in a bed to recover faster.\nMechanical characters and prosthetics need timed repairs with spare parts. Medicine and sleep cannot repair metal. Health & recovery shows health, trauma and armor condition. Cloth restores equipment condition.\nFree allies can pick a captive's shackles. First aid, trade and completed work improve faction standing.",
    "Use Armor Layers to fit a light body garment under outer armor. Weight must be five or less. Swaps preserve stolen ownership and fail safely if your pack is full.\nOuter body covers 95% of chest hits. New light/medium/heavy sets cover 40/65/80% of arm hits; headwear covers 85%, legwear 90%, boots 35%. An uncovered hit bypasses that piece. Blunt impacts use 72% of outer protection. Underlayers add reduced protection. Wear lowers protection to a minimum of 65%; maintain with cloth. Inspect both armor and movement before equipping."};
