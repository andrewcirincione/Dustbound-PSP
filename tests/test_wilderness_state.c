#include "game.h"
#include "combat_pose.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 game_new(73819,0);Actor*a=&g.actors[0];a->x=a->y=400;
 population_generate(0,POP_RAIDERS);assert(population_materialize(0));WorldEvent*e=&g.population.groups[0];int n=e->members[0];Actor*b=&g.actors[n];
 b->health[LARM]=39;b->target=1;b->attackPhase=1;b->attackTarget=1;
 int weapon=b->gear[0];assert(population_store(0));assert(!e->spawned&&e->reserve[0].health[LARM]==39&&e->reserve[0].gear[0]==weapon);
 assert(population_materialize(0));assert(g.actors[e->members[0]].health[LARM]==39);assert(population_store(0));
 int site=g.wilderness.building[0]-1;
 actor_enter_room(0,site);for(int t=0;t<3;t++)expedition_tick(1);
 ExpeditionCrew*crew=&g.expedition.crew[0][0];assert(crew->initialized&&crew->count>0);
 for(int m=0;m<crew->count;m++){n=crew->live[m];assert(n>=0&&depth_room(&g.actors[n])==site+1);}
 assert(save_game("build/wilderness-guard.sav")&&load_game("build/wilderness-guard.sav"));
 assert(g.expedition.crew[0][0].initialized&&g.expedition.crew[0][0].live[0]>=0);
 game_new(73819,0);a=&g.actors[0];a->x=a->y=400;a->gear[0]=WEAPON_ITEMS+109;a->autoattack=0;a->target=-1;ui.screen=SCREEN_WORLD;
 game_tick(.1f,(Input){.pressed=KEY_SQUARE});for(int t=0;t<3;t++)game_tick(.1f,(Input){0});assert(a->autoattack&&a->target<0&&combat_stance(a)>.99f);
 game_tick(.1f,(Input){.pressed=KEY_SQUARE});for(int t=0;t<4;t++)game_tick(.1f,(Input){0});assert(!a->autoattack&&combat_stance(a)<.01f);
 puts("PASS: Square visibly raises a greatsword into guard without a target, then returns it to the passive shoulder rest.");
 puts("PASS: distant combat releases live slots without losing wounds/equipment; ruin guards inhabit actual interiors and survive save/load.");
 return 0;
}
