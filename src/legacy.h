/* Exact 0.1 save layout. Do not edit. */
typedef struct {
    char name[16];
    float x, y, homeX, homeY, tx, ty, health[6], blood, hunger, fatigue, bleed, cooldown, action, anim,
        unconscious;
    float xp[20];
    Stack pack[20];
    int gear[4];
    int active, kind, faction, squad, dead, job, target, taskKind, taskIndex, sneaking, autoattack, carrying,
        carriedBy, lostMask, prostheticMask, bounty, arrested, lootTaken;
} LegacyActor;
typedef struct {
    float x, y;
    int type, faction, owner, level, stock;
    float progress;
} LegacyBuilding;
typedef struct {
    float x, y;
    int type, remaining, locked, owner, item, qty;
    float regen;
} LegacyNode;
typedef struct {
    const char *name;
    float x, y;
    int faction;
} LegacyTown;
typedef struct {
    uint32_t seed, rng;
    float time, dayTime, raidTimer, autosave;
    int day, money, selected, squadCount, tech, rep[4], kills, discovered[5], stock[5][30], lastRestock;
    int actorCount, buildingCount, nodeCount, baseExists;
    float baseX, baseY;
    uint8_t seen[128 * 128 / 8];
    LegacyActor actors[80];
    LegacyBuilding buildings[100];
    LegacyNode nodes[200];
} LegacyGame;
