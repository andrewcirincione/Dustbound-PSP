typedef struct {
    uint32_t seed, rng;
    float time, dayTime, raidTimer, autosave;
    int day, money, selected, squadCount, tech, rep[FACTIONS], kills, discovered[OLD_TOWNS], stock[OLD_TOWNS][2052],
        lastRestock;
    int actorCount, buildingCount, nodeCount, baseExists;
    float baseX, baseY;
    uint8_t seen[FOG_SIZE * FOG_SIZE / 8];
    Actor actors[MAX_ACTORS];
    Building buildings[320];
    Node nodes[MAX_NODES];
    Contract contracts[CONTRACTS];
    int townOwner[OLD_TOWNS], prosperity[OLD_TOWNS], townState[OLD_TOWNS], weather, season, contractsDone;
    int factionWar[FACTIONS], storyStage, playOrigin, unlocks, difficulty;
    float eventTimer, weatherTimer, worldPulse, hitstop, shake;
    char worldNews[96];
    /* Version 3 extension. Version 2 ends immediately before these fields. */
    int interiorBuilding, interiorActor;
    float interiorX, interiorY;
} PreV5Game;
