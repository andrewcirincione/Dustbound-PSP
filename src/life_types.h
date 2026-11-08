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
#endif
