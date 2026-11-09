#include "game.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int conscious(const Actor*);
extern int add_build(float,float,int,int,int);
extern void init_actor(int,const char*,int,int,float,float,int);
#define C (g.civil)
const char *raceNames[SPECIES]={"LOWLANDER","ASHLANDER","CLOCKWORK","REEDKIN","STONEKIN"};
const SpeciesDef speciesDefs[SPECIES]={
 {"Lowlander","Balanced traveler. Starts with better medicine and perception.",100,0,1,1,
  {[MEDICINE]=5,[PERCEPTION]=3}},
 {"Ashlander","Survives on little food. Tough and observant, with a lighter load.",105,-4,.98f,.58f,
  {[STRENGTH]=-2,[DEXTERITY]=2,[TOUGHNESS]=8,[STEALTH]=4,[PERCEPTION]=6}},
 {"Clockwork","Immune to hunger and bleeding. Repairs with parts instead of bandages.",120,9,.88f,0,
  {[STRENGTH]=5,[TOUGHNESS]=5,[ENGINEERING]=12,[SCIENCE]=7,[ATHLETICS]=-2,[STEALTH]=-2}},
 {"Reedkin","Fast, quiet, and hard to hit. Fragile and carries less.",74,-9,1.23f,.80f,
  {[STRENGTH]=-4,[DEXTERITY]=6,[TOUGHNESS]=-2,[ATHLETICS]=8,[DODGE]=10,[STEALTH]=8}},
 {"Stonekin","Powerful hauler with thick skin. Slow and needs more food.",145,16,.79f,1.48f,
  {[STRENGTH]=9,[TOUGHNESS]=9,[LABOUR]=7,[DEXTERITY]=-2,[ATHLETICS]=-2,[STEALTH]=-3}}
};
int species_index(const Actor*a){return a->race>=0&&a->race<SPECIES?a->race:0;}
float species_health(const Actor*a){return a->kind==BEAST?100:speciesDefs[species_index(a)].health;}
int actor_capacity(const Actor*a){return 30+effective_level(a,STRENGTH)*2+gear_bonus(a,3)+speciesDefs[species_index(a)].carry+limb_capacity_bonus(a);}
float species_food(const Actor*a){return speciesDefs[species_index(a)].food;}
int species_start_level(int origin,int race,int skill){
 static const int strength[5]={9,12,16,3,7};int n=skill==STRENGTH?strength[origin]:3;
 if(origin==1&&skill==MELEE)n=8;if(origin==1&&skill==DEFENCE)n=7;
 if(origin==2&&skill==LABOUR)n=12;if(origin==2&&skill==SMITHING)n=8;
 if(origin==3&&skill==STEALTH)n=12;if(origin==3&&skill==THEFT)n=10;
 n+=speciesDefs[race].bonus[skill];return n<1?1:n>100?100:n;
}
void species_start(Actor*a,int origin,int race){
 a->race=race;for(int s=0;s<SKILLS;s++){int l=species_start_level(origin,race,s);a->xp[s]=12*(l-1)*(l-1);}
 for(int l=0;l<LIMBS;l++)a->health[l]=species_health(a);
}
const EliteDef eliteDefs[ELITES]={
 {"Shogun Raizen","Iron sovereign","Raizen's Last Edict","The Iron Throne", "Raizen's court regalia; a piercing oathblade and a heavy command cuirass.",11,0,68,0,47,SHOGUN},
 {"Abbess Ember","Ash pontiff","Ember's Censer","Vestments of Cinders","The abbess carries a staggering censer-staff and sealed ceremonial robes.",7,1,62,16,39,MONKS},
 {"Vulture King","Feast sovereign","King's Hunger","The Red Banquet","The flesh king's serrated cleaver and intimidating bone mantle.",14,4,70,87,37,HOSTILE},
 {"Provost Cairn","Compact governor","Cairn's Equalizer","Charter Coat","The compact's governor wears a guarded coat and wields a balanced maul.",0,0,45,85,42,UNION},
 {"Scrap Queen","Mesa ruler","Queen's Warrant","Crown of Scrap","A machine ruler's armor-piercing rifle and industrial plated mantle.",8,2,64,29,38,OUTLAWS},
 {"Speaker Moss","Grove speaker","Moss's Quiet Reach","Living Reedweave","A reedkin speaker's long-reaching spear and quiet, light carapace.",15,3,57,5,43,MONKS},
 {"Needle Saint","Caravan champion","Saint's Mercy","Road-Saint Brigandine","A quick dueling blade and weatherproof armor worn by Copper's champion.",1,3,42,3,36,SCAVENGERS},
 {"Salt Marshal","Saltwatch champion","The Dry Verdict","Salt Marshal's Shell","A saltwatch marshal's hard-hitting polearm and dust-sealed plated coat.",2,1,48,86,42,UNION},
 {"Dyer Regent","Reed market leader","Regent's Hook","Dyer's Silk Guard","A hooked saber and light guarded coat mark the market's protector.",3,3,43,17,44,SCAVENGERS},
 {"Gatewarden Yara","Iron gate champion","Yara's Gatebreaker","The Bastion","A stonekin sentinel's penetrating war pick and exceptionally heavy plate.",6,4,60,12,38,SHOGUN},
 {"Admiral Soot","Cinder commander","Soot's Broadside","Cinder Officer's Coat","The port commander fights with a scattergun and load-bearing officer coat.",10,0,52,31,45,UNION},
 {"Winter Oracle","Pale sanctuary leader","Oracle's Bell","Winter's Silence","A staggering temple staff and sealed winter vestments defend the pass.",12,1,54,16,40,MONKS},
 {"Glass Ronin","Wandering duelist","Glass Horizon","Cracked Mirror Haori","A roaming duelist trades armor for an exceptionally quick, accurate katana.",-1,0,55,0,44,FREE},
 {"Nine Knives","Wandering cutthroat","The Ninth Answer","Ninefold Shroud","A knife-fighter's serrated blade and stealthy outlaw mantle.",-1,3,49,9,40,OUTLAWS},
 {"Iron Pilgrim","Wandering machine","Pilgrim's Burden","Pilgrim's Iron Skin","A clockwork pilgrim carries a huge blunt weapon and weather-sealed plate.",-1,2,63,15,38,FREE},
 {"The Bellmaker","Wandering giant","Last Bell","Bellfounder's Apron","A stonekin smith's staggering hammer and industrial armor.",-1,4,58,14,35,FREE},
 {"Red Surveyor","Wandering hunter","Far Red Line","Surveyor's Dustcoat","A patient marksman's accurate long rifle and sealed expedition coat.",-1,1,51,29,32,OUTLAWS},
 {"Hollow Prince","Exiled claimant","Empty Crown","Prince Without Land","An exile carries a long heavy blade and guarded royal lamellar.",-1,0,66,84,34,OUTLAWS}
};
static char gearNames[132][40],gearDesc[132][112];
int regalia_owner(int item){return item>=REGALIA_ITEMS&&item<REGALIA_ITEMS+36?(item-REGALIA_ITEMS)/2:-1;}
void civil_content(void){
 static const char*name[4]={"Court brigandine","Reed carapace","Basalt plate","Censor vestments"};
 static const int family[4]={36,43,38,39},armor[4]={22,10,38,8},weight[4]={9,3,19,3},speed[4]={-4,6,-17,2},stealth[4]={-5,12,-24,4},effect[4]={4,6,4,9};
 for(int k=0;k<4;k++)for(int v=0;v<24;v++){int n=k*24+v,id=CIV_ARMOR_ITEMS+n,m=v/4,q=v%4;ItemDef*d=&itemDefs[id];*d=itemDefs[gear_id(family[k],m,q)];
  snprintf(gearNames[n],40,"%s %s %s",qualityNames[q],materialNames[m],name[k]);d->name=gearNames[n];d->desc=gearDesc[n];d->armor=armor[k]+m*2+q*4;d->weight=weight[k];d->speed=speed[k];d->stealth=stealth[k];d->effect=effect[k];d->potency=5+m+q*2;d->protection=k==3?35:0;d->price=140+m*70+q*q*110+d->armor*10;
  snprintf(gearDesc[n],112,"%s. Armor %d; move %+d; stealth %+d; weight %d.",k==0?"Court guard layers":k==1?"Light marsh weave":k==2?"Heavy stone-faced plate":"Sealed temple vestments",d->armor,d->speed,d->stealth,d->weight);
 }
 for(int e=0;e<ELITES;e++)for(int slot=0;slot<2;slot++){int n=96+e*2+slot;ItemDef*d=&itemDefs[CIV_ARMOR_ITEMS+n];const EliteDef*z=&eliteDefs[e];*d=itemDefs[gear_id(slot?z->armorFamily:z->weaponFamily,5,3)];
  d->name=slot?z->armor:z->weapon;d->desc=gearDesc[n];d->trim=0xe2b866;d->price+=1800+z->skill*15;
  if(!slot){d->power+=8+e%5;d->effect=e%6;d->potency=10+e%7;d->penetration+=e%3==0?16:5;d->accuracy+=e==12||e==16?14:3;if(e==12||e==6)d->cooldown*=.72f;if(e==5)d->range+=.7f;if(e==1||e==11||e==15){d->effect=3;d->potency=15;}}
  else{d->armor+=6+e%4;d->effect=e==5||e==13?6:e==1||e==11||e==16?9:e==10?10:4;d->potency=12+e%5;if(e==5||e==13){d->stealth+=18;d->weight=4;}if(e==9){d->armor+=12;d->weight+=6;d->speed-=5;}if(e==1||e==11||e==16)d->protection+=35;if(e==10)d->capacity+=18;}
  snprintf(gearDesc[n],112,"%s's %s. %s +%d; %s %d.",z->name,slot?"personal armor":"personal weapon",effectNames[d->effect],d->potency,slot?"armor":"damage",slot?d->armor:d->power);
 }
}
/* Culture remains tied to the settlement even after a faction occupation. */
int civil_style(int t){if(t<0||t>=TOWNS)return 0;int f=towns[t].faction;return f==SHOGUN?1:f==MONKS?2:f==SCAVENGERS?3:f==OUTLAWS?4:f==HOSTILE?5:0;}
const char*civil_style_name(int t){static const char*n[]={"TRADE BOROUGH","FORTRESS COURT","TEMPLE CLOISTER","SCRAP VILLAGE","FOUNDRY ENCLAVE","FLESH-CULT CAMP"};return n[civil_style(t)];}
static void clear_position(float*x,float*y){float ox=*x,oy=*y;for(int r=0;r<14;r++)for(int q=0;q<16;q++){float a=q*.392699f,xx=ox+r*cosf(a),yy=oy+r*sinf(a);if(can_walk(xx,yy)&&can_walk(xx+.4f,yy)&&can_walk(xx,yy+.4f)){*x=xx;*y=yy;return;}}}
static void building_position(int style,int type,int seq,float*x,float*y){
 static const float standard[13][2]={{-4,-3},{4,-3},{-4,3},{0,-5},{0,1},{-7,5},{8,5},{8,9},{6,9},{7,0},{-9,-7},{2,8},{-7,-7}};
 static const float court[13][2]={{-7,-4},{-6,7},{6,7},{0,-8},{0,2},{-8,4},{8,-1},{-8,-1},{9,8},{9,-7},{-11,-8},{4,2},{6,-5}};
 static const float temple[13][2]={{-7,-2},{-7,7},{7,7},{0,-8},{0,2},{-4,-5},{8,-1},{4,-5},{-9,8},{9,-7},{-11,-8},{0,-3},{7,-5}};
 static const float village[13][2]={{-5,-6},{4,1},{-3,6},{7,-6},{-1,-1},{-7,3},{7,6},{8,9},{-6,9},{-8,-8},{10,-9},{2,6},{5,-9}};
 static const float foundry[13][2]={{-8,-5},{-5,4},{-7,9},{5,-7},{-1,3},{-8,0},{3,-1},{7,1},{7,7},{9,-5},{-11,-7},{2,4},{-1,-6}};
 static const float cult[13][2]={{-5,-7},{7,-2},{6,5},{0,-8},{2,1},{-5,4},{7,-7},{-8,-3},{4,9},{9,5},{-10,-8},{-8,7},{-3,-3}};
 const float(*p)[2]=style==1?court:style==2?temple:style==3?village:style==4?foundry:style==5?cult:standard;
 *x=p[type][0];*y=p[type][1];if(type==HOUSE&&seq){if(style==1||style==2){*x=7;*y=-4+(seq-1)*5;}else{*x+=9-(seq-1)*3;*y+=6+seq;}}
 if (style == 3 && type == HOUSE && seq == 1) { *x = 10; *y = -1; }
 if(type==WALL){*x=-11+seq*2;*y=-10;}
}
static void settlement_layout(void){
 int original=g.buildingCount;
 for(int t=1;t<TOWNS;t++){
  int counters[13]={0},style=civil_style(t);
  for(int i=0;i<original;i++){Building*b=&g.buildings[i];if(b->owner>=0||distance(b->x,b->y,towns[t].x,towns[t].y)>15)continue;
   int landmark=0;for(int k=0;k<LANDMARKS;k++)if(g.depth.siteBuilding[k]==i)landmark=1;if(landmark)continue;
   float x,y;building_position(style,b->type,counters[b->type]++,&x,&y);b->x=towns[t].x+x;b->y=towns[t].y+y;
  }
  if(style==1){
   /* Open north/south gates and a broad axial approach to the keep. */
   for(int side=-1;side<=1;side+=2)for(int k=-5;k<=5;k++){add_build(towns[t].x+side*12,towns[t].y+k*2,WALL,g.townOwner[t],-1);if(abs(k)>=2)add_build(towns[t].x+k*2,towns[t].y+side*12,WALL,g.townOwner[t],-1);}
  }else if(style==2){for(int k=-4;k<=4;k++)add_build(towns[t].x+k*2,towns[t].y-12,WALL,g.townOwner[t],-1);}
  else if(style==5)add_build(towns[t].x-8,towns[t].y+7,CAMPFIRE,g.townOwner[t],-1);
 }
 /* Services stand beside their own building, not in a common overlapping pile. */
 for(int n=0;n<g.actorCount;n++){Actor*a=&g.actors[n];int t=a->homeTown;if(t<=0||t>=TOWNS||a->squad||a->dead||!conscious(a)||a->target>=0||depth_named(n)>=0||regional_named(n)||living_owned(n)||a->prisonTime>0||g.depth.actors[n].captive||a->carrying>=0)continue;
  int employed=0;for(int j=0;j<CONTRACTS;j++)if(g.contracts[j].active==1&&g.society.jobActor[j]==n+1)employed=1;if(employed)continue;
  int role=npc_role(a),type=role==ROLE_MERCHANT?SHOP:role==ROLE_INNKEEPER?BAR:role==ROLE_MEDIC?BED:role==ROLE_SMITH?FORGE:-1;
  float x=towns[t].x+(a->kind==GUARD?3:-4),y=towns[t].y+(a->kind==GUARD?9:4);
  if(type>=0)for(int b=0;b<g.buildingCount;b++)if(g.buildings[b].owner<0&&g.buildings[b].type==type&&distance(g.buildings[b].x,g.buildings[b].y,towns[t].x,towns[t].y)<15){x=g.buildings[b].x+1.8f;y=g.buildings[b].y+.8f;break;}
  clear_position(&x,&y);a->x=a->homeX=a->tx=x;a->y=a->homeY=a->ty=y;a->routeLen=a->routePos=0;
 }
 for(int i=0;i<g.nodeCount;i++){Node*n=&g.nodes[i];if(n->owner&&town_near(n->x,n->y,16)>=0&&!can_walk(n->x,n->y))clear_position(&n->x,&n->y);}
 for(int i=0;i<g.actorCount;i++)if(g.actors[i].squad&&!actor_inside(&g.actors[i])&&!can_walk(g.actors[i].x,g.actors[i].y))clear_position(&g.actors[i].x,&g.actors[i].y);
}
int civil_building_sprite(int n,int fallback){
 Building*b=&g.buildings[n];if(b->owner>=0)return fallback;int t=town_near(b->x,b->y,19);if(t<0)return fallback;int style=civil_style(t);
 if(t==OLD_TOWNS && b->type==HOUSE)return 7;
 if(t==OLD_TOWNS+1 && b->type==STORE)return 4;
 if(t==OLD_TOWNS+2){
  if(b->type==HOUSE || b->type==TOWER)return 0;
  if(b->type==SHOP)return 7;
  if(b->type==BAR)return 2;
 }
 if(b->type==HOUSE)return style==1?4:style==2?7:style==3?(n%2?5:0):style==4?0:style==5?5:n%3==0?7:0;
 if(b->type==TOWER&&style==3)return 5;if(b->type==TOWER&&style==2)return 4;
 if(b->type==BAR)return style==3||style==4?7:style==5?5:style==2?4:fallback;
 if(b->type==SHOP&&style==5)return 5;
 return fallback;
}

/* The capitals share services, but their streets and civic buildings are authored separately. */
typedef struct { int type, x, y; } CivicBuilding;
static const CivicBuilding capitals[WAR_FACTIONS][17] = {
 {{SHOP,-4,-2},{BAR,4,-2},{WELL,0,1},{FORGE,5,4},{BENCH,-4,4},{STORE,7,7},
  {WINDMILL,-7,6},{HOUSE,-8,1},{HOUSE,8,-1},{BED,4,0},{TOWER,-7,-5},
  {WALL,-9,-7},{WALL,-7,-7},{WALL,7,-7},{WALL,9,-7},{-1}},
 {{STORE,0,-7},{TOWER,-6,-5},{TOWER,6,-5},{SHOP,-5,1},{BAR,5,1},{FORGE,-6,6},
  {WELL,0,-2},{BED,5,3},{HOUSE,7,-1},{BENCH,7,6},
  {WALL,-7,8},{WALL,-5,8},{WALL,-3,8},{WALL,3,8},{WALL,5,8},{WALL,7,8},{-1}},
 {{BAR,0,-6},{BED,2,-3},{BED,-2,-3},{SHOP,5,0},{HOUSE,-5,0},{HOUSE,5,5},
  {WELL,0,0},{FARM,-4,4},{FARM,-2,6},{CAMPFIRE,0,6},{BENCH,-5,-4},
  {WALL,-7,-5},{WALL,-7,-3},{WALL,7,-5},{WALL,7,-3},{-1}}
};
static const char *capitalCustoms[WAR_FACTIONS] = {
 "BRASS JUNCTION: the Compact elects wardens at the well. Rifle crews guard the rail yard. Captured troops work the depot under guard.",
 "VERMILION KEEP: the Iron houses bring rice and steel to the court. Wear lamellar and a Shogunate helm at the gate. Captives work the ironworks.",
 "CINDER ABBEY: the healers keep a public hospice and herb beds. Pilgrims share the fire. Prisoners work the cloister as penance."
};
const char *war_capital_custom(int town) {
 return town>=OLD_TOWNS && town<TOWNS ? capitalCustoms[town-OLD_TOWNS] : NULL;
}
void war_yard(int town, float *x, float *y) {
 static const int dx[WAR_FACTIONS]={8,-8,-6}, dy[WAR_FACTIONS]={4,3,7};
 int index=town-OLD_TOWNS;
 *x=towns[town].x+(index>=0?dx[index]:-9);
 *y=towns[town].y+(index>=0?dy[index]:6);
}
static int civic_build_clear(float x,float y) {
 if(!can_walk(x,y))return 0;
 for(int n=0;n<g.buildingCount;n++)
  if(distance(x,y,g.buildings[n].x,g.buildings[n].y)<1.7f)return 0;
 for(int n=0;n<g.actorCount;n++)
  if(g.actors[n].active && g.actors[n].squad && distance(x,y,g.actors[n].x,g.actors[n].y)<1.8f)return 0;
 return 1;
}
void war_build_capitals(void) {
 for(int k=0;k<WAR_FACTIONS;k++){
  int town=OLD_TOWNS+k, faction=towns[town].faction;
  g.townOwner[town]=faction;
  g.prosperity[town]=78;
  restock_town(town);
  for(const CivicBuilding *b=capitals[k];b->type>=0;b++){
   float x=towns[town].x+b->x,y=towns[town].y+b->y;
   if(b->type!=WALL && !civic_build_clear(x,y)){
    static const int shift[8][2]={{2,0},{-2,0},{0,2},{0,-2},{3,3},{-3,3},{3,-3},{-3,-3}};
    for(int j=0;j<8;j++)if(civic_build_clear(x+shift[j][0],y+shift[j][1])){x+=shift[j][0];y+=shift[j][1];break;}
   }
   if(civic_build_clear(x,y))add_build(x,y,b->type,faction,-1);
  }
 }
}
void war_outfit(Actor *a, int role, int variation, int quality) {
 int side=war_faction_index(a->faction);
 if(side<0)return;
 static const int arms[WAR_FACTIONS][4]={{29,8,84,29},{5,0,5,2},{16,25,16,5}};
 static const int body[WAR_FACTIONS]={42,34,39}, head[WAR_FACTIONS]={53,50,57};
 static const int material[WAR_FACTIONS]={2,4,1};
 int m=material[side], guard=role==ROLE_MARSHAL;
 a->gear[0]=guard?gear_id(arms[side][variation%4],m,quality):NONE;
 a->gear[1]=gear_id(body[side],m,quality);
 a->gear[2]=gear_id(head[side],m,quality);
 a->gear[3]=gear_id(side==1?63:side==2?61:60,m,quality);
 a->gear[4]=gear_id(side==2?73:69,m,quality);
 a->gear[5]=role==ROLE_MEDIC?gear_id(78,m,1):NONE;
 if(weapon_ranged(a->gear[0])){
  int ammo=gear_family(a->gear[0])>=29?CARTRIDGE:BOLTS;
  if(!inventory_count(a,ammo))inventory_add(a,ammo,24);
 }
}
void war_add_residents(void) {
 static const char *names[WAR_FACTIONS][WAR_RESIDENTS]={
  {"Warden Flint","Rail rifleman","Broker Keel","Medic Sable","Inn keeper"},
  {"Lord Hideo","Iron spearman","Factor Aya","Medic Ren","Inn keeper"},
  {"Prior Sol","Ash bowkeeper","Herbalist Fen","Sister Edda","Inn keeper"}
 };
 static const int kind[WAR_RESIDENTS]={GUARD,GUARD,TRADER,SETTLER,SETTLER};
 static const int role[WAR_RESIDENTS]={ROLE_MARSHAL,ROLE_MARSHAL,ROLE_MERCHANT,ROLE_MEDIC,ROLE_INNKEEPER};
 static const int station[WAR_RESIDENTS]={TOWER,FORGE,SHOP,BED,BAR};
 for(int k=0;k<WAR_FACTIONS;k++){
  int town=OLD_TOWNS+k;
  for(int j=0;j<WAR_RESIDENTS;j++){
   if(g.war.resident[k][j])continue;
   int n=-1;
   for(int i=0;i<g.actorCount;i++)if(g.living.owned[i]<0){n=i;break;}
   if(n<0){if(g.actorCount>=MAX_ACTORS)return;n=g.actorCount++;}
   float x=towns[town].x+2,y=towns[town].y+1;
   for(int i=0;i<g.buildingCount;i++){
    Building *b=&g.buildings[i];
    if(b->owner<0 && b->type==station[j] && distance(b->x,b->y,x,y)<16){x=b->x+1.65f;y=b->y+.4f;break;}
   }
   clear_position(&x,&y);
   init_actor(n,names[k][j],kind[j],g.townOwner[town],x,y,j==0?42:j==1?26:7);
   Actor *a=&g.actors[n];
   a->homeTown=town;
   a->voice=role[j];
   a->race=k==2?1:k==1&&j==1?4:0;
   for(int l=0;l<LIMBS;l++)a->health[l]=species_health(a);
   war_outfit(a,role[j],j,j==0?3:1);
   if(role[j]==ROLE_MEDIC)inventory_add(a,MEDKIT,3);
   g.living.owned[n]=0;
   g.living.anger[n]=0;
   g.living.command[n]=-1;
   g.war.resident[k][j]=n+1;
  }
 }
}
void civil_stock(int t){for(int k=0;k<4;k++){int preferred=t==15?1:civil_style(t)==1?0:civil_style(t)==2?3:civil_style(t)==3?1:2;int v=(t*3+g.day+k*5)%24;g.stock[t][CIV_ARMOR_ITEMS+preferred*24+v]=1;}}
int civil_elite_index(int n){if(n<0)return -1;for(int e=0;e<ELITES;e++)if(C.eliteActor[e]==n+1)return e;return -1;}
static int candidate(int e){
 const EliteDef*z=&eliteDefs[e];int best=-1;
 for(int n=0;n<g.actorCount;n++){Actor*a=&g.actors[n];if(a->squad||a->dead||!conscious(a)||a->carriedBy>=0||a->carrying>=0||a->prisonTime>0||g.depth.actors[n].captive||depth_named(n)>=0||regional_named(n)||living_owned(n)||civil_elite_index(n)>=0||g.society.freed[n])continue;
  int employed=0;for(int j=0;j<CONTRACTS;j++)if(g.contracts[j].active==1&&g.society.jobActor[j]==n+1)employed=1;if(employed)continue;
  if(z->town>=0){if(a->homeTown==z->town&&a->kind==GUARD&&a->voice!=ROLE_MERCENARY)return n;}
  else if(a->kind==RAIDER)best=n;
 }return best;
}
void civil_init(void){
 if(C.version)return;memset(&C,0,sizeof(C));C.version=1;settlement_layout();
 for(int n=0;n<g.actorCount;n++){Actor*a=&g.actors[n];if(a->squad||a->dead||a->kind==BEAST||a->race!=0||depth_named(n)>=0||regional_named(n))continue;
  int t=a->homeTown,style=civil_style(t),r=style==1?(n%3==0?4:0):style==2?(t==15?3:1):style==3?(n%2?3:0):style==4?(n%2?2:4):style==5?(n%2?4:1):n%5;
  a->race=r;for(int l=0;l<LIMBS;l++)a->health[l]*=species_health(a)/100;
 }
 for(int e=0;e<ELITES;e++){
  int n=candidate(e);if(n<0)continue;C.eliteActor[e]=n+1;const EliteDef*z=&eliteDefs[e];Actor*a=&g.actors[n];
  snprintf(a->name,sizeof(a->name),"%s",z->name);a->kind=GUARD;a->voice=ROLE_CITIZEN;a->faction=z->town>=0?g.townOwner[z->town]:z->faction;a->race=z->race;
  for(int s=0;s<SKILLS;s++){int lv=s==MELEE||s==DEFENCE||s==TOUGHNESS||s==STRENGTH||s==DEXTERITY||s==CROSSBOWS||s==DODGE?z->skill:s==ATHLETICS?22:z->skill/3;a->xp[s]=12*(lv-1)*(lv-1);}
  for(int l=0;l<LIMBS;l++)a->health[l]=species_health(a);
  for(int k=0;k<2;k++)if(a->gear[k]>=UNIQUE_ITEMS&&a->gear[k]<V090_ITEMS)inventory_add(a,a->gear[k],1);
  a->gear[0]=REGALIA_ITEMS+e*2;a->gear[1]=REGALIA_ITEMS+e*2+1;a->gear[2]=gear_id(z->town==7||z->town==12?57:z->town==14?55:z->town==11||z->town==6?50:49,4,2);
  if(weapon_ranged(a->gear[0]))inventory_add(a,z->weaponFamily>=29?CARTRIDGE:BOLTS,80);
  if(z->town>=0){a->homeTown=z->town;a->x=towns[z->town].x+2.6f;a->y=towns[z->town].y-5;}
  else{int t=5+(e-12),u=(t+4)%TOWNS;C.routeTown[e]=u;a->homeTown=t;a->x=towns[t].x*.6f+towns[u].x*.4f;a->y=towns[t].y*.6f+towns[u].y*.4f;a->bountyReward=z->faction==OUTLAWS?1500+e*100:0;}
  clear_position(&a->x,&a->y);a->homeX=a->tx=a->x;a->homeY=a->ty=a->y;a->target=-1;a->routeLen=a->routePos=0;
 }
 for(int t=0;t<TOWNS;t++)civil_stock(t);
}
int civil_npc_job(int n,float dt){
 int e=civil_elite_index(n);if(e<12||C.fallen[e])return 0;Actor*a=&g.actors[n];if(a->squad||g.depth.actors[n].captive||a->prisonTime>0||!conscious(a))return 0;
 int t=C.routeTown[e];if(distance(a->x,a->y,towns[t].x,towns[t].y)<22)C.routeTown[e]=(t+3+(e%3))%TOWNS;
 t=C.routeTown[e];route_toward(a,towns[t].x,towns[t].y,dt*.65f);return 1;
}
void civil_tick(float dt){
 C.pulse+=dt;if(C.pulse<1)return;float step=fminf(2,C.pulse);C.pulse=0;
 for(int e=0;e<ELITES;e++){int n=C.eliteActor[e]-1;if(n<0)continue;Actor*a=&g.actors[n];if(a->dead&&!C.fallen[e]){C.fallen[e]=1;if(eliteDefs[e].town>=0){int t=eliteDefs[e].town;g.prosperity[t]=(int)fmaxf(10,g.prosperity[t]-12);if(distance(a->x,a->y,g.actors[g.selected].x,g.actors[g.selected].y)<45)log_warn("%s has fallen. %s faces a leadership vacuum.",a->name,towns[t].name);}continue;}
  if(e>=12&&!a->dead&&distance(a->x,a->y,g.actors[g.selected].x,g.actors[g.selected].y)>45)civil_npc_job(n,step);
 }
}
int civil_validate(const Game*s){
 if(!s->civil.version)return 1;if(s->civil.version!=1||!isfinite(s->civil.pulse)||s->civil.pulse<0)return 0;
 for(int e=0;e<ELITES;e++){int n=s->civil.eliteActor[e];if(n<0||n>s->actorCount||s->civil.routeTown[e]<0||s->civil.routeTown[e]>=TOWNS||s->civil.fallen[e]<0||s->civil.fallen[e]>1)return 0;for(int k=0;k<e;k++)if(n&&s->civil.eliteActor[k]==n)return 0;}
 return 1;
}
#undef C
