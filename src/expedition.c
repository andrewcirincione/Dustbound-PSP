#include "game.h"
#include "settlement_art_ids.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
extern void init_actor(int,const char*,int,int,float,float,int);
extern int hostile(const Actor*,const Actor*);
typedef struct {const char *name,*record;int model,theme,tier,floors,w,h,faction,robot,occupied;} ExpeditionSite;
#include "expedition_data.h"
static int crewOwner[MAX_ACTORS];
static const ExpeditionSite *site_def(int s){return s>=0&&s<EXP_SITES?&expeditionSites[s]:NULL;}
static int site_in(const Game*s,int b){
 if(b<0||b>=s->buildingCount)return -1;
 int v=s->frontier.variant[b];
 if(v>=BUILD_WILD_FIRST&&v<=BUILD_WILD_LAST){int i=v-BUILD_WILD_FIRST;return s->wilderness.building[i]==b+1?i:-1;}
 if(v>=BUILD_EXP_FIRST&&v<=BUILD_EXP_LAST){int i=v-BUILD_EXP_FIRST;return s->expedition.primary[i]==b+1?i+16:-1;}
 return -1;
}
int expedition_site(int b){return site_in(&g,b);}
int expedition_large(int b){return expedition_site(b)>=0;}
const char *expedition_name(int s){const ExpeditionSite*z=site_def(s);return z?z->name:"Unknown location";}
const char *expedition_record(int s){const ExpeditionSite*z=site_def(s);return z?z->record:"The record is illegible.";}
int expedition_inhabited(int s){const ExpeditionSite*z=site_def(s);return z&&z->occupied;}
int expedition_model(int b){const ExpeditionSite*z=site_def(expedition_site(b));return z?z->model:-1;}
int expedition_material(int b){const ExpeditionSite*z=site_def(expedition_site(b));return z?EXP_MATERIAL_FIRST+(z->theme==0?0:z->theme==4||z->theme==7?1:z->theme==2||z->theme==5||z->theme==1?2:3):-1;}
int expedition_levels(int b){const ExpeditionSite*z=site_def(expedition_site(b));return z?z->floors:1;}
float expedition_width(int b){const ExpeditionSite*z=site_def(expedition_site(b));return z?z->w:7;}
float expedition_height(int b){const ExpeditionSite*z=site_def(expedition_site(b));return z?z->h:7;}
static int primary(int s){return s<16?g.wilderness.building[s]-1:g.expedition.primary[s-16]-1;}
int expedition_bounds_valid(const Game*s,int room,float x,float y){int i=site_in(s,room-1);float w=i>=0?expeditionSites[i].w:7,h=i>=0?expeditionSites[i].h:7;return room>0&&room<=s->buildingCount&&isfinite(x)&&isfinite(y)&&x>=.4f&&y>=.4f&&x<=w-.4f&&y<=h-.4f;}
int expedition_floor_valid(const Game*s,int room,int floor){
 if(floor<0||floor>=EXP_FLOORS)return 0;
 if(!room)return floor==0;
 if(room<1||room>s->buildingCount)return 0;
 int i=site_in(s,room-1);if(i>=0)return floor<expeditionSites[i].floors;
 return floor==0||(floor==1&&s->buildings[room-1].type==BAR);
}
const char *expedition_floor_name(int s,int floor){
 static const char*names[8][3]={{"Machine hall","Gantry deck","Control vault"},{"Guard rooms","Signal gallery","Command loft"},{"Pilgrim hall","Reading rooms","Sealed archive"},{"Courtyard rooms","Bunk galleries","Roof refuge"},{"Loading works","Pump galleries","Reactor vault"},{"Barracks","Armory passages","Command vault"},{"Treatment wards","Dormitories","Medicine store"},{"Freight floor","Sorting galleries","Secure store"}};
 const ExpeditionSite*z=site_def(s);return z&&floor>=0&&floor<3?names[z->theme%8][floor]:"Interior";
}
static unsigned mix(unsigned x){x^=x<<13;x^=x>>17;x^=x<<5;return x?x:73819;}
static int field_clear(float x,float y){
 if(x<24||y<24||x>WORLD-24||y>WORLD-24||town_near(x,y,38)>=0)return 0;
 if(g.baseExists&&distance(x,y,g.baseX,g.baseY)<40)return 0;
 for(int b=0;b<g.buildingCount;b++)if(distance(x,y,g.buildings[b].x,g.buildings[b].y)<23)return 0;
 for(int dx=-7;dx<=7;dx+=2)for(int dy=-7;dy<=7;dy+=2)if(!can_walk(x+dx,y+dy)||terrain_road(x+dx,y+dy))return 0;
 return 1;
}
static int reward_weapon(unsigned seed,int tier){
 int candidates[ARSENAL_WEAPONS],n=0;for(int i=0;i<ARSENAL_WEAPONS;i++)if(arsenal_tier(WEAPON_ITEMS+i)==tier)candidates[n++]=WEAPON_ITEMS+i;
 return n?candidates[seed%n]:SABRE;
}
static void useful_cache(int s,int f,int c,Stack *p) {
 const ExpeditionSite *z=&expeditionSites[s];int tier=g.expedition.tier[s],last=f==z->floors-1;
 unsigned seed=mix(g.seed+s*9781u+f*337u+c*37u);
 int gearTier=tier+(f>0);if(gearTier>4)gearTier=4;
 static const int supplies[8][3]={{PARTS,COAL,COPPER},{BOLTS,MEDKIT,RATION},{RELIC,CLOTH,HERBAL_TEA},{RATION,JERKY,CLOTH},{IRONBAR,TOOLS,PARTS},{BOLTS,MEDKIT,RATION},{MEDKIT,TONIC,CLOTH},{IRONBAR,COPPER,HIDE}};
 for(int i=0;i<3;i++)p[i]=(Stack){supplies[z->theme%8][(c+i+f)%3],1+(seed>>(i*4))%(2+tier)};
 if(z->occupied&&tier<2){p[0]=(Stack){c%2?RATION:PARTS,2+seed%4};p[1]=(Stack){c%2?MEDKIT:BEDROLL_ITEM,1};}
 if(c==1&&!z->occupied){p[0]=(Stack){reward_weapon(seed,gearTier),1};p[1]=(Stack){BOLTS,12+6*tier};}
 if(c==2&&f>0){p[0]=(Stack){armor_cache_item(seed^3917u,gearTier),1};p[1]=(Stack){z->robot?PARTS:MEDKIT,2+tier};}
 if(c==3&&last){p[0]=(Stack){reward_weapon(seed,gearTier),1};p[1]=(Stack){armor_cache_item(seed^3917u,gearTier),1};p[2]=(Stack){z->robot||z->theme==2?RELIC:TONIC,1+tier};}
}
static void upgrade_interiors(void) {
 if(g.repairs.interiorsVersion)return;
 for(int s=0;s<EXP_SITES;s++)for(int f=0;f<expeditionSites[s].floors;f++)for(int c=0;c<EXP_CHESTS;c++){
  Stack desired[3];useful_cache(s,f,c,desired);
  for(int i=0;i<3;i++)if(g.expedition.loot[s][f][c][i].qty>0){
   /* An empty or partly emptied cache never gains replacement quantities. */
   int remaining=g.expedition.loot[s][f][c][i].qty;
   g.expedition.loot[s][f][c][i]=desired[i];
   if(g.expedition.loot[s][f][c][i].qty>remaining)g.expedition.loot[s][f][c][i].qty=remaining;
  }
 }
 g.repairs.interiorsVersion=1;
}
void expedition_init(void){
 if(g.expedition.version){upgrade_interiors();memset(crewOwner,0,sizeof(crewOwner));for(int s=0;s<EXP_SITES;s++)for(int f=0;f<EXP_FLOORS;f++){ExpeditionCrew*c=&g.expedition.crew[s][f];if(c->initialized)for(int m=0;m<c->count;m++)if(c->live[m]>=0)crewOwner[c->live[m]]=1+(s*EXP_FLOORS+f)*EXP_GUARDS+m;}return;}
 memset(crewOwner,0,sizeof(crewOwner));
 memset(&g.expedition,0,sizeof(g.expedition));g.expedition.version=1;
 for(int i=0;i<EXP_ADDITIONS;i++){
  if(g.buildingCount>MAX_BUILDINGS-24)break;
  unsigned seed=mix(g.seed^(i+1)*2654435761u);float ox=100+(i%6)*260,oy=100+(i/6)*270,x=0,y=0;int found=0;
  for(int r=0;r<28&&!found;r++)for(int a=0;a<24;a++){float angle=(a+i*.7f)*.2617994f;x=ox+cosf(angle)*r*5;y=oy+sinf(angle)*r*5;if(field_clear(x,y)){found=1;break;}}
  if(!found)continue;
  const ExpeditionSite*z=&expeditionSites[i+16];int type=z->theme==0||z->theme==4?FORGE:z->theme==1||z->theme==5?TOWER:z->theme==3?BAR:z->theme==7?STORE:HOUSE;
  int b=add_build(x,y,type,z->occupied?z->faction:FREE,-1);if(b<0)continue;g.frontier.variant[b]=BUILD_EXP_FIRST+i;g.expedition.primary[i]=b+1;
  int decor=add_build(x-7,y+5,z->theme==0||z->theme==4?WINDMILL:z->occupied?CAMPFIRE:WELL,FREE,-1);if(decor>=0)g.frontier.variant[decor]=BUILD_EXP_FIRST+i;
  (void)seed;
 }
 for(int s=0;s<EXP_SITES;s++){
  int b=primary(s);const ExpeditionSite*z=&expeditionSites[s];if(b<0)continue;
  int tier=z->tier;if(starter_country(g.buildings[b].x,g.buildings[b].y)&&tier>1)tier=1;g.expedition.tier[s]=tier;
  for(int f=0;f<z->floors;f++)for(int c=0;c<EXP_CHESTS;c++)useful_cache(s,f,c,g.expedition.loot[s][f][c]);
 }
 g.repairs.interiorsVersion=1;
 /* Original v33 residents are superseded by per-floor crews, not duplicated. */
 for(int i=0;i<16;i++)if(g.wilderness.guardSerial[i]){
  int k=48+i;if(g.population.groups[k].serial!=g.wilderness.guardSerial[i])continue;
  WorldEvent*e=&g.population.groups[k];ExpeditionCrew*c=&g.expedition.crew[i][0];memset(c,0,sizeof(*c));c->initialized=1;c->count=e->count;for(int m=0;m<EXP_GUARDS;m++)c->live[m]=-1;
  for(int m=0;m<e->count&&m<EXP_GUARDS;m++){
   int n=e->spawned?e->members[m]:-1;c->actor[m]=n>=0?g.actors[n]:e->reserve[m];c->depth[m]=n>=0?g.depth.actors[n]:e->reserveDepth[m];c->actor[m].homeX=c->depth[m].rx;c->actor[m].homeY=c->depth[m].ry;
   if(n>=0){c->live[m]=n;crewOwner[n]=1+(i*EXP_FLOORS)*EXP_GUARDS+m;g.living.owned[n]=0;g.population.state[k].detached|=1<<m;e->members[m]=-1;}
   e->reserve[m].dead=1;e->reserveDepth[m].room=0;
  }
  g.population.state[k].phase=POP_RUINS;
 }
}
int expedition_validate(const Game*s){
 if(s->expedition.version<0||s->expedition.version>1)return 0;
 for(int i=0;i<EXP_ADDITIONS;i++){int b=s->expedition.primary[i];if(b<0||b>s->buildingCount||(b&&s->frontier.variant[b-1]!=BUILD_EXP_FIRST+i))return 0;}
 unsigned char used[MAX_ACTORS]={0};
 for(int i=0;i<EXP_SITES;i++){
  if(s->expedition.tier[i]>4)return 0;
  for(int f=0;f<EXP_FLOORS;f++){
   const ExpeditionCrew*c=&s->expedition.crew[i][f];if(c->initialized<0||c->initialized>1||c->count<0||c->count>EXP_GUARDS)return 0;
   if(c->initialized)for(int m=0;m<c->count;m++){int n=c->live[m];if(n<-1||n>=s->actorCount||(n>=0&&(used[n]++||s->actors[n].squad)))return 0;
    const Actor*a=n>=0?&s->actors[n]:&c->actor[m];if(!memchr(a->name,0,sizeof(a->name))||!isfinite(a->blood)||!isfinite(a->hunger))return 0;
    for(int l=0;l<LIMBS;l++)if(!isfinite(a->health[l]))return 0;
    for(int p=0;p<PACK;p++)if(a->pack[p].item<0||a->pack[p].item>=ITEMS||a->pack[p].qty<0||a->pack[p].qty>999)return 0;
   }
   for(int c=0;c<EXP_CHESTS;c++)for(int p=0;p<3;p++){Stack q=s->expedition.loot[i][f][c][p];if(q.item<0||q.item>=ITEMS||q.qty<0||q.qty>999)return 0;}
  }
 }
 return 1;
}
static void line_wall(float *lines,int *n,int max,float x0,float y0,float x1,float y1) {
 if(*n<max && fabsf(x1-x0)+fabsf(y1-y0)>.1f){float *p=lines+4*(*n)++;p[0]=x0;p[1]=y0;p[2]=x1;p[3]=y1;}
}
static void doorway(float *lines,int *n,int max,int horizontal,float at,float start,float end,float door) {
 float lo=fmaxf(start,door-1.25f),hi=fminf(end,door+1.25f);
 if(horizontal){line_wall(lines,n,max,start,at,lo,at);line_wall(lines,n,max,hi,at,end,at);}
 else{line_wall(lines,n,max,at,start,at,lo);line_wall(lines,n,max,at,hi,at,end);}
}
static int make_walls(int b,int floor,float *lines,int max) {
 int s=expedition_site(b);if(s<0)return 0;const ExpeditionSite*z=&expeditionSites[s];
 int n=0;float w=z->w,h=z->h;
 if(z->theme==5){
  if(!floor){ /* Sleeping wings around a clear central aisle and entry hall. */
   doorway(lines,&n,max,0,8,0,h-7,7);doorway(lines,&n,max,0,w-8,0,h-7,7);
   doorway(lines,&n,max,1,h-7,0,8,3.5f);doorway(lines,&n,max,1,h-7,8,w-8,w/2);doorway(lines,&n,max,1,h-7,w-8,w,w-3.5f);
  }else if(floor==1){ /* Armory, maintenance bay and a separate supply office. */
   doorway(lines,&n,max,0,w*.42f,0,h-4,h*.48f);
   doorway(lines,&n,max,1,h*.55f,w*.42f,w,w*.7f);
  }else{ /* Two command offices open onto a broad briefing room. */
   doorway(lines,&n,max,1,8,0,w/2,4);doorway(lines,&n,max,1,8,w/2,w,w-4);
   doorway(lines,&n,max,0,w/2,0,8,4);
  }
 }else if(z->theme==0||z->theme==4){
  if(!floor)doorway(lines,&n,max,0,w-7,0,h-4,h/2);
  else if(floor==1)doorway(lines,&n,max,1,h/2,0,w,w/2);
  else{doorway(lines,&n,max,1,8,0,w,w/2);doorway(lines,&n,max,0,w/2,0,8,4);}
 }else if(z->theme==3){
  if(!floor){doorway(lines,&n,max,0,7,0,h-8,h/2);doorway(lines,&n,max,0,w-7,0,h-8,h/2);}
  else doorway(lines,&n,max,1,h/2,0,w,w/2);
 }else if(z->theme==6){
  doorway(lines,&n,max,1,h/2,0,w,w/2);
  if(floor)doorway(lines,&n,max,0,w/2,0,h/2,h/4);
 }else if(z->theme==2){
  if(floor){doorway(lines,&n,max,1,h*.4f,0,w,w/2);if(floor==2)doorway(lines,&n,max,0,w/2,0,h*.4f,h*.2f);}
 }else if(z->theme==1){
  doorway(lines,&n,max,1,floor?8:h-7,0,w,w/2);
  if(floor==2)doorway(lines,&n,max,0,w/2,0,8,4);
 }else{ /* Loading hall below, sorting rooms above, secure store at the top. */
  if(floor)doorway(lines,&n,max,0,w/2,0,h-5,h/2);
  else doorway(lines,&n,max,1,7,0,w,w/2);
 }
 return n;
}
/* Fixed site geometry is shared by all path probes, actors and render passes. */
int expedition_walls(int b,int floor,float *lines,int max) {
 typedef struct{unsigned seed;int b,floor,n;float lines[256];} WallCache;
 static WallCache cache[4];static unsigned cursor;WallCache *c=NULL;
 for(int i=0;i<4;i++)if(cache[i].n&&cache[i].seed==g.seed&&cache[i].b==b&&cache[i].floor==floor){c=&cache[i];break;}
 if(!c){c=&cache[cursor++%4];c->seed=g.seed;c->b=b;c->floor=floor;c->n=make_walls(b,floor,c->lines,64);}
 int n=c->n<max?c->n:max;if(n)memcpy(lines,c->lines,n*4*sizeof(float));return n;
}
int expedition_walkable(int b,int floor,float x,float y){
 if(!expedition_large(b))return 1;
 float w=expedition_width(b),h=expedition_height(b);if(x<.4f||y<.4f||x>w-.4f||y>h-.4f)return 0;
 float walls[256];int n=expedition_walls(b,floor,walls,64);
 for(int i=0;i<n;i++){float*p=walls+4*i;if(p[0]==p[2]){if(fabsf(x-p[0])<.20f&&y>p[1]-.15f&&y<p[3]+.15f)return 0;}else if(fabsf(y-p[1])<.20f&&x>p[0]-.15f&&x<p[2]+.15f)return 0;}
 return 1;
}
int expedition_stair_point(int b,int floor,int up,float*x,float*y){
 int levels=expedition_levels(b);if((up&&floor+1>=levels)||(!up&&floor==0))return 0;
 int source=up?floor:floor-1;*x=source%2?2:expedition_width(b)-2;*y=source%2?expedition_height(b)-2:2;return 1;
}
static void prop(InteriorProp*p,int*n,float x,float y,float w,float d,int art,int action){if(*n<ROOM_PROPS)p[(*n)++]=(InteriorProp){x,y,w,d,art,action};}
static void chest_point(int b,int floor,int c,float*x,float*y){
 float w=expedition_width(b),h=expedition_height(b);
 *x=c&1?w-5:2.4f;*y=c&2?h-4:2.4f;
 if(floor==1){*x=c&1?w-3:3;*y=c&2?h-6:5;}
 if(floor==2){*x=c&1?w-3:3;*y=c&2?h-6:3;}
}
int expedition_props(int b,int floor,InteriorProp*p){
 int s=expedition_site(b);if(s<0)return 0;const ExpeditionSite*z=&expeditionSites[s];int n=0;float w=z->w,h=z->h;
 for(int c=0;c<EXP_CHESTS;c++){float x,y;chest_point(b,floor,c,&x,&y);prop(p,&n,x,y,.46f,.42f,20,ROOM_EXP_CACHE);}
 prop(p,&n,5,2,.7f,.32f,17,ROOM_RUIN_RECORD);
 float x,y;if(expedition_stair_point(b,floor,1,&x,&y))prop(p,&n,x,y,.7f,.55f,ART_INN_STAIRS,ROOM_EXP_UP);
 if(expedition_stair_point(b,floor,0,&x,&y))prop(p,&n,x,y,.7f,.55f,ART_STAIRS_DOWN,ROOM_EXP_DOWN);
 if(z->occupied&&floor==0)prop(p,&n,5,h-3,.8f,.35f,17,ROOM_EXP_TRADE);
 int theme=z->theme;
 if(theme==5){
  if(!floor){
   for(int side=0;side<2;side++)for(int row=0;row<4;row++){
    float bx=side?w-3:3,by=5+row*3;
    prop(p,&n,bx,by,.58f,.81f,16,ROOM_REST);
   }
   prop(p,&n,w/2,h-3,1.1f,.55f,17,ROOM_NONE);prop(p,&n,w/2+3,h-3,.6f,.45f,ART_IRON_STOVE,ROOM_NONE);
  }else if(floor==1){
   prop(p,&n,3,9,.6f,.45f,ART_ARMOR_STAND,ROOM_NONE);prop(p,&n,3,13,.6f,.45f,ART_ARMOR_STAND,ROOM_NONE);
   prop(p,&n,w*.65f,4,1.1f,.55f,17,ROOM_NONE);prop(p,&n,w-3,10,.7f,.45f,21,ROOM_NONE);
   prop(p,&n,w*.65f,h-4,.8f,.5f,19,ROOM_NONE);prop(p,&n,6,h-4,.7f,.45f,ART_CARAVAN_BALES,ROOM_NONE);
  }else{
   prop(p,&n,w/2,h/2+2,1.3f,.65f,17,ROOM_NONE);
   prop(p,&n,7,3,.7f,.45f,21,ROOM_NONE);prop(p,&n,w-7,3,.7f,.45f,21,ROOM_NONE);
   prop(p,&n,5,h/2,.58f,.81f,ART_LOW_COT,ROOM_REST);
  }
 }else if(theme==6||(theme==3&&floor>0)){
  for(int side=0;side<2;side++)for(int row=0;row<3;row++)prop(p,&n,side?w-3:3,7+row*4,.58f,.81f,theme==6?ART_LOW_COT:16,ROOM_REST);
  prop(p,&n,w/2,4,.8f,.4f,17,ROOM_NONE);prop(p,&n,w-4,h-3,.6f,.4f,ART_IRON_STOVE,ROOM_NONE);
 }else if(theme==3){
  prop(p,&n,3,7,.58f,.81f,16,ROOM_REST);prop(p,&n,w-3,7,.58f,.81f,16,ROOM_REST);
  prop(p,&n,w/2,7,.8f,.4f,17,ROOM_NONE);prop(p,&n,w/2+3,7,.6f,.4f,ART_IRON_STOVE,ROOM_NONE);
  prop(p,&n,3,h/2+3,.7f,.45f,ART_CARAVAN_BALES,ROOM_NONE);prop(p,&n,w-3,h/2+3,.7f,.45f,ART_CARAVAN_BALES,ROOM_NONE);
 }else if(theme==0||theme==4){
  if(!floor){
   for(int j=0;j<3;j++)prop(p,&n,5+j*5,h/2,1.0f,.7f,theme==4?19:ART_IRON_STOVE,ROOM_NONE);
   prop(p,&n,w-3,h/2-4,.6f,.4f,21,ROOM_NONE);
  }else if(floor==1){
   for(int j=0;j<3;j++)prop(p,&n,6+j*5,3,.7f,.45f,21,ROOM_NONE);
   prop(p,&n,w/2,h-6,1.0f,.6f,19,ROOM_NONE);
  }else{prop(p,&n,w/2,h/2,1.2f,.5f,ART_RESEARCH_TABLE,ROOM_NONE);prop(p,&n,w-6,4,.7f,.4f,21,ROOM_NONE);}
 }else if(theme==2){
  if(!floor){prop(p,&n,w/2,3,1.0f,.65f,ART_SIDE_ALTAR,ROOM_NONE);for(int row=0;row<2;row++)for(int side=0;side<2;side++)prop(p,&n,w/2+(side?4:-4),10+row*5,1.0f,.4f,17,ROOM_NONE);}
  else{for(int j=0;j<3;j++)prop(p,&n,3+j*(w-6)/2,7,.65f,.45f,21,ROOM_NONE);prop(p,&n,w/2,h/2+3,1.1f,.5f,ART_RESEARCH_TABLE,ROOM_NONE);}
 }else if(theme==1){
  if(!floor){prop(p,&n,3,7,.58f,.81f,16,ROOM_REST);prop(p,&n,w-3,7,.58f,.81f,16,ROOM_REST);prop(p,&n,w/2,h-3,.9f,.4f,17,ROOM_NONE);}
  else{prop(p,&n,w/2,h/2,.9f,.4f,ART_RESEARCH_TABLE,ROOM_NONE);prop(p,&n,w-3,6,.6f,.4f,21,ROOM_NONE);}
 }else{
  for(int side=0;side<2;side++)for(int row=0;row<2;row++)prop(p,&n,side?w-5:5,10+row*5,.8f,.5f,floor?21:ART_CARAVAN_BALES,ROOM_NONE);
  if(z->occupied){prop(p,&n,w/2,4,.8f,.4f,17,ROOM_NONE);prop(p,&n,3,7,.58f,.81f,16,ROOM_REST);}
 }
 return n;
}
int expedition_cache_index(int b,int floor,float x,float y){int s=expedition_site(b);if(s<0||floor<0||floor>=EXP_FLOORS)return -1;for(int c=0;c<EXP_CHESTS;c++){float px,py;chest_point(b,floor,c,&px,&py);if(distance(x,y,px,py)<.2f)return (s*EXP_FLOORS+floor)*EXP_CHESTS+c;}return -1;}
Stack *expedition_cache(int encoded){if(encoded<0||encoded>=EXP_SITES*EXP_FLOORS*EXP_CHESTS)return NULL;int c=encoded%EXP_CHESTS,f=encoded/EXP_CHESTS%EXP_FLOORS,s=encoded/EXP_CHESTS/EXP_FLOORS;return g.expedition.loot[s][f][c];}
int expedition_cache_owner(int encoded){int s=encoded/(EXP_FLOORS*EXP_CHESTS);const ExpeditionSite*z=site_def(s);return z&&z->occupied?z->faction:FREE;}
void expedition_stairs(Actor*a,int up){
 int room=depth_room(a),b=room-1,floor=actor_floor(a),next=floor+(up?1:-1);float x,y;
 if(!expedition_large(b)||next<0||next>=expedition_levels(b)||!expedition_stair_point(b,floor,up,&x,&y))return;
 int n=a-g.actors;unsigned char follow[MAX_ACTORS]={0};if(n==g.selected)for(int j=0;j<g.actorCount;j++)if(j!=n&&g.actors[j].squad&&assigned_job(&g.actors[j])==JOB_FOLLOW&&depth_distance(a,&g.actors[j])<2)follow[j]=1;
 g.innFloor[n]=next;g.depth.actors[n].rx=x-1.2f;g.depth.actors[n].ry=y;a->routeLen=a->routePos=a->attackPhase=0;a->target=-1;
 if(n==g.selected){g.interiorX=x-1.2f;g.interiorY=y;}
 if(a->carrying>=0){g.innFloor[a->carrying]=next;g.depth.actors[a->carrying].rx=x-1.2f;g.depth.actors[a->carrying].ry=y;}
 for(int j=0;j<g.actorCount;j++)if(follow[j]){g.innFloor[j]=next;g.depth.actors[j].rx=x-1.8f;g.depth.actors[j].ry=y;g.actors[j].routeLen=0;}
 log_msg("%s / %s",expedition_name(expedition_site(b)),expedition_floor_name(expedition_site(b),next));
}
int expedition_robot(const Actor*a){return a->kind==BEAST&&a->race==2&&a->voice>=77&&a->voice<=78?a->voice-77:-1;}
int expedition_owns(int n){return n>=0&&n<MAX_ACTORS&&crewOwner[n]>0;}
static void create_crew(int s,int floor){
 ExpeditionCrew*c=&g.expedition.crew[s][floor];if(c->initialized)return;
 const ExpeditionSite*z=&expeditionSites[s];int b=primary(s),tier=g.expedition.tier[s];memset(c,0,sizeof(*c));c->initialized=1;c->count=z->occupied?(floor?2:3):2+tier/2;c->stockDay=g.day;
 for(int m=0;m<EXP_GUARDS;m++)c->live[m]=-1;
 for(int m=0;m<c->count;m++){
  Actor*a=&c->actor[m];DepthActor*d=&c->depth[m];unsigned seed=mix(g.seed+s*8917u+floor*179u+m*37u);int boss=m==0&&floor==z->floors-1&&!z->occupied;int level=2+tier*13+floor*4+(boss?6:0);
  memset(a,0,sizeof(*a));memset(d,0,sizeof(*d));d->upkeep=100;d->room=b+1;
  a->active=1;a->kind=z->robot?BEAST:z->occupied?(m?GUARD:floor?SETTLER:TRADER):RAIDER;a->faction=z->occupied?z->faction:HOSTILE;
  a->race=z->robot?2:seed%2;a->voice=z->robot?77+(m+floor)%2:z->occupied?(m?ROLE_MARSHAL:ROLE_MERCHANT):0;
  snprintf(a->name,sizeof(a->name),"%s",z->robot?boss?"Core Warden":m%2?"Iron Crawler":"Old Sentinel":z->occupied?m?"Outpost guard":"Outpost trader":boss?"Vault keeper":"Ruin scavenger");
  a->healthScale=z->robot?125+tier*35+floor*15+(boss?65:0):100+tier*10;a->body=seed%3;a->skin=seed%8;a->hair=seed%7;a->hairColor=seed%6;a->beard=z->robot?0:(seed>>9)%8;
  a->target=a->attackTarget=a->carrying=a->carriedBy=a->prisonTown=-1;a->homeTown=-1;
  a->x=g.buildings[b].x;a->y=g.buildings[b].y;a->blood=a->hunger=100;
  for(int l=0;l<LIMBS;l++)a->health[l]=species_health(a);
  for(int k=0;k<SKILLS;k++)a->xp[k]=12*(level-1)*(level-1);
  if(!z->robot){living_gear(a,m,tier,seed%10000);armor_wanderer(a,seed,tier);arsenal_outfit(a,seed,tier);inventory_add(a,MEDKIT,1);inventory_add(a,RATION,3);}else{inventory_add(a,PARTS,2+tier);inventory_add(a,IRONBAR,1+tier);if(boss)inventory_add(a,RELIC,2);}
  float px=m%2?z->w-5:5,py=m<2?z->h-5:5;
  if(boss){px=z->w-3;py=z->h-5;}
  if(z->occupied){px=m?6:5;py=z->h-3.8f;}
  d->rx=a->homeX=a->tx=px;d->ry=a->homeY=a->ty=py;
  if(z->occupied&&floor==0&&m==1){d->room=0;a->x+=3.5f;a->y+=3.5f;a->homeX=a->tx=a->x;a->homeY=a->ty=a->y;}

  if(z->occupied&&!m){inventory_add(a,RATION,6);inventory_add(a,MEDKIT,4);inventory_add(a,PARTS,4);inventory_add(a,BEDROLL_ITEM,2);}
 }
}
static int squad_present(int room,int floor){for(int n=0;n<g.actorCount;n++)if(g.actors[n].squad&&!g.actors[n].dead&&depth_room(&g.actors[n])==room&&actor_floor(&g.actors[n])==floor)return 1;return 0;}
static void store_crew(int s,int f){
 ExpeditionCrew*c=&g.expedition.crew[s][f];if(!c->initialized)return;
 for(int m=0;m<c->count;m++){int n=c->live[m];if(n<0)continue;Actor*a=&g.actors[n];
  if(a->squad){c->actor[m].active=0;c->live[m]=-1;crewOwner[n]=0;continue;}
  if(a->carriedBy>=0||a->carrying>=0||merc_slot(a)>=0||g.society.freed[n])continue;
  c->actor[m]=*a;c->depth[m]=g.depth.actors[n];c->actor[m].target=c->actor[m].attackTarget=-1;c->actor[m].attackPhase=0;
  a->active=0;g.living.owned[n]=-1;c->live[m]=-1;crewOwner[n]=0;
  for(int j=0;j<g.actorCount;j++){if(g.actors[j].target==n)g.actors[j].target=-1;if(g.actors[j].attackTarget==n){g.actors[j].attackTarget=-1;g.actors[j].attackPhase=0;}}
 }
}
static int crew_needed(int s,int floor){
 int b=primary(s);if(b<0)return 0;
 if(squad_present(b+1,floor))return 1;
 if(floor==0&&expeditionSites[s].occupied)for(int n=0;n<g.actorCount;n++)if(g.actors[n].squad&&!g.actors[n].dead&&!actor_inside(&g.actors[n])&&distance(g.actors[n].x,g.actors[n].y,g.buildings[b].x,g.buildings[b].y)<30)return 1;
 return 0;
}
void expedition_tick(float dt){
 if(!g.expedition.version)return;
 for(int s=0;s<EXP_SITES;s++)for(int f=0;f<EXP_FLOORS;f++)if(g.expedition.crew[s][f].initialized&&!crew_needed(s,f))store_crew(s,f);
 for(int s=0;s<EXP_SITES;s++){int b=primary(s);if(b<0)continue;
  Actor*lead=&g.actors[g.selected];if(!actor_inside(lead)&&distance(lead->x,lead->y,g.buildings[b].x,g.buildings[b].y)<22&&!(g.expedition.discovered[s/32]&(1u<<(s%32)))){g.expedition.discovered[s/32]|=1u<<(s%32);log_msg("Found %s.",expedition_name(s));}
  for(int f=0;f<expeditionSites[s].floors;f++)if((depth_room(lead)==b+1&&actor_floor(lead)==f)||(f==0&&expeditionSites[s].occupied&&!actor_inside(lead)&&distance(lead->x,lead->y,g.buildings[b].x,g.buildings[b].y)<30)){
   create_crew(s,f);ExpeditionCrew*c=&g.expedition.crew[s][f];
   for(int m=0;m<c->count;m++)if(c->live[m]<0&&c->actor[m].active){int n=-1;for(int j=0;j<g.actorCount;j++)if(g.living.owned[j]<0){n=j;break;}if(n<0&&g.actorCount<MAX_ACTORS)n=g.actorCount++;if(n<0)break;
    society_actor_reset(n);g.actors[n]=c->actor[m];g.depth.actors[n]=c->depth[m];g.innFloor[n]=f;g.living.owned[n]=0;g.living.anger[n]=0;g.living.command[n]=-1;c->live[m]=n;crewOwner[n]=1+(s*EXP_FLOORS+f)*EXP_GUARDS+m;
    for(int k=0;k<WEAPON_XP_SLOTS;k++)g.society.weaponXP[n][k]=g.actors[n].xp[MELEE];
   }
  }
 }
 (void)dt;
}
int expedition_clear_sight(int b,int floor,float x,float y,float tx,float ty){int steps=(int)(distance(x,y,tx,ty)*4)+1;for(int j=1;j<steps;j++)if(!expedition_walkable(b,floor,x+(tx-x)*j/steps,y+(ty-y)*j/steps))return 0;return 1;}
int expedition_actor_tick(int n,float dt){
 if(!expedition_owns(n))return 0;Actor*a=&g.actors[n];if(!conscious(a)||a->attackPhase)return 1;
 int room=depth_room(a),floor=actor_floor(a),target=-1;float best=16;
 for(int j=0;j<g.actorCount;j++){Actor*b=&g.actors[j];if(j==n||!conscious(b)||depth_room(b)!=room||actor_floor(b)!=floor||!hostile(a,b))continue;
  float d=depth_distance(a,b);if(d<best&&(a->target==j||expedition_clear_sight(room-1,floor,depth_x(a),depth_y(a),depth_x(b),depth_y(b)))){best=d;target=j;}
 }
 a->target=target;if(target>=0){Actor*b=&g.actors[target];float range=a->gear[0]?itemDefs[a->gear[0]].range:1.65f;
  if(best<=range&&expedition_clear_sight(room-1,floor,depth_x(a),depth_y(a),depth_x(b),depth_y(b)))strike(n,target);
  else room_route_toward(a,depth_x(b),depth_y(b),dt);
 }else if(distance(depth_x(a),depth_y(a),a->homeX,a->homeY)>1)room_route_toward(a,a->homeX,a->homeY,dt*.5f);
 return 1;
}
void expedition_services(int s){
 if(s<0||s>=EXP_SITES||!expeditionSites[s].occupied){log_msg("No trader occupies this site.");return;}
 ExpeditionCrew*c=&g.expedition.crew[s][0];int n=c->live[0];if(n<0||!conscious(&g.actors[n])||hostile(&g.actors[n],&g.actors[g.selected])){log_msg("The trading counter is unattended.");return;}
 if(c->stockDay<g.day){if(inventory_count(&g.actors[n],RATION)<6)inventory_add(&g.actors[n],RATION,4);if(inventory_count(&g.actors[n],MEDKIT)<4)inventory_add(&g.actors[n],MEDKIT,2);c->stockDay=g.day;}
 ui.roadTrader=n+1;ui.tradeTown=-1;ui.selection=ui.scroll=0;ui.tradeSmith=0;ui.screen=SCREEN_TRADE;
}
