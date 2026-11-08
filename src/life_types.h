#ifndef DUSTBOUND_LIFE_TYPES_H
#define DUSTBOUND_LIFE_TYPES_H
/* Append-only v13 state: never alter the released v12 Population prefix. */
enum { LIFE_ROAM, LIFE_HUNT, LIFE_AMBUSH, LIFE_RESCUE, LIFE_RETREAT, LIFE_RESUPPLY,
 LIFE_RETURN, LIFE_REFUGE, LIFE_SETTLED, LIFE_REINFORCE, LIFE_DELIVER, LIFE_RAID, LIFE_RECOVER, LIFE_GOALS };
enum { CAMP_READY, CAMP_PACKING, CAMP_AWAY, CAMP_ABANDONED };
typedef struct {
 int serial,goal,targetGroup,targetSerial,targetTown,sourceTown,camp,layout;
 int baseline,losses,ambushSprung,settled,cargo[3],carry[POP_MEMBERS];
 int supplies,hunts,rescues,relocations,arrivals,missionDone,seen;
 float decision,clock,emptyClock,travelClock,stalled,lastX,lastY;
} GroupLife;
typedef struct {
 int stage,elite,original,rival,support[2],pledge,relief,winner,leader,policy;
 int reinforcements[2],refugees,shipments;
 float age,pulse;
} TownPolitics;
typedef struct {
 int version,rescues,hunts,resupplies,relocations,settled,missions,succession;
 float pulse;
 GroupLife groups[POP_GROUPS];
 TownPolitics towns[TOWNS];
} FrontierLife;
/* v14: observations are snapshots, never remote tracking of living groups. */
typedef struct { int serial,goal,town,faction,type; float x,y,time; char name[32]; } FieldSighting;
typedef struct { FieldSighting group[POP_GROUPS]; } FieldNotes;

enum { WAR_FACTIONS = 3, WAR_RESIDENTS = 5, WAR_REPORTS = 16 };
enum { WAR_TAKEOVER = 1, WAR_CLASH, WAR_SUCCESSION };
typedef struct {
 int kind, town, winner, other, prisoners;
 float x, y, time;
} WarReport;
typedef struct {
 int version, reportNext, lastSpoken;
 int resident[WAR_FACTIONS][WAR_RESIDENTS];
 int prisonTown[MAX_ACTORS]; /* Town index + 1. Zero means unchained. */
 float nextRaid[WAR_FACTIONS], heldSince[TOWNS], yardClock;
 float nextGossip;
 WarReport report[WAR_REPORTS];
} FrontierWar;
#endif
