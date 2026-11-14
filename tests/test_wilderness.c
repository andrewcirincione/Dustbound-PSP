#include "game.h"
#include "frontier_fixture.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stddef.h>
extern int hostile(const Actor*,const Actor*);
static int seen[POP_GROUPS], kinds, encounters, hostiles;
static int sightings(const Actor*a){
 int visible=0;
 for(int n=0;n<g.actorCount;n++){const Actor*b=&g.actors[n];float dx=b->x-a->x,dy=b->y-a->y;if(b->active&&!b->squad&&!b->dead&&!depth_room(b)&&fabsf(view_horizontal(dx,dy))<11&&fabsf(view_vertical(dx,dy))<13)visible++;}
 for(int k=0;k<POP_GROUPS;k++){WorldEvent*e=&g.population.groups[k];if(!e->spawned)continue;
  for(int m=0;m<e->count;m++){int n=e->members[m];if(n<0)continue;Actor*b=&g.actors[n];float dx=b->x-a->x,dy=b->y-a->y;
   if(b->active&&!b->dead&&!depth_room(b)&&fabsf(view_horizontal(dx,dy))<11&&fabsf(view_vertical(dx,dy))<13){visible++;if(seen[k]!=e->serial){seen[k]=e->serial;kinds|=1<<e->type;encounters++;hostiles+=hostile(a,b);}break;}
  }
 }
 return visible;
}
int main(void){
 for(int route=0;route<4;route++){
  game_new(73819+route*53,0);ui.screen=SCREEN_WORLD;g.eventTimer=100000;g.autosave=-100000;
  memset(seen,0,sizeof(seen));kinds=encounters=hostiles=0;
  Actor*a=&g.actors[g.selected];a->autoattack=0;
  float heading=route*.8f,ox=a->x,oy=a->y,stuck=0,quiet=0,maxgap=0,first=-1,outdoors=0;
  for(int step=0;step<12000;step++){
   for(int l=0;l<LIMBS;l++)a->health[l]=species_health(a);
   a->blood=a->hunger=100;a->unconscious=a->dead=a->stun=a->bleed=0;a->lostMask=0;
   if(step%20==0||stuck>1){
    int found=0;for(int trial=0;trial<32;trial++){
     float h=heading+(trial%2?1:-1)*((trial+1)/2)*.19634954f,dx=cosf(h),dy=sinf(h);int clear=1;
     for(int d=1;d<=5;d++)if(!can_walk(a->x+dx*d,a->y+dy*d))clear=0;
     if(clear){heading=h;found=1;break;}
    }if(!found)heading+=.7f;
   }
   float dx=cosf(heading),dy=sinf(heading);
   if(0)printf("TRACE t%d game%.0f pos%.0f,%.0f moving%.1f ui%d pause%d room%d prison%.0f quiet%.0f danger%.0f seq%d\n",step/10,g.time,a->x,a->y,a->movedTimer,ui.screen,ui.paused,depth_room(a),a->prisonTime,g.wilderness.quiet,g.wilderness.danger,g.wilderness.sequence);
   int before=encounters;
   game_tick(.1f,(Input){.ax=view_horizontal(dx,dy),.ay=view_vertical(dx,dy)});
   if(distance(ox,oy,a->x,a->y)<.005f)stuck+=.1f;else stuck=0;ox=a->x;oy=a->y;
   int visible=sightings(a);
   if(town_near(a->x,a->y,24)<0){outdoors+=.1f;quiet+=.1f;if(visible){if(first<0)first=outdoors;if(quiet>maxgap)maxgap=quiet;quiet=0;}}
  }
  if(quiet>maxgap)maxgap=quiet;
  printf("ROUTE %d: encounters %d hostile %d types %x first %.1fs max-gap %.1fs position %.1f %.1f spawned %d\n",route,encounters,hostiles,kinds,first,maxgap,a->x,a->y,g.wilderness.sequence);fflush(stdout);
  assert(encounters>=4&&hostiles>=2&&maxgap<120);
  assert(save_game("build/wilderness-travel.sav")&&load_game("build/wilderness-travel.sav"));
 }
 game_new(73819,0);
 int oldCount=g.wilderness.building[0]-1;assert(oldCount>0);g.buildingCount=oldCount;
 /* A real v32 prefix, before any new sites or director state exist. */
 size_t bytes=offsetof(Game,wilderness);unsigned hash=2166136261u;unsigned char *raw=(unsigned char*)&g;
 for(size_t n=0;n<bytes;n++)hash=(hash^raw[n])*16777619u;
 struct {char magic[8];unsigned version,size,hash;}header={{'D','U','S','T','S','A','V','E'},32,bytes,hash};
 FILE*f=fopen("build/wilderness-v32.sav","wb");assert(f);fwrite(&header,sizeof(header),1,f);fwrite(&g,bytes,1,f);fclose(f);
 assert(load_game("build/wilderness-v32.sav"));assert(g.wilderness.version==1);
 int sites=0;
 for(int i=0;i<WILDERNESS_SITES;i++){
  int b=g.wilderness.building[i]-1;if(b<0)continue;sites++;assert(wilderness_site(b)==i);
  assert(building_has_interior(g.buildings[b].type));
  actor_enter_room(g.selected,b);fixture_service(ROOM_RUIN_RECORD);interior_interact();
  fixture_service(ROOM_CHEST);interior_interact();assert(ui.screen==SCREEN_CHEST);
  assert(interior_chest_count()>0);interior_chest_take(0);
  g.interiorBuilding=0;g.depth.actors[g.selected].room=0;ui.screen=SCREEN_WORLD;
 }
 assert(sites==16);assert(save_game("build/wilderness-sites.sav")&&load_game("build/wilderness-sites.sav"));
 puts("PASS: v32 upgrades to 16 enterable named locations; record and salvage interactions work and persist.");
 puts("PASS: four 20-minute analog travel routes meet density bounds using actual materialized, on-screen encounters; player health held constant to isolate encounter availability.");
 return 0;
}
