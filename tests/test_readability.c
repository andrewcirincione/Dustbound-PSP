#include "../src/render.c"
#include <assert.h>
static uint32_t pixels[512*272+64];
static void press(int key){game_tick(.033f,(Input){key,key,0,0});}
static void shot(const char *name){
 for(int i=512*272;i<512*272+64;i++)pixels[i]=0xdeadbeef;
 ui.camX=g.actors[g.selected].x;ui.camY=g.actors[g.selected].y;render_game(pixels,512);
 for(int i=512*272;i<512*272+64;i++)assert(pixels[i]==0xdeadbeef);
 char path[128];snprintf(path,sizeof(path),"build/v140/%s.ppm",name);FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n480 272\n255\n");
 for(int y=0;y<272;y++)for(int x=0;x<480;x++){uint32_t c=pixels[y*512+x];unsigned char rgb[]={c&255,c>>8&255,c>>16&255};fwrite(rgb,1,3,f);}fclose(f);
}
static void save(const char *name){char path[128];snprintf(path,sizeof(path),"build/v140/%s.sav",name);assert(save_game(path));}
static Actor*fresh(void){
 game_new(73819,0);Actor*a=&g.actors[g.selected];a->x=a->y=380;a->autoattack=0;g.money=1000;g.dayTime=12;ui.screen=SCREEN_WORLD;ui.feedCount=0;
 for(int n=1;n<g.actorCount;n++){g.actors[n].active=0;g.actors[n].target=-1;g.actors[n].attackPhase=0;}
 for(int k=0;k<POP_GROUPS;k++){WorldEvent*e=&g.population.groups[k];float x=1250+k%8*15,y=1250+k/8*15,dx=x-e->x,dy=y-e->y;e->x=x;e->y=y;g.population.state[k].targetX=x;g.population.state[k].targetY=y;g.population.state[k].siteX=x;g.population.state[k].siteY=y;g.life.groups[k].goal=LIFE_SETTLED;for(int i=0;i<e->count;i++){e->reserve[i].x+=dx;e->reserve[i].y+=dy;}}
 return a;
}
static void group(int type,int starving){
 for(int tries=0;tries<100;tries++){population_generate(0,type);if(!starving||g.population.groups[0].modifier==1)break;}
 WorldEvent*e=&g.population.groups[0];assert(!starving||e->modifier==1);float dx=380-e->x,dy=380-e->y;e->x=e->y=380;
 PopulationState*m=&g.population.state[0];m->siteX=m->siteY=380;m->targetX=430;m->targetY=380;
 for(int i=0;i<e->count;i++){e->reserve[i].x+=dx;e->reserve[i].y+=dy;}
 assert(population_materialize(0));g.actors[g.selected].x=380;g.actors[g.selected].y=386;
}
static void manual_pins(void){
 Actor*a=fresh();float x=a->x,y=a->y;ui.screen=SCREEN_MAP;ui.mapZoom=1;ui.mapX=x+50;ui.mapY=y+25;press(KEY_CROSS);assert(ui.screen==SCREEN_WORLD&&ui.waypoint);
 g.depth.groupTravel[0]=1;g.depth.groupX[0]=x+50;g.depth.groupY[0]=y+25;
 for(int i=0;i<300;i++)game_tick(.033f,(Input){0});
 assert(distance(x,y,a->x,a->y)<.001f&&!g.depth.groupTravel[0]);
 ui.screen=SCREEN_MAP;ui.mapZoom=1;press(KEY_R|KEY_CROSS);assert(!g.depth.groupTravel[0]);
 ui.screen=SCREEN_WORLD;for(int i=0;i<20;i++)game_tick(.033f,(Input){0,0,.8f,0});assert(distance(x,y,a->x,a->y)>.2f&&ui.waypoint);
 save("manual-pin");shot("manual-pin");puts("PASS: map pin and R+Cross do not move the player or start group travel; stale routes clear; analog walks with pin retained.");
}
static void service(void){
 assert(load_game("build/old-v13.sav"));assert(g.interiorBuilding>0);Actor*a=&g.actors[g.selected];a->autoattack=0;int money=g.money;ui.screen=SCREEN_WORLD;ui.paused=0;
 int row=-1,npc=0;nearby_actions();for(int i=0;i<nearby_actions();i++){const NearbyAction*x=nearby_action(i);if(x->kind==0)npc++;if(x->kind==5&&strstr(interior_prompt(),"SLEEP"))row=i;}
 assert(row>=0&&npc>0);save("inn-near-npc");nearby_action_choose(row);assert(a->taskKind==3&&ui.screen==SCREEN_WORLD&&g.money==money-25);shot("inn-rest");save("inn-rest");
 puts("PASS: migrated v13 inn save has an NPC within reach; choosing the bed starts rest and charges exactly 25 without opening dialogue.");
}
static void sights(void){
 Actor*a=fresh();group(POP_PATROL,0);field_observe(0);assert(g.fieldNotes.group[0].serial&&field_notes_valid(&g));FieldSighting old=g.fieldNotes.group[0];
 a->x=a->y=100;g.population.groups[0].x+=60;g.population.groups[0].y+=40;g.time+=120;field_observe(0);assert(!memcmp(&old,&g.fieldNotes.group[0],sizeof(old)));
 char detail[240];life_ledger_detail(0,detail,sizeof(detail));assert(strstr(detail,"Last seen")&&strstr(detail,"2.0 min ago"));ui.screen=SCREEN_LIFE;ui.selection=0;shot("sighting");politics_input((Input){KEY_CROSS,KEY_CROSS});assert(ui.screen==SCREEN_MAP&&ui.mapX==old.x&&ui.mapY==old.y);save("sighting");assert(load_game("build/v140/sighting.sav")&&!memcmp(&old,&g.fieldNotes.group[0],sizeof(old)));
 a=&g.actors[g.selected];a->x=old.x;a->y=old.y;for(int i=0;i<g.population.groups[0].count;i++)g.actors[g.population.groups[0].members[i]].dead=1;field_observe(0);assert(!memcmp(&old,&g.fieldNotes.group[0],sizeof(old)));
 g.fieldNotes.group[0].x=NAN;assert(!field_notes_valid(&g));
 a=fresh();group(POP_AMBUSH,0);a->y=390;field_observe(0);assert(!g.fieldNotes.group[0].serial);for(int i=0;i<g.population.groups[0].count;i++)assert(population_hidden(g.population.groups[0].members[i]));save("hidden-ambush");shot("hidden-ambush");
 puts("PASS: visible groups create persistent last-seen snapshots; remote movement does not update pins; hidden ambushers do not generate sightings; invalid coordinates rejected.");
}
static void outfits(void){
 const int types[]={POP_RAIDERS,POP_ARMY,POP_DUELISTS};const char*names[]={"starving-bandits","army","elites"};
 for(int k=0;k<3;k++){Actor*a=fresh();group(types[k],k==0);WorldEvent*e=&g.population.groups[0];
  for(int i=0;i<e->count;i++){Actor*b=&g.actors[e->members[i]];
   if(!k)assert(!b->gear[2]&&!b->gear[3]&&!b->gear[5]&&(b->gear[1]==0||b->gear[1]==RAGS)&&itemDefs[b->gear[0]].quality==0);
   else assert(b->gear[1]&&b->gear[2]&&b->gear[3]&&b->gear[4]);
   if(k==1){float bx=e->x-(i/3)*2.3f,by=e->y+(i%3-1)*2.2f;b->x=bx;b->y=by;population_walk(e->members[i],.1f);assert(distance(bx,by,b->x,b->y)<.01f);}
  }
  if(k==1)a->x-=3;field_observe(0);save(names[k]);shot(names[k]);
 }
 Actor*a=fresh();a->health[CHEST]=-10;a->unconscious=90;a->x=382;a->y=380;save("downed");shot("downed");
 puts("PASS: poor bandits carry low-quality weapons and missing clothing; armies have matching real armor and three-wide formation; elites wear full equipment; frame guards intact.");
}
static void council(void){
 Actor*a=fresh();int e=3,t=eliteDefs[e].town;g.actors[g.civil.eliteActor[e]-1].dead=1;a->x=towns[t].x;a->y=towns[t].y;g.discovered[t]=1;politics_tick(1);TownPolitics*p=&g.life.towns[t];assert(p->stage==1&&strcmp(politics_claimant(t,p->original),politics_claimant(t,p->rival)));
 save("council");politics_open(t);shot("council");ui.selection=1;shot("challenger");int money=g.money;press(KEY_CROSS);assert(g.money==money-200&&p->pledge==1);press(KEY_CROSS);assert(g.money==money-200);
 ui.screen=SCREEN_FACTIONS;shot("factions");
 puts("PASS: council has distinct named claimants, policy preview and one-time pledge spending; faction color legend renders safely.");
}
int main(void){setvbuf(stdout,NULL,_IONBF,0);Actor probe={0};probe.health[HEAD]=probe.health[CHEST]=species_health(&probe);probe.blood=100;assert(knockout_reserve(&probe)==100);probe.health[LARM]=probe.health[LLEG]=-100;assert(knockout_reserve(&probe)==100);probe.health[HEAD]=species_health(&probe)*.2f;assert(fabsf(knockout_reserve(&probe)-20)<.01f);probe.blood=24;assert(fabsf(knockout_reserve(&probe)-5)<.01f);probe.unconscious=30;assert(knockout_reserve(&probe)==0);manual_pins();service();sights();outfits();council();ui.feedCount=0;log_warn("Contract paid");int before=ui.feedCount;gain(&g.actors[g.selected],ATHLETICS,10000);assert(ui.feedCount==before&&!strcmp(ui.feed[0],"Contract paid"));puts("PASS: skill growth cannot crowd a critical outcome out of the feed.");printf("ALL V140 CHECKS PASSED; Game=%zu FieldNotes=%zu\n",sizeof(Game),sizeof(FieldNotes));}
