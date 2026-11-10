#ifndef DUSTBOUND_POPULATION_TYPES_H
#define DUSTBOUND_POPULATION_TYPES_H
#define OLD_POP_GROUPS 80
#define WAR_POP_GROUPS 83
#define ROAMING_BANDIT_GROUPS 8
#define POP_GROUPS (WAR_POP_GROUPS + ROAMING_BANDIT_GROUPS)
#define POP_MEMBERS 16
#define POP_TYPES 16
#define POP_PROPS 8
enum {
    POP_SCAVENGERS, POP_REFUGEES, POP_HUNTERS, POP_SMUGGLERS,
    POP_MERCHANTS, POP_PATROL, POP_RAIDERS, POP_BEASTS,
    POP_PILGRIMS, POP_WOUNDED, POP_BATTLEFIELD, POP_ARMY,
    POP_AMBUSH, POP_SALVAGE, POP_PRISONERS, POP_DUELISTS
};
enum { POP_REST, POP_TRAVEL, POP_FORAGE, POP_FIGHT, POP_RUINS };
typedef struct {
    int phase, beds, stock[3], cacheItem, cacheQty, detached;
    int seen, generation, fights, arrivals, imported, opponent, harvests;
    float clock, targetX, targetY, siteX, siteY, harvestClock;
    float offsetX[5], offsetY[5];
} PopulationState;
typedef struct {
    float weaponXP[WEAPON_XP_SLOTS], recognized[FACTIONS];
    int wanted[FACTIONS], cannibal, captorFaction;
} PopulationSocial;
typedef struct {
    int version, serial, conflicts, deaths, arrivals, imported;
    unsigned rng;
    float pulse, offlinePulse;
    WorldEvent groups[POP_GROUPS];
    PopulationState state[POP_GROUPS];
    PopulationSocial social[POP_GROUPS][POP_MEMBERS];
} Population;
#endif
