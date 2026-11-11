#include "game.h"
#include "save_fixture.h"
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
static void merged_proficiencies(void) {
    assert(WEAPON_CLASSES == 6);
    const int classes[] = {
        WC_SLASHING, WC_SLASHING, WC_SLASHING, WC_LIGHT, WC_POLEARMS, WC_POLEARMS, WC_POLEARMS, WC_SLASHING,
        WC_SLASHING, WC_LIGHT, WC_SLASHING, WC_HEAVY, WC_HEAVY, WC_HEAVY, WC_HEAVY, WC_HEAVY,
        WC_POLEARMS, WC_SLASHING, WC_SLASHING, WC_SLASHING, WC_HEAVY, WC_SLASHING, WC_SLASHING, WC_HEAVY,
        WC_ARCHERY, WC_ARCHERY, WC_ARCHERY, WC_ARCHERY, WC_ARCHERY, WC_FIREARMS, WC_FIREARMS, WC_FIREARMS,
        WC_HEAVY, WC_HEAVY, WC_POLEARMS, WC_HEAVY
    };
    for (int i = 0; i < WEAPON_FAMILIES; i++)
        assert(weapon_class(gear_id(weapon_family_index(i), 1, 1)) == classes[i]);
    assert(weapon_class(SCIMITAR_ITEMS) == WC_SLASHING && weapon_class(RAPIER_ITEMS) == WC_LIGHT);
    assert(weapon_style(gear_id(0, 1, 1)) != weapon_style(SABRE));
    assert(weapon_cut_ratio(gear_id(84, 1, 1)) == .45f && weapon_cut_ratio(gear_id(14, 1, 1)) == .05f);
    assert(weapon_class(NONE) == -1);

    pair(gear_id(0, 1, 1), SABRE);
    /* v15 and v16 share the payload layout. Version selects the XP order. */
    const float oldXP[WEAPON_XP_SLOTS] = {2700, 7500, 100000, 15000, 3100, 10800, 1800, 4800, 7500, 10800, 30000};
    const float expected[WEAPON_XP_SLOTS] = {10200, 6600, 118100, 10800, 18300, 30000};
    const float oldStoredXP[WEAPON_XP_SLOTS] = {144, 300, 0, 0, 0, 0, 0, 0, 44, 66, 1200};
    const float expectedStored[WEAPON_XP_SLOTS] = {444, 0, 0, 0, 110, 1200};
    memcpy(g.society.weaponXP[0], oldXP, sizeof(oldXP));
    memcpy(g.population.social[0][0].weaponXP, oldStoredXP, sizeof(oldStoredXP));
    const char *legacy = "build/progression-v15.sav";
    save_prior_fixture(legacy, 15);

    assert(load_game(legacy));
    assert(!memcmp(g.society.weaponXP[0], expected, sizeof(expected)));
    assert(!memcmp(g.population.social[0][0].weaponXP, expectedStored, sizeof(expectedStored)));
    assert(weapon_class_level(&g.actors[0], weapon_class(SABRE)) == 30);
    assert(weapon_class_level(&g.actors[0], WC_HEAVY) == 100);
    assert(save_game("build/progression-v17.sav") && load_game("build/progression-v17.sav"));
    assert(!memcmp(g.society.weaponXP[0], expected, sizeof(expected)));
    assert(!memcmp(g.population.social[0][0].weaponXP, expectedStored, sizeof(expectedStored)));
    puts("PASS: all six class mappings, shared XP, capped v15 migration of live and stored actors, and v17 reload.");
}
int main(void){
 setvbuf(stdout,NULL,_IONBF,0);int w=gear_id(0,1,1);unsigned hit=outcome(w,SABRE,0,0),block=outcome(w,SABRE,1,0);
 pair(w,SABRE);g.rng=hit;combat_impact(0,1,w);float classHit=g.society.weaponXP[0][WC_SLASHING],defHit=g.actors[1].xp[DEFENCE];assert(g.actors[0].xp[MELEE]>0&&g.actors[0].xp[STRENGTH]>0&&g.actors[0].xp[DEXTERITY]>0&&classHit>0);assert(g.actors[1].xp[TOUGHNESS]>0&&defHit>0&&g.actors[1].xp[STRENGTH]>0);assert(!g.actors[1].xp[DODGE]);
 pair(w,SABRE);g.rng=block;combat_impact(0,1,w);assert(g.society.weaponXP[0][WC_SLASHING]<classHit&&g.actors[1].xp[DEFENCE]<defHit&&g.actors[1].xp[STRENGTH]>0);assert(g.actors[1].xp[TOUGHNESS]==0);
 unsigned dodge=outcome(w,NONE,1,0),hitUnarmed=outcome(w,NONE,0,0);pair(w,NONE);g.rng=dodge;combat_impact(0,1,w);float dx=g.actors[1].xp[DODGE];assert(dx>0&&g.actors[1].impactKind==3);pair(w,NONE);g.rng=hitUnarmed;combat_impact(0,1,w);assert(g.actors[1].xp[DODGE]>=dx*1.99f&&g.actors[1].xp[DEFENCE]==0);
 unsigned stagger=outcome(w,SABRE,1,1);pair(w,SABRE);g.actors[1].stun=.2f;g.rng=stagger;combat_impact(0,1,w);assert(g.actors[1].xp[DODGE]>0&&g.actors[1].xp[DEFENCE]==0);
 pair(w,NONE);float light=dodge_mobility(&g.actors[1]);g.actors[1].pack[0]=(Stack){ORE,999};assert(dodge_mobility(&g.actors[1])<light);
 pair(NONE,SABRE);combat_impact(0,1,NONE);assert(g.actors[0].xp[MARTIAL]>0&&g.actors[0].xp[STRENGTH]>0&&g.actors[0].xp[DEXTERITY]>0);
 assert(fabsf(combat_learning(50,100)-6)<.001&&fabsf(combat_learning(100,50)-.1f)<.001&&combat_learning(10,30)>combat_learning(30,10));
 pair(w,SABRE);g.actors[1].xp[DEFENCE]=12*49*49;g.rng=hit;combat_impact(0,1,w);float strongXP=g.actors[0].xp[MELEE];pair(w,SABRE);g.rng=hit;combat_impact(0,1,w);assert(strongXP>g.actors[0].xp[MELEE]);
 pair(w,SABRE);g.rng=hit;combat_impact(0,1,w);float unarmoredXP=g.actors[1].xp[TOUGHNESS];pair(w,SABRE);g.actors[1].gear[1]=PLATE;g.rng=hit;combat_impact(0,1,w);assert(fabsf(g.actors[1].xp[TOUGHNESS]-unarmoredXP)<.01f);
 pair(w,SABRE);float before=weapon_handling(&g.actors[0],w);g.society.weaponXP[0][WC_SLASHING]=12*49*49;assert(weapon_class_level(&g.actors[0],WC_SLASHING)==50&&weapon_handling(&g.actors[0],w)<before);assert(weapon_class_level(&g.actors[0],WC_LIGHT)==1);assert(save_game("build/progression-v9.sav"));assert(load_game("build/progression-v9.sav"));assert(weapon_class_level(&g.actors[0],WC_SLASHING)==50);g.society.weaponXP[0][0]=NAN;assert(!society_validate(&g));
 merged_proficiencies();
 pair(w,RUSTSWORD);g.squadCount=1;g.actors[1].squad=0;g.actors[1].kind=RAIDER;g.actors[1].faction=HOSTILE;g.actors[0].autoattack=1;g.actors[1].autoattack=1;for(int n=2;n<g.actorCount;n++)g.actors[n].active=0;assert(save_game("build/v090/DUST_6.sav"));
 puts("PASS: combat strength/dexterity/melee/class XP; hit toughness and defense; reduced blocked XP; unarmed and stagger dodge; load penalty; harder-opponent multipliers; armor-safe toughness training; persistent independent class progress and handling benefit.");
}
