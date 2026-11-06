#ifndef DUSTBOUND_DEPTH_TYPES_H
#define DUSTBOUND_DEPTH_TYPES_H
#define LANDMARKS 8
#define PROJECTILES 24
#define DEPTH_NPCS 8
enum { ROUTE_BLOCKED, ROUTE_BYPASS, ROUTE_TRUCE, ROUTE_CAPTURED, ROUTE_FREED };
enum {
    SITE_WRECK,
    SITE_CULVERT,
    SITE_CAMP,
    SITE_CAGES,
    SITE_SHRINE,
    SITE_FOUNDRY,
    SITE_BEACON,
    SITE_STORES
};
enum { NPC_SORA, NPC_NELL, NPC_RELL, NPC_ORIN, NPC_TAMSIN, NPC_PATROL_A, NPC_PATROL_B, NPC_CONVOY };
enum { ORDER_BALANCED, ORDER_BLOCK, ORDER_RANGED, ORDER_HOLD_FIRE };
typedef struct {
    int order, group, queue[4], queued, paused, layer, layerOwner;
    int captive, confiscated, confiscatedOwner, room;
    float rx, ry, escape, recognized, trauma[LIMBS], upkeep, trespass;
} DepthActor;
typedef struct {
    float labor, fuel, condition;
} DepthBuilding;
typedef struct {
    int active, source, weapon, room;
    float x, y, vx, vy, left;
} Projectile;
typedef struct {
    int version, route, rescued, reported, supplied, freed, visited, salvage, lastDay;
    int npc[DEPTH_NPCS], siteBuilding[LANDMARKS], retal, convoyTrips;
    float siteX[LANDMARKS], siteY[LANDMARKS], retaliation, convoyClock;
    int groupLeader[3], groupTravel[3];
    float groupX[3], groupY[3];
    DepthActor actors[MAX_ACTORS];
    DepthBuilding buildings[MAX_BUILDINGS];
    Projectile shots[PROJECTILES];
} FrontierDepth;
#endif
