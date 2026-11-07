#include "game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static void pair(int w,int defensive){
 game_new(73819,0);g.selected=0;g.squadCount=2;
 for(int n=0;n<2;n++){Actor*a=&g.actors[n];a->squad=1;a->active=1;a->dead=0;a->unconscious=a->stun=0;a->carriedBy=-1;a->x=400+n;a->y=400;a->lostMask=a->prostheticMask=0;a->blood=a->hunger=100;memset(a->xp,0,sizeof(a->xp));memset(a->gear,0,sizeof(a->gear));memset(a->pack,0,sizeof(a->pack));for(int j=0;j<LIMBS;j++)a->health[j]=100;memset(&g.depth.actors[n],0,sizeof(DepthActor));}
 g.actors[0].gear[0]=w;g.actors[1].gear[0]=defensive;g.rng=1;
}
static unsigned outcome(int w,int defensive,int avoid,int stagger){for(unsigned seed=1;seed<500;seed++){pair(w,defensive);g.rng=seed;g.actors[1].stun=stagger?.2f:0;combat_impact(0,1,w);if((g.actors[1].impactKind!=1)==avoid)return seed;}assert(0);return 0;}
int main(void){
 setvbuf(stdout,NULL,_IONBF,0);int w=gear_id(0,1,1);unsigned hit=outcome(w,SABRE,0,0),block=outcome(w,SABRE,1,0);
 pair(w,SABRE);g.rng=hit;combat_impact(0,1,w);float classHit=g.society.weaponXP[0][WC_KATANAS],defHit=g.actors[1].xp[DEFENCE];assert(g.actors[0].xp[MELEE]>0&&g.actors[0].xp[STRENGTH]>0&&g.actors[0].xp[DEXTERITY]>0&&classHit>0);assert(g.actors[1].xp[TOUGHNESS]>0&&defHit>0&&g.actors[1].xp[STRENGTH]>0);assert(!g.actors[1].xp[DODGE]);
 pair(w,SABRE);g.rng=block;combat_impact(0,1,w);assert(g.society.weaponXP[0][WC_KATANAS]<classHit&&g.actors[1].xp[DEFENCE]<defHit&&g.actors[1].xp[STRENGTH]>0);assert(g.actors[1].xp[TOUGHNESS]==0);
 unsigned dodge=outcome(w,NONE,1,0),hitUnarmed=outcome(w,NONE,0,0);pair(w,NONE);g.rng=dodge;combat_impact(0,1,w);float dx=g.actors[1].xp[DODGE];assert(dx>0&&g.actors[1].impactKind==3);pair(w,NONE);g.rng=hitUnarmed;combat_impact(0,1,w);assert(g.actors[1].xp[DODGE]>=dx*1.99f&&g.actors[1].xp[DEFENCE]==0);
 unsigned stagger=outcome(w,SABRE,1,1);pair(w,SABRE);g.actors[1].stun=.2f;g.rng=stagger;combat_impact(0,1,w);assert(g.actors[1].xp[DODGE]>0&&g.actors[1].xp[DEFENCE]==0);
 pair(w,NONE);float light=dodge_mobility(&g.actors[1]);g.actors[1].pack[0]=(Stack){ORE,999};assert(dodge_mobility(&g.actors[1])<light);
 pair(NONE,SABRE);combat_impact(0,1,NONE);assert(g.actors[0].xp[MARTIAL]>0&&g.actors[0].xp[STRENGTH]>0&&g.actors[0].xp[DEXTERITY]>0);
 assert(fabsf(combat_learning(50,100)-6)<.001&&fabsf(combat_learning(100,50)-.1f)<.001&&combat_learning(10,30)>combat_learning(30,10));
 pair(w,SABRE);g.actors[1].xp[DEFENCE]=12*49*49;g.rng=hit;combat_impact(0,1,w);float strongXP=g.actors[0].xp[MELEE];pair(w,SABRE);g.rng=hit;combat_impact(0,1,w);assert(strongXP>g.actors[0].xp[MELEE]);
 pair(w,SABRE);g.rng=hit;combat_impact(0,1,w);float unarmoredXP=g.actors[1].xp[TOUGHNESS];pair(w,SABRE);g.actors[1].gear[1]=PLATE;g.rng=hit;combat_impact(0,1,w);assert(fabsf(g.actors[1].xp[TOUGHNESS]-unarmoredXP)<.01f);
 pair(w,SABRE);float before=weapon_handling(&g.actors[0],w);g.society.weaponXP[0][WC_KATANAS]=12*49*49;assert(weapon_class_level(&g.actors[0],WC_KATANAS)==50&&weapon_handling(&g.actors[0],w)<before);assert(weapon_class_level(&g.actors[0],WC_RAPIERS)==1);assert(save_game("build/progression-v9.sav"));assert(load_game("build/progression-v9.sav"));assert(weapon_class_level(&g.actors[0],WC_KATANAS)==50);g.society.weaponXP[0][0]=NAN;assert(!society_validate(&g));
 pair(w,RUSTSWORD);g.squadCount=1;g.actors[1].squad=0;g.actors[1].kind=RAIDER;g.actors[1].faction=HOSTILE;g.actors[0].autoattack=1;g.actors[1].autoattack=1;for(int n=2;n<g.actorCount;n++)g.actors[n].active=0;assert(save_game("build/v090/DUST_6.sav"));
 puts("PASS: combat strength/dexterity/melee/class XP; hit toughness and defense; reduced blocked XP; unarmed and stagger dodge; load penalty; harder-opponent multipliers; armor-safe toughness training; persistent independent class progress and handling benefit.");
}
