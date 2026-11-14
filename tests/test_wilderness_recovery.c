#include "game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
extern int hostile(const Actor*,const Actor*);
int main(void){
 for(int run=0;run<3;run++){
  game_new(81920+run*7,0);Actor*a=&g.actors[g.selected];a->x=a->y=400;a->autoattack=0;
  ui.screen=SCREEN_WORLD;g.day=42;g.time=41*DAY_SECONDS;g.eventTimer=100000;g.autosave=-100000;
  for(int n=1;n<g.actorCount;n++)g.actors[n].active=0;
  for(int k=0;k<EVENTS;k++)g.living.events[k].status=2;
  for(int k=0;k<POP_GROUPS;k++){
   WorldEvent*e=&g.population.groups[k];e->x=1200+k%10*15;e->y=1200+k/10*15;e->age=e->presence=600;
   g.population.state[k].siteX=e->x;g.population.state[k].siteY=e->y;g.population.state[k].phase=POP_RUINS;
   g.life.groups[k].goal=LIFE_SETTLED;
   for(int m=0;m<e->count;m++){e->reserve[m].dead=1;e->reserve[m].x=e->x;e->reserve[m].y=e->y;}
  }
  int seen[POP_GROUPS]={0},encounters=0,threats=0,camps=0;float gap=0,maxgap=0,heading=0;
  for(int step=0;step<3600;step++){
   for(int l=0;l<LIMBS;l++)a->health[l]=species_health(a);
   a->blood=a->hunger=100;a->dead=a->unconscious=a->stun=a->bleed=0;
   if(step%10==0){for(int turn=0;turn<32;turn++){float h=heading+(turn%2?1:-1)*(turn+1)/2*.1963495f;int clear=1;for(int d=1;d<6;d++)if(!can_walk(a->x+cosf(h)*d,a->y+sinf(h)*d))clear=0;if(clear){heading=h;break;}}}
   game_tick(.1f,(Input){.ax=view_horizontal(cosf(heading),sinf(heading)),.ay=view_vertical(cosf(heading),sinf(heading))});
   int visible=0;
   for(int k=0;k<POP_GROUPS;k++){
    WorldEvent*e=&g.population.groups[k];if(!e->spawned)continue;
    for(int m=0;m<e->count;m++){int n=e->members[m];if(n<0)continue;Actor*b=&g.actors[n];float x=b->x-a->x,y=b->y-a->y;
     if(!b->dead&&!depth_room(b)&&fabsf(view_horizontal(x,y))<11&&fabsf(view_vertical(x,y))<13){visible=1;
      if(seen[k]!=e->serial){seen[k]=e->serial;encounters++;camps+=g.population.state[k].beds>0;
       if(hostile(a,b)){threats++;/* Simulate a cleared encounter, not a combat-balance test. */for(int j=0;j<e->count;j++)if(e->members[j]>=0){g.actors[e->members[j]].dead=1;g.actors[e->members[j]].health[CHEST]=-100;}}
      }break;
     }
    }
   }
   gap+=.1f;if(visible){if(gap>maxgap)maxgap=gap;gap=0;}
  }
  if(gap>maxgap)maxgap=gap;
  printf("DEPLETED %d: %d visible groups, %d cleared threats, %d camps; longest empty stretch %.1fs; director %d\n",run,encounters,threats,camps,maxgap,g.wilderness.sequence);fflush(stdout);
  assert(encounters>=5&&threats>=3&&camps>=1&&maxgap<100);
  assert(population_validate(&g));assert(save_game("build/wilderness-depleted.sav")&&load_game("build/wilderness-depleted.sav"));
 }
 puts("PASS: three depleted day-42 frontiers repopulate during six-minute analog walks, including hostile replenishment after simulated clears and real camps.");
 return 0;
}
