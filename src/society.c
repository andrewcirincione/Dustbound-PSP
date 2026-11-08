#include "game.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
extern int conscious(const Actor*);
extern int add_actor(const char*,int,int,float,float,int);
extern int hostile(const Actor*,const Actor*);
#define S (g.society)
static float bound(float v,float a,float b){return fminf(b,fmaxf(a,v));}
static int actor_no(const Actor*a){return (uintptr_t)a>=(uintptr_t)g.actors&&(uintptr_t)a<(uintptr_t)(g.actors+g.actorCount)?(int)(a-g.actors):-1;}
const char *weaponClassNames[WEAPON_CLASSES]={"Katanas","Sabres","Hackers","Heavy blades","Blunt","Polearms","Rapiers","Knives","Bows","Crossbows","Firearms"};
int weapon_class(int it){
 if(!it)return -1;if(weapon_rapier(it))return WC_RAPIERS;
 int f=gear_family(it);if(it>=SCIMITAR_ITEMS&&it<SCIMITAR_ITEMS+24)return WC_SABRES;
 if(it==RUSTSWORD||it==SABRE)return WC_SABRES;if(it==CLEAVER)return WC_HACKERS;if(it==STAFF)return WC_POLEARMS;if(it==CROSSBOW)return WC_CROSSBOWS;
 if(f==0||f==1||f==2)return WC_KATANAS;if(f==3||f==9)return WC_KNIVES;
 if(f==4||f==5||f==6||f==16||f==86)return WC_POLEARMS;
 if(f==11||f==12||f==13||f==87)return WC_HACKERS;if(f==20||f==84)return WC_HEAVY;
 if(f==14||f==15||f==23||f==85)return WC_BLUNT;if(f==24||f==28)return WC_BOWS;
 if(f>=25&&f<=27)return WC_CROSSBOWS;if(f>=29&&f<=31)return WC_FIREARMS;
 return f>=0&&f<24?WC_SABRES:-1;
}
int weapon_class_level(const Actor*a,int c){int n=actor_no(a);if(c<0||c>=WEAPON_CLASSES||n<0)return 1;if(!a->squad)return level(a,c>=WC_BOWS?CROSSBOWS:MELEE);return (int)bound(1+sqrtf(S.weaponXP[n][c]/12),1,100);}
void weapon_class_gain(Actor*a,int w,float amount){int n=actor_no(a),c=weapon_class(w);if(n<0||c<0||!a->squad)return;int before=weapon_class_level(a,c);S.weaponXP[n][c]=bound(S.weaponXP[n][c]+amount,0,117612);if(n==g.selected&&weapon_class_level(a,c)>before)log_msg("%s: %s %d",a->name,weaponClassNames[c],weapon_class_level(a,c));}
float weapon_cut_ratio(int it){int c=weapon_class(it);if(!it)return 1;if(c==WC_BLUNT)return .05f;if(it==STAFF||gear_family(it)==16)return .05f;return c==WC_HEAVY?.45f:c==WC_HACKERS?.55f:c==WC_KATANAS?.9f:c==WC_SABRES?.8f:c==WC_RAPIERS||c==WC_KNIVES?.95f:.75f;}
float combat_learning(float learner,float opponent){float d=opponent-learner;return bound(1+d*(d>=0?.1f:.04f),.1f,6);}
int combat_evasion(const Actor*a){return !a->gear[0]||weapon_ranged(a->gear[0])||a->stun>0;}
float dodge_mobility(const Actor*a){float capacity=actor_capacity(a);return bound(1-fmaxf(0,encumbrance(a)-capacity)/fmaxf(1,capacity),.05f,1);}
static const int exports[TOWNS]={GRAIN,COPPER,WATER,HIDE,MEAT,ORE,IRONBAR,CLOTH,PARTS,WOOD,COAL,IRONBAR,MEDKIT,GRAIN,MEAT,WOOD};
static const int imports[TOWNS]={IRONBAR,WOOD,GRAIN,MEDKIT,CLOTH,WATER,COAL,MEAT,MEDKIT,IRONBAR,WOOD,PARTS,WATER,ORE,MEDKIT,PARTS};
static int commodity(int it){return it==RATION||it==MEDKIT||it==ORE||it==COPPER||it==WOOD||it==CLOTH||it==WATER||it==GRAIN||it==MEAT||it==IRONBAR||it==HIDE||it==COAL||it==PARTS||it==TONIC||it==RELIC;}
float market_factor(int town,int item){
 if(!commodity(item))return 1;
 float v=item==exports[town]?.59f:item==imports[town]?1.72f:1.f;
 if(item==RATION&&exports[town]==GRAIN)v=.68f;
 if(item==RATION&&imports[town]==GRAIN)v=1.55f;
 int stock=g.stock[town][item];v+=stock<4?.25f:stock<10?.1f:stock>45?-.12f:0;
 if(S.crisis[town]==1&&(item==RATION||item==GRAIN||item==WATER))v+=.4f;
 if(S.crisis[town]==2&&(item==MEDKIT||item==IRONBAR||item==PARTS))v+=.35f;
 return bound(v,.48f,2.25f);
}
int market_percent(int t,int it,int selling){float normal=itemDefs[it].price*(selling?.75f:1);return normal>0?(int)roundf((item_price(t,it,selling)/normal-1)*100):0;}
const char *trade_tip(int town){
 static char line[240];int bestItem=0,bestTown=-1,bestProfit=0;
 if(town<0)return "Ask a town merchant for current prices and transport work.";
 for(int it=1;it<BASE_ITEMS;it++)if(commodity(it)&&g.stock[town][it]>0)for(int t=0;t<TOWNS;t++)if(t!=town){int profit=item_price(t,it,1)-item_price(town,it,0);if(profit>bestProfit){bestProfit=profit;bestItem=it;bestTown=t;}}
 if(bestTown>=0)snprintf(line,sizeof(line),"%s: buy here for %d, sell at %s for %d. About %d silver per unit before travel costs. Supply and unrest change prices.",itemDefs[bestItem].name,item_price(town,bestItem,0),towns[bestTown].name,item_price(bestTown,bestItem,1),bestProfit);
 else snprintf(line,sizeof(line),"No profitable export quote today. We produce %s and need %s. Ask again after trade or shortages change stock.",itemDefs[exports[town]].name,itemDefs[imports[town]].name);
 return line;
}
void society_stock(int t){
 g.stock[t][exports[t]]=40;g.stock[t][imports[t]]=6;
 for(int k=0;k<3;k++){int v=(t*3+g.day+k*7)%24;g.stock[t][SCIMITAR_ITEMS+v]=1;g.stock[t][RAPIER_ITEMS+(v+5)%24]=1;}
 if(t==8){g.stock[t][gear_id(37,0,1)]=2;g.stock[t][gear_id(55,0,1)]=2;}
 if(t==12){g.stock[t][gear_id(39,0,1)]=2;g.stock[t][gear_id(57,0,1)]=2;}
 if(t==6){g.stock[t][gear_id(34,1,1)]=2;g.stock[t][gear_id(50,1,1)]=2;}
}
int wanted_amount(const Actor*a,int f){int n=actor_no(a);if(n<0||f<0||f>=FACTIONS)return 0;int sum=0;for(int j=0;j<FACTIONS;j++)sum+=S.wanted[n][j];return sum?S.wanted[n][f]:f==UNION?a->bounty:0;}
static void wanted_sum(Actor*a){int n=actor_no(a);a->bounty=0;for(int f=0;f<FACTIONS;f++)a->bounty+=S.wanted[n][f];}
void crime_add(Actor*a,int f,int amount){int n=actor_no(a);if(n<0||f<=FREE||f>=FACTIONS)return;if(a->bounty>0){int sum=0;for(int j=0;j<FACTIONS;j++)sum+=S.wanted[n][j];if(!sum)S.wanted[n][UNION]=a->bounty;}S.wanted[n][f]=(int)bound(S.wanted[n][f]+amount,0,100000);S.recognized[n][f]=600;wanted_sum(a);}
void crime_clear(Actor*a,int f){int n=actor_no(a);if(n<0||f<0||f>=FACTIONS)return;S.wanted[n][f]=0;S.recognized[n][f]=0;wanted_sum(a);}
int disguise_faction(const Actor*a){
 int n=actor_no(a),bodyItem=a->gear[1];if(!bodyItem&&n>=0)bodyItem=g.depth.actors[n].layer;
 int body=gear_family(bodyItem),head=gear_family(a->gear[2]);
 if(body==39&&head==57)return MONKS;
 if((body==34||body==47)&&(head==50||head==58||head==59))return SHOGUN;
 if(body==37&&head==55)return HOSTILE;
 if(body==36&&head==51)return OUTLAWS;
 if(body==42&&head==53)return UNION;
 if(body==32&&head==49)return SCAVENGERS;
 return FREE;
}
int disguise_score(const Actor*a,int f){if(disguise_faction(a)!=f)return 0;int v=55+itemDefs[a->gear[1]].quality*5+level(a,STEALTH)/2-(a->sneaking?18:0)-(a->autoattack?12:0);if(f==MONKS&&a->prostheticMask)v=0;return (int)bound(v,0,95);}
const char *town_custom(int t){
 if(t==7)return "ASH MONASTERY: only robed initiates enter. Robe and cowl required; exposed prosthetics are heresy. Lingering near inquisitors risks recognition.";
 if(t==11)return "SHOGUN'S REST: closed garrison. Lamellar or captain armor plus a shogunate helm opens the gate. Officers inspect strangers at close range.";
 if(t==14)return "VULTURE NEST: a flesh cult. Bone harness and oni mask blend in. Outsiders are beaten, carried to the cooking ground, bound and eaten. Rescue is possible.";
 if(t==8)return "BLACK MESA: smugglers sell forbidden uniforms and machine parts. Ordinary stolen goods find buyers here, but local theft still has consequences.";
 if(t==12)return "PALE PASS: pilgrim sanctuary; medicine is produced here, water is scarce. Pilgrim clothing is sold openly.";
 return "Local production, shortages and faction control change trade. Ask merchants for a route and marshals for work.";
}
static int restricted(int t){return (t==7||t==11||t==14)&&g.townOwner[t]==towns[t].faction;}
static int guard_exists(int t){for(int n=0;n<g.actorCount;n++){Actor*b=&g.actors[n];if(!b->squad&&b->kind==GUARD&&b->homeTown==t&&!b->dead&&b->unconscious<=0)return 1;}return 0;}
int society_gate(Actor*a,float x,float y){
 int n=actor_no(a);if(n<0||!a->squad)return 1;
 for(int t=0;t<TOWNS;t++)if(restricted(t)&&distance(x,y,towns[t].x,towns[t].y)<18&&distance(a->x,a->y,towns[t].x,towns[t].y)>=18&&guard_exists(t)){
  int f=g.townOwner[t];if(disguise_score(a,f)>=40&&S.recognized[n][f]<=0)return 1;
  if(S.gateMessage[n]<=0){log_msg("%s: matching body armor and headwear required.",towns[t].name);S.gateMessage[n]=5;}return 0;
 }return 1;
}
/* -1 means defer to the normal faction relationship. */
int society_hostile(const Actor*a,const Actor*b){
 int ai=actor_no(a),bi=actor_no(b),pa=a->squad||(ai>=0&&S.freed[ai]),pb=b->squad||(bi>=0&&S.freed[bi]);
 const Actor*p=pa?a:pb?b:NULL,*w=pa?b:a;if(!p||w->squad||(pa&&pb))return -1;
 int n=actor_no(p),f=w->faction,t=w->homeTown;if(n<0||f==FREE||f==WILDLIFE)return -1;
 if(disguise_score(p,f)>=40&&S.recognized[n][f]<=0&&(actor_no(w)<0||!g.living.anger[actor_no(w)]))return 0;
 if(w->kind==GUARD&&wanted_amount(p,f)>=80)return 1;
 if(t>=0&&restricted(t)&&distance(p->x,p->y,towns[t].x,towns[t].y)<18&&disguise_score(p,f)<40)return 1;
 return -1;
}
void society_actor_reset(int n){
 if(n<0||n>=MAX_ACTORS)return;
 memset(S.weaponXP[n],0,sizeof(S.weaponXP[n]));memset(S.wanted[n],0,sizeof(S.wanted[n]));memset(S.recognized[n],0,sizeof(S.recognized[n]));
 S.queueCount[n]=S.cannibal[n]=S.rescueHelper[n]=S.freed[n]=S.captorFaction[n]=0;
 S.eating[n]=S.suspicion[n]=S.gateMessage[n]=0;
 for(int i=0;i<MAX_ACTORS;i++)if(S.rescueHelper[i]==n+1){S.rescueHelper[i]=0;g.depth.actors[i].escape=0;}
}
int society_rescue_busy(int helper){for(int n=0;n<g.actorCount;n++)if(S.rescueHelper[n]==helper+1)return 1;return 0;}
void society_rescue_cancel(int helper){for(int n=0;n<g.actorCount;n++)if(S.rescueHelper[n]==helper+1){S.rescueHelper[n]=0;g.depth.actors[n].escape=0;}}
static int rescue_faction(int n){
 Actor*a=&g.actors[n];if(a->prisonTime>0&&a->prisonTown>=0)return g.townOwner[a->prisonTown];
 if(S.captorFaction[n])return S.captorFaction[n];return g.depth.actors[n].captive==3?HOSTILE:OUTLAWS;
}
static int rescue_guard(int n,float reach){
 Actor*a=&g.actors[n];int faction=rescue_faction(n);
 for(int j=0;j<g.actorCount;j++){Actor*b=&g.actors[j];if(j!=n&&!b->squad&&conscious(b)&&(b->kind==GUARD||b->kind==RAIDER)&&
  (b->faction==faction||(faction==OUTLAWS&&b->faction==HOSTILE))&&depth_distance(a,b)<reach)return j;}
 return -1;
}
int society_rescue_start(int n){
 if(n<0||n>=g.actorCount)return 0;Actor*a=&g.actors[g.selected],*b=&g.actors[n];DepthActor*z=&g.depth.actors[n];
 if(!conscious(a)||!b->active||b->dead||b->carriedBy>=0||depth_room(a)!=depth_room(b)||depth_distance(a,b)>TALK_REACH||(!z->captive&&b->prisonTime<=0))return 0;
 if(z->captive==2||S.cannibal[n]==1){log_msg("Stop the captor and set the prisoner down first.");return 0;}
 if(S.rescueHelper[n]&&S.rescueHelper[n]!=g.selected+1){log_msg("An ally is already working those restraints.");return 0;}
 society_rescue_cancel(g.selected);S.captorFaction[n]=rescue_faction(n);S.rescueHelper[n]=g.selected+1;z->escape=1;a->taskKind=0;
 log_msg("Picking %s's restraints. Stay close; guards can catch you.",b->name);depth_open(SCREEN_WORLD);return 1;
}
static void rescue_refuge(Actor*a,int former){
 int town=-1;float best=1e9f;
 for(int t=0;t<TOWNS;t++)if(g.townOwner[t]!=former&&g.townOwner[t]!=HOSTILE&&g.townOwner[t]!=OUTLAWS){float d=distance(a->x,a->y,towns[t].x,towns[t].y);if(d<best){town=t;best=d;}}
 if(town<0)town=0;a->homeTown=town;a->homeX=towns[town].x-9;a->homeY=towns[town].y+6;
 for(int r=0;r<12&&!can_walk(a->homeX,a->homeY);r++){a->homeX=towns[town].x-8+(r%4);a->homeY=towns[town].y+7+r/4;}
 a->tx=a->homeX;a->ty=a->homeY;a->routeLen=a->routePos=0;
}
static void rescue_finish(int n,int helper){
 Actor*a=&g.actors[n];DepthActor*z=&g.depth.actors[n];int faction=rescue_faction(n);
 z->escape=0;z->captive=0;z->recognized=1800;S.cannibal[n]=0;S.eating[n]=0;S.rescueHelper[n]=0;
 a->prisonTime=0;a->prisonTown=-1;a->target=-1;a->attackPhase=0;g.living.command[n]=-1;
 S.captorFaction[n]=faction;crime_add(a,faction,80);S.recognized[n][faction]=1800;g.depth.freed++;
 if(z->confiscated&&!a->gear[0]){a->gear[0]=z->confiscated;a->gearStolen[0]=z->confiscatedOwner;z->confiscated=z->confiscatedOwner=0;}
 if(!a->squad){S.freed[n]=(g.seed+(unsigned)n)%3==0?1:2;a->kind=SETTLER;a->faction=FREE;a->autoattack=0;rescue_refuge(a,faction);}
 log_warn("%s is free.%s",a->name,a->squad?"":S.freed[n]==1?" They choose to follow. Recruit or dismiss through nearby actions.":" They are running for refuge.");
 (void)helper;
}
void society_rescue_order(int n,int order){
 if(n<0||n>=g.actorCount)return;Actor*a=&g.actors[n],*p=&g.actors[g.selected];
 if(!S.freed[n]||a->squad||a->dead||!conscious(p)||g.depth.actors[n].captive||a->prisonTime>0||depth_distance(a,p)>TALK_REACH)return;
 if(order==ACTION_RESCUED_RECRUIT){if(g.squadCount>=SQUAD){log_msg("Squad full. They remain free and can follow or seek refuge.");return;}
  a->squad=1;a->kind=PLAYER;a->faction=FREE;a->job=JOB_FOLLOW;a->autoattack=1;g.squadCount++;g.depth.actors[n].group=g.depth.actors[g.selected].group;S.freed[n]=0;log_msg("%s joins your expedition.",a->name);
 }else if(order==ACTION_RESCUED_LEAVE){S.freed[n]=2;rescue_refuge(a,S.captorFaction[n]);log_msg("%s: Fine. I will make my own way to %s.",a->name,towns[a->homeTown].name);}
 else{S.freed[n]=1;log_msg("%s: I will stay close. Lead the way.",a->name);}
}
static void tick_rescues(float dt){
 for(int n=0;n<g.actorCount;n++)if(S.rescueHelper[n]){
  int h=S.rescueHelper[n]-1;Actor*a=&g.actors[n],*helper=&g.actors[h];DepthActor*z=&g.depth.actors[n];int guard=rescue_guard(n,4);
  if(a->dead||!a->active||!conscious(helper)||a->carriedBy>=0||depth_room(helper)!=depth_room(a)||depth_distance(helper,a)>TALK_REACH||(!z->captive&&a->prisonTime<=0)||guard>=0){
   S.rescueHelper[n]=0;z->escape=0;
   if(guard>=0){crime_add(helper,g.actors[guard].faction,80);g.actors[guard].target=h;log_warn("Caught freeing a prisoner! %s alerts the guards.",g.actors[guard].name);}
   else log_msg("Rescue interrupted. Return to the restraints.");continue;
  }
  z->escape+=dt*(inventory_count(helper,TOOLS)?2:1)*(1+level(helper,LOCKPICK)*.02f);gain(helper,LOCKPICK,dt*2);
  if(z->escape>=18)rescue_finish(n,h);
 }
 for(int n=0;n<g.actorCount;n++){Actor*a=&g.actors[n];if(S.freed[n]&&conscious(a)&&!a->squad&&distance(a->x,a->y,g.actors[g.selected].x,g.actors[g.selected].y)>45)society_npc_job(n,dt);}
}
static void seed_prisoners(void){
 if(S.prisonSeeded)return;S.prisonSeeded=1;
 for(int j=0;j<2;j++){int t=j?6:0;float x=towns[t].x-9,y=towns[t].y+5;
  for(int k=0;k<12&&!can_walk(x,y);k++){x=towns[t].x-9+k%4;y=towns[t].y+6+k/4;}
  if(!can_walk(x,y))continue;int n=add_actor(j?"Kest":"Ilan",SETTLER,FREE,x,y,4+j*3);if(n<0)continue;
  Actor*a=&g.actors[n];a->prisonTown=t;a->prisonTime=7200;a->gear[1]=RAGS;a->gear[2]=0;g.depth.actors[n].confiscated=a->gear[0];a->gear[0]=0;
  S.captorFaction[n]=g.townOwner[t];crime_add(a,g.townOwner[t],120);
 }
}

void society_init(void){
 if(S.version){seed_prisoners();return;}memset(&S,0,sizeof(S));S.version=1;S.lastEconomyDay=g.day;
 for(int n=0;n<g.actorCount;n++)if(g.actors[n].squad&&g.actors[n].bounty>0)S.wanted[n][UNION]=g.actors[n].bounty;
 for(int t=0;t<TOWNS;t++){society_stock(t);int armed=0;for(int n=0;n<g.actorCount;n++){
  Actor*a=&g.actors[n];if(a->squad||a->homeTown!=t)continue;
  if(a->kind==GUARD&&t>=5&&!armed++){a->gear[0]=UNIQUE_ITEMS+t%8;a->gear[1]=UNIQUE_ITEMS+8+t%6;}
  if(a->kind==GUARD&&t==10&&armed==1)a->gear[5]=UNIQUE_ITEMS+14;
  if(a->kind==GUARD&&t==12&&armed==1)a->gear[2]=UNIQUE_ITEMS+15;
  if(a->kind==GUARD&&(t==7||t==11||t==14)){a->gear[1]=gear_id(t==7?39:t==11?34:37,t==11?1:0,2);a->gear[2]=gear_id(t==7?57:t==11?50:55,t==11?1:0,2);snprintf(a->name,sizeof(a->name),"%s",t==7?"Ash inquisitor":t==11?"Gate inspector":"Feast butcher");}
 }}
 seed_prisoners();
}
int bounty_contract_active(int actor){for(int i=0;i<CONTRACTS;i++)if(g.contracts[i].active==1&&g.contracts[i].type==7&&S.jobActor[i]==actor+1)return 1;return 0;}
int society_contract(int town,int kind){
 if(town<0||town>=TOWNS||kind<4||kind>7)return 0;
 int slot=-1,live=0;for(int i=0;i<CONTRACTS;i++){if(g.contracts[i].active==1){if(g.contracts[i].town==town&&g.contracts[i].type==kind){log_msg("That job is already active.");return 0;}if(g.contracts[i].type==5||g.contracts[i].type==6)live++;}else if(slot<0)slot=i;}
 if(slot<0){log_msg("Finish an active contract first.");return 0;}if((kind==5||kind==6)&&live>=4){log_msg("Four service jobs already active.");return 0;}
 int actor=-1;Actor*p=&g.actors[g.selected];
 if(kind>=5){for(int n=0;n<g.actorCount;n++){Actor*a=&g.actors[n];int used=0;for(int i=0;i<CONTRACTS;i++)if(g.contracts[i].active==1&&S.jobActor[i]==n+1)used=1;
  if(used||a->squad||a->dead||a->unconscious>0||living_owned(n)||civil_elite_index(n)>=0||depth_named(n)>=0)continue;
  if(kind==7?(a->kind==RAIDER&&a->bountyReward>0):(a->homeTown==town&&a->kind==(kind==5?CARAVAN:GUARD))){actor=n;break;}}
  if(actor<0&&kind==5){int count=0;for(int n=0;n<g.actorCount;n++)if(g.actors[n].kind==CARAVAN&&!living_owned(n)&&depth_named(n)<0)count++;
   if(count<4&&g.actorCount<MAX_ACTORS){float x=towns[town].x,y=towns[town].y+12;for(int j=0;j<12&&!can_walk(x,y);j++)x+=1;
    if(can_walk(x,y)){actor=add_actor("Freight caravan",CARAVAN,g.townOwner[town],x,y,14);if(actor>=0){g.actors[actor].homeTown=town;g.actors[actor].gear[0]=RAPIER_ITEMS+5;inventory_add(&g.actors[actor],exports[town],4);}}}
  }
  if(actor<0){log_msg(kind==7?"No unclaimed named bounty today.":"No free employer here; finish current work.");return 0;}
 }
 Contract c={0};c.active=1;c.type=kind;c.town=town;c.target=(town+1+g.day%3)%TOWNS;c.reward=kind==4?260:kind==5?280:kind==6?180:600;c.needed=kind==4?4:1;
 if(kind==4){c.item=exports[town];int bond=item_price(town,c.item,0)*c.needed;if(g.money<bond){log_msg("Freight deposit: %d silver, refunded on delivery.",bond);return 0;}
  if(!inventory_add(p,c.item,c.needed)){log_msg("Make room for four units of freight.");return 0;}g.money-=bond;S.jobBond[slot]=bond;
 }else S.jobBond[slot]=0;
 if(kind==7)c.reward=g.actors[actor].bountyReward*2;
 g.contracts[slot]=c;S.jobActor[slot]=actor+1;S.jobStage[slot]=0;S.jobWarned[slot]=0;S.jobFar[slot]=S.jobWarning[slot]=0;S.jobElapsed[slot]=S.jobNear[slot]=S.jobPaid[slot]=S.jobDistance[slot]=0;
 if(actor>=0){S.jobX[slot]=g.actors[actor].x;S.jobY[slot]=g.actors[actor].y;}
 if(kind==5||kind==6){log_msg("Follow %s. Stay within 8m while moving to earn wages.",g.actors[actor].name);return 1;}
 log_msg(kind==4?"Freight loaded. Deliver through your journal at the destination.":kind==7?"Named bounty accepted. Carry the wanted person to this town.":"Service accepted. Stay within 8m of your employer to earn wages.");return 1;
}
void society_contract_text(int n,char*out,int size){Contract*c=&g.contracts[n];int who=S.jobActor[n]-1;
 if(c->active==3){snprintf(out,size,"Service failed. No completion bonus. Wages already earned are retained. Ask for new work at a settlement.");return;}
 if(c->type==4)snprintf(out,size,"Deliver %d %s to %s. %d silver plus %d deposit refund. Bring the cargo; no return trip.",c->needed,itemDefs[c->item].name,towns[c->target].name,c->reward,S.jobBond[n]);
 else if(c->type==7)snprintf(out,size,"Bring %s to %s. %d alive; %d dead. Last reported in %s. Carry the body; claim here.",who>=0?g.actors[who].name:"wanted raider",towns[c->town].name,c->reward,c->reward/2,who>=0?regionNames[biome(g.actors[who].homeX,g.actors[who].homeY)]:"the wastes");
 else snprintf(out,size,"%s %s. Stay within 8m while they move: 12 silver / 30s. %d completion bonus. Served %.0fs. %s",c->type==5?"Escort":"Patrol with",who>=0?g.actors[who].name:"employer",c->reward,S.jobNear[n],c->progress?"Report to the hiring town for bonus.":c->type==5?towns[c->target].name:"Three minutes of patrol duty required.");
}
int society_complete(int n){if(n<0||n>=CONTRACTS||g.contracts[n].active!=1)return 0;Contract*c=&g.contracts[n];Actor*a=&g.actors[g.selected];int town=c->type==4?c->target:c->town;if(town_near(a->x,a->y,14)!=town){log_msg("Claim at %s.",towns[town].name);return 0;}
 int reward=c->reward+S.jobBond[n];
 if(c->type==4){if(!inventory_remove(a,c->item,c->needed)){log_msg("Bring the full freight shipment.");return 0;}g.stock[town][c->item]+=c->needed;}
 else if(c->type==7){int who=S.jobActor[n]-1;if(who<0||who>=g.actorCount||a->carrying!=who||g.actors[who].bountyReward<=0){log_msg("Carry the named bounty; previously claimed proofs cannot pay twice.");return 0;}Actor*b=&g.actors[who];if(!b->dead&&b->unconscious<=0){log_msg("The prisoner must be subdued.");return 0;}if(b->dead)reward/=2;b->bountyReward=0;b->active=0;b->carriedBy=-1;a->carrying=-1;}
 else if(!c->progress){log_msg("Complete the escort or patrol first.");return 0;}
 c->active=2;c->completed=1;g.money+=reward;g.contractsDone++;g.rep[g.townOwner[town]]+=5;S.jobBond[n]=0;log_msg("Job paid: %d silver.",reward);return 1;
}
int society_npc_job(int n,float dt){
 Actor*freed=&g.actors[n];if(S.freed[n]&&!freed->squad){
  if(!conscious(freed)||freed->prisonTime>0||g.depth.actors[n].captive)return 1;
  if(S.freed[n]==1){Actor*leader=&g.actors[g.selected];if(depth_room(leader)==depth_room(freed)&&depth_distance(leader,freed)>2.4f)route_toward(freed,leader->x,leader->y,dt);}
  else if(S.freed[n]==2){route_toward(freed,freed->homeX,freed->homeY,dt);if(distance(freed->x,freed->y,freed->homeX,freed->homeY)<2){S.freed[n]=3;log_msg("%s reached refuge at %s.",freed->name,towns[freed->homeTown].name);}}
  return 1;
 }
 Actor*cook=&g.actors[n];if(cook->kind==GUARD&&cook->faction==HOSTILE&&cook->homeTown==14&&g.townOwner[14]==HOSTILE){for(int v=0;v<g.actorCount;v++)if(S.cannibal[v]==2&&!g.actors[v].dead&&distance(cook->x,cook->y,g.actors[v].x,g.actors[v].y)<24){if(distance(cook->x,cook->y,g.actors[v].x,g.actors[v].y)>2.5f)route_toward(cook,g.actors[v].x,g.actors[v].y,dt*.7f);return 1;}}

 for(int i=0;i<CONTRACTS;i++){Contract*c=&g.contracts[i];if(c->active!=1||(c->type!=5&&c->type!=6)||S.jobActor[i]!=n+1||c->progress)continue;
 Actor*a=&g.actors[n];float x=towns[c->target].x,y=towns[c->target].y;
 if(c->type==6){static const float dx[]={18,0,-18,0},dy[]={0,18,0,-18};x=towns[c->town].x+dx[S.jobStage[i]%4];y=towns[c->town].y+dy[S.jobStage[i]%4];if(distance(a->x,a->y,x,y)<3)S.jobStage[i]++;}
 if(distance(a->x,a->y,x,y)>2)route_toward(a,x,y,dt*.72f);return 1;
 }return 0;
}
static void tick_jobs(float dt){
 Actor*p=&g.actors[g.selected];for(int i=0;i<CONTRACTS;i++){Contract*c=&g.contracts[i];if(c->active!=1||(c->type!=5&&c->type!=6)||c->progress)continue;
 int n=S.jobActor[i]-1;if(n<0||n>=g.actorCount)continue;Actor*a=&g.actors[n];
 if(a->dead||!a->active){c->active=3;log_warn("Service failed: employer died.");continue;}
 if(distance(a->x,a->y,p->x,p->y)>45)society_npc_job(n,dt);
 int near=conscious(p)&&depth_room(p)==depth_room(a)&&depth_distance(p,a)<=8;
 S.jobWarning[i]=fmaxf(0,S.jobWarning[i]-dt);
 if(!near){S.jobFar[i]+=dt;if(S.jobFar[i]>=2&&S.jobWarning[i]<=0){log_warn("%s: Keep up! You are too far away; wages paused.",a->name);S.jobWarned[i]=1;S.jobWarning[i]=25;}}
 else {S.jobFar[i]=0;if(S.jobWarned[i]&&depth_distance(p,a)<7){log_msg("%s: There you are. Stay with me to earn your pay.",a->name);S.jobWarned[i]=0;}}
 S.jobElapsed[i]+=dt;float moved=distance(a->x,a->y,S.jobX[i],S.jobY[i]);S.jobX[i]=a->x;S.jobY[i]=a->y;
 if(near&&conscious(a)&&moved>.001f){S.jobNear[i]+=dt;S.jobDistance[i]+=moved;if(S.jobNear[i]-S.jobPaid[i]>=30){g.money+=12;S.jobPaid[i]+=30;log_msg("Service wage: +12 silver.");}}
 if((c->type==5&&distance(a->x,a->y,towns[c->target].x,towns[c->target].y)<4)||(c->type==6&&S.jobNear[i]>=180&&S.jobDistance[i]>=35)){
   if(c->type==6||S.jobNear[i]>=S.jobElapsed[i]*.55f){c->progress=1;log_msg("Service complete. Claim your bonus at %s.",towns[c->town].name);}else{c->active=3;log_warn("Escort arrived without enough service; bonus forfeited.");}
 }
 if(S.jobElapsed[i]>3600){c->active=3;log_warn("Service contract expired.");}
 }
}
static int near_guard(const Actor*a,int town,float reach){for(int n=0;n<g.actorCount;n++){Actor*b=&g.actors[n];if(!b->squad&&b->kind==GUARD&&b->faction==g.townOwner[town]&&b->homeTown==town&&conscious(b)&&depth_distance(a,b)<reach)return n;}return -1;}
static void tick_cannibals(float dt){
 for(int n=0;n<g.actorCount;n++){Actor*a=&g.actors[n];if(a->dead)continue;
 if(S.cannibal[n]==1){int captor=a->carriedBy;if(captor<0||!conscious(&g.actors[captor])){S.cannibal[n]=0;g.depth.actors[n].captive=0;continue;}
 Actor*b=&g.actors[captor];route_toward(b,towns[14].x-10,towns[14].y+6,dt*.7f);a->x=b->x;a->y=b->y;
 if(distance(b->x,b->y,towns[14].x-10,towns[14].y+6)<2){b->carrying=-1;a->carriedBy=-1;S.cannibal[n]=2;S.eating[n]=0;log_warn("%s is bound at the cooking ground. Rescue them!",a->name);}
 }else if(S.cannibal[n]==2){
 DepthActor*z=&g.depth.actors[n];a->target=-1;a->attackPhase=0;a->taskKind=0;

 if(near_guard(a,14,5)>=0){S.eating[n]+=dt;if(S.eating[n]>=45){S.eating[n]-=10;a->health[CHEST]-=12;a->blood-=15;a->unconscious=fmaxf(a->unconscious,12);if(a->squad)log_warn("Cannibals are eating %s!",a->name);if(a->blood<=0||a->health[CHEST]<-70){a->dead=1;a->unconscious=9999;z->captive=0;S.cannibal[n]=0;}}}
 }else if(a->squad&&a->race!=2&&!actor_inside(a)&&a->unconscious>0&&a->carriedBy<0&&!g.depth.actors[n].captive&&g.townOwner[14]==HOSTILE&&distance(a->x,a->y,towns[14].x,towns[14].y)<24){int c=near_guard(a,14,TALK_REACH);if(c>=0&&g.actors[c].carrying<0){g.actors[c].carrying=n;a->carriedBy=c;S.cannibal[n]=1;g.depth.actors[n].captive=3;a->bleed=0;log_warn("Cannibals seized %s and are carrying them to the fire.",a->name);}}
 }
}
void society_tick(float dt){
 tick_rescues(dt);
 tick_jobs(dt);tick_cannibals(dt);
 for(int n=0;n<g.actorCount;n++)if(g.actors[n].squad){Actor*a=&g.actors[n];S.gateMessage[n]=fmaxf(0,S.gateMessage[n]-dt);for(int f=0;f<FACTIONS;f++)S.recognized[n][f]=fmaxf(0,S.recognized[n][f]-dt);
  int t=town_near(a->x,a->y,18),f=t>=0?g.townOwner[t]:FREE;
  if(t>=0&&f>0&&conscious(a)&&near_guard(a,t,3)>=0&&disguise_faction(a)==f){int score=disguise_score(a,f);S.suspicion[n]+=dt*((100-score)/30.f+(wanted_amount(a,f)>=300?8:0));if(S.recognized[n][f]<=0&&(S.suspicion[n]>=100||!score)){S.suspicion[n]=0;crime_add(a,f,80);log_warn("Your %s disguise was exposed.",factionNames[f]);}}
  else S.suspicion[n]=fmaxf(0,S.suspicion[n]-dt*2);
 }
 S.economyClock+=dt;if(S.economyClock>=60){S.economyClock-=60;
 for(int t=0;t<TOWNS;t++){int it=exports[t];g.stock[t][it]=(int)bound(g.stock[t][it]+4,0,90);int food=g.stock[t][RATION]+g.stock[t][GRAIN];if(g.stock[t][RATION]>0)g.stock[t][RATION]--;else if(g.stock[t][GRAIN]>0)g.stock[t][GRAIN]--;if(g.stock[t][WATER]>0)g.stock[t][WATER]--;g.prosperity[t]=(int)bound(g.prosperity[t]+(food<4?-1:1),10,95);
  if(g.day!=S.lastEconomyDay){S.crisis[t]=(g.day+t*3+g.seed)%9<2?1+(g.day+t)%2:0;if(S.crisis[t])g.stock[t][imports[t]]=g.stock[t][imports[t]]/2;}
 }S.lastEconomyDay=g.day;}
}
static NearbyAction actions[ACTION_LIMIT];static int actionCount;
static void add_action(int type,int kind,int target){if(actionCount<ACTION_LIMIT)actions[actionCount++]=(NearbyAction){type,kind,target,depth_room(&g.actors[g.selected])};}
int living_local_count(void){int n=0;Actor*a=&g.actors[g.selected];for(int e=0;e<EVENTS;e++)if(!g.living.events[e].status&&distance(a->x,a->y,g.living.events[e].x,g.living.events[e].y)<8)n++;return n;}
int living_local_slot(int row){Actor*a=&g.actors[g.selected];for(int e=0;e<EVENTS;e++)if(!g.living.events[e].status&&distance(a->x,a->y,g.living.events[e].x,g.living.events[e].y)<8&&row--==0)return e;return -1;}
int nearby_actions(void){
 actionCount=0;Actor*a=&g.actors[g.selected];int room=depth_room(a);
 if(g.depth.actors[g.selected].captive||a->prisonTime>0)add_action(ACTION_RESCUE,0,g.selected);
 for(int n=0;n<g.actorCount;n++){Actor*b=&g.actors[n];if(n==g.selected||!b->active||depth_room(b)!=room||depth_distance(a,b)>TALK_REACH||b->carriedBy>=0)continue;
  if(!b->dead&&(g.depth.actors[n].captive||b->prisonTime>0))add_action(ACTION_RESCUE,0,n);
  if(S.freed[n]&&!b->squad&&!b->dead){add_action(S.freed[n]==1?ACTION_RESCUED_LEAVE:ACTION_RESCUED_FOLLOW,0,n);add_action(ACTION_RESCUED_RECRUIT,0,n);}
  if(b->dead||b->unconscious>0){add_action(ACTION_CONTEXT,0,n);if(a->carrying<0&&!g.depth.actors[n].captive)add_action(ACTION_CARRY,0,n);}
  else if(!b->squad){if(!hostile(a,b)||b->target<0){add_action(ACTION_CONTEXT,0,n);if(a->sneaking)add_action(ACTION_STEAL,0,n);}add_action(ACTION_ATTACK,0,n);}
  else add_action(ACTION_CONTEXT,0,n);
 }
 if(room){if(interior_context()!=-2)add_action(ACTION_CONTEXT,5,room-1);return actionCount;}
 for(int n=0;n<g.nodeCount;n++)if(g.nodes[n].remaining&&distance(a->x,a->y,g.nodes[n].x,g.nodes[n].y)<=WORK_REACH){if(g.nodes[n].locked){add_action(ACTION_UNLOCK,1,n);add_action(ACTION_OPEN,1,n);}else add_action(ACTION_CONTEXT,1,n);}
 for(int b=0;b<g.buildingCount;b++){Building*v=&g.buildings[b];if(building_has_interior(v->type)&&distance(a->x,a->y,v->x+1.45f,v->y+.65f)<=1.7f)add_action(ACTION_CONTEXT,3,b);if(v->type>=WELL&&v->type!=WALL&&v->type!=WINDMILL&&distance(a->x,a->y,v->x,v->y)<=1.7f)add_action(ACTION_CONTEXT,2,b);}
 for(int k=0;k<LANDMARKS;k++)if(distance(a->x,a->y,g.depth.siteX[k],g.depth.siteY[k])<=1.7f)add_action(ACTION_CONTEXT,4,k);
 for(int i=0;i<living_local_count();i++)add_action(ACTION_EVENT,6,living_local_slot(i));
 for(int k=0;k<POP_GROUPS;k++)if(g.population.version&&distance(a->x,a->y,g.population.groups[k].x,g.population.groups[k].y)<55){
  if(distance(a->x,a->y,g.population.groups[k].x,g.population.groups[k].y)<8)add_action(ACTION_POPULATION,0,k);
  int count=population_object_count(k);
  for(int j=0;j<count;j++){float x,y;int type=population_object(k,j,&x,&y);if(distance(a->x,a->y,x,y)>1.7f)continue;
   if(type==1){add_action(ACTION_POPULATION,1,k);if(g.population.groups[k].type==POP_MERCHANTS||g.population.groups[k].type==POP_SMUGGLERS)for(int v=0;v<3;v++)add_action(ACTION_POPULATION,30+v,k);}
   else if(type==2)add_action(ACTION_POPULATION,10+j,k);
   else if(type==3)add_action(ACTION_POPULATION,20,k);
  }
 }
 int town=town_near(a->x,a->y,24);if(town>=0&&g.life.version&&g.life.towns[town].stage)add_action(ACTION_POLITICS,0,town);
 return actionCount;
}
const NearbyAction *nearby_action(int row){return row>=0&&row<actionCount?&actions[row]:NULL;}
void nearby_action_label(int row,char*out,int size){const NearbyAction*x=nearby_action(row);if(!x){snprintf(out,size,"Nothing within reach");return;}
 if(x->type==ACTION_POLITICS){snprintf(out,size,"Council at %s",towns[x->target].name);return;}
 if(x->type==ACTION_POPULATION){population_action_label(x->target,x->kind,out,size);return;}
 if(x->type==ACTION_EVENT){char name[70];living_title(x->target,name,sizeof(name));snprintf(out,size,"Investigate %s",name);return;}
 if(x->type>=ACTION_RESCUED_FOLLOW){snprintf(out,size,"%s %s",x->type==ACTION_RESCUED_FOLLOW?"Ask to follow:":x->type==ACTION_RESCUED_LEAVE?"Leave me alone:":"Recruit:",g.actors[x->target].name);return;}
 if(x->kind==5){snprintf(out,size,"%s",interior_prompt());return;}
 if(x->type==ACTION_ATTACK||x->type==ACTION_CARRY||x->type==ACTION_RESCUE||x->type==ACTION_STEAL){snprintf(out,size,"%s %s",x->type==ACTION_ATTACK?"Attack":x->type==ACTION_CARRY?"Carry":x->type==ACTION_RESCUE?(g.actors[x->target].prisonTime>0?"Free prisoner:":"Free from shackles:"):"Pickpocket",g.actors[x->target].name);return;}
 int kind=ui.contextKind,index=ui.contextIndex;ui.contextKind=x->kind;ui.contextIndex=x->target;
 snprintf(out,size,"%s %s",x->type==ACTION_UNLOCK?"Pick lock":x->type==ACTION_OPEN?"Open after unlocking":context_action(),context_name());ui.contextKind=kind;ui.contextIndex=index;
}
static int action_valid(const NearbyAction*x){Actor*a=&g.actors[g.selected];if(!conscious(a)||depth_room(a)!=x->room)return 0;int n=x->target;
 if(x->type==ACTION_POLITICS)return n>=0&&n<TOWNS&&distance(a->x,a->y,towns[n].x,towns[n].y)<=24;
 if(x->type==ACTION_POPULATION)return population_action_valid(n,x->kind);
 if(x->type==ACTION_EVENT)return n>=0&&n<EVENTS&&!g.living.events[n].status&&distance(a->x,a->y,g.living.events[n].x,g.living.events[n].y)<=8;
 if(x->kind==0)return n>=0&&n<g.actorCount&&g.actors[n].active&&depth_room(&g.actors[n])==x->room&&depth_distance(a,&g.actors[n])<=TALK_REACH;
 if(x->kind==1)return n>=0&&n<g.nodeCount&&g.nodes[n].remaining&&distance(a->x,a->y,g.nodes[n].x,g.nodes[n].y)<=WORK_REACH;
 if(x->kind==4)return n>=0&&n<LANDMARKS&&distance(a->x,a->y,g.depth.siteX[n],g.depth.siteY[n])<=1.7f;
 if(x->kind==5)return g.interiorBuilding==n+1;
 return n>=0&&n<g.buildingCount&&distance(a->x,a->y,g.buildings[n].x+(x->kind==3?1.45f:0),g.buildings[n].y+(x->kind==3?.65f:0))<=1.7f;
}
static void perform_action(const NearbyAction*x){Actor*a=&g.actors[g.selected];int n=x->target;if(!action_valid(x)){log_msg("Queued target is no longer within reach.");return;}
 if(x->type==ACTION_POLITICS){politics_open(n);return;}
 if(x->type==ACTION_POPULATION){population_action(n,x->kind);return;}
 if(x->type==ACTION_EVENT){WorldEvent*e=&g.living.events[n];if(e->type==2||e->type==3||e->type==4)living_action(n);else{char b[160];living_detail(n,b,sizeof(b));log_msg("%s",b);}return;}
 if(x->type==ACTION_ATTACK){command_attack(n);return;}
 if(x->type==ACTION_RESCUE){society_rescue_start(n);return;}
 if(x->type>=ACTION_RESCUED_FOLLOW){society_rescue_order(n,x->type);return;}
 if(x->type==ACTION_CARRY){Actor*b=&g.actors[n];if(a->carrying<0&&b->carriedBy<0&&!g.depth.actors[n].captive&&(b->dead||b->unconscious>0)){a->carrying=n;b->carriedBy=g.selected;b->x=a->x;b->y=a->y;log_msg("Carrying %s.",b->name);}return;}
 if(x->type==ACTION_STEAL){if(a->sneaking&&conscious(&g.actors[n])){ui.lootActor=n;ui.selection=0;ui.screen=SCREEN_THEFT;}return;}
 if(x->type==ACTION_OPEN&&g.nodes[n].locked){log_msg("The chest is still locked.");return;}
 if(x->kind==5){interior_interact();return;}
 interact_context(x->kind,n);
}
void action_menu_open(void){nearby_actions();if(!actionCount){log_msg("No actions within reach.");return;}ui.screen=SCREEN_ACTIONS;ui.selection=0;ui.menuReturn=0;}
void nearby_action_choose(int row){const NearbyAction*p=nearby_action(row);if(!p)return;NearbyAction a=*p;int n=g.selected;ui.screen=ui.paused?SCREEN_ACTIONS:SCREEN_WORLD;
 if(ui.paused){int q=S.queueCount[n];if(q>=ACTION_QUEUE){log_msg("Four actions queued. Clear or resume first.");return;}S.queue[n][q]=a;S.queueCount[n]++;log_msg("Action queued. R resumes; movement cancels queued actions.");}
 else perform_action(&a);
}
void society_queue_tick(void){int n=g.selected;Actor*a=&g.actors[n];if(ui.paused||ui.screen!=SCREEN_WORLD||!S.queueCount[n]||a->taskKind||a->attackPhase||society_rescue_busy(n))return;NearbyAction x=S.queue[n][0];for(int i=1;i<S.queueCount[n];i++)S.queue[n][i-1]=S.queue[n][i];S.queueCount[n]--;perform_action(&x);}
int society_validate(const Game*v){const Society*s=&v->society;if(!s->version)return 1;if(s->version!=1||!isfinite(s->economyClock)||s->economyClock<0||s->prisonSeeded<0||s->prisonSeeded>1)return 0;
 for(int t=0;t<TOWNS;t++)if(s->crisis[t]<0||s->crisis[t]>2)return 0;
 for(int n=0;n<MAX_ACTORS;n++){if(s->rescueHelper[n]<0||s->rescueHelper[n]>v->actorCount||s->freed[n]<0||s->freed[n]>3||s->captorFaction[n]<0||s->captorFaction[n]>=FACTIONS)return 0;for(int c=0;c<WEAPON_CLASSES;c++)if(!isfinite(s->weaponXP[n][c])||s->weaponXP[n][c]<0||s->weaponXP[n][c]>117612)return 0;if(s->queueCount[n]<0||s->queueCount[n]>ACTION_QUEUE||s->cannibal[n]<0||s->cannibal[n]>2||!isfinite(s->eating[n])||!isfinite(s->suspicion[n])||!isfinite(s->gateMessage[n]))return 0;
  for(int f=0;f<FACTIONS;f++)if(s->wanted[n][f]<0||s->wanted[n][f]>100000||!isfinite(s->recognized[n][f]))return 0;
  for(int q=0;q<s->queueCount[n];q++){const NearbyAction*a=&s->queue[n][q];if(a->type<0||a->type>ACTION_POLITICS||a->kind<0||a->kind>32||a->room<0||a->room>v->buildingCount||a->target<0||a->target>=MAX_NODES)return 0;
   if(a->type==ACTION_POLITICS){if(a->target>=TOWNS||a->kind!=0)return 0;continue;}
   if(a->type==ACTION_POPULATION){if(a->target>=POP_GROUPS||!(a->kind==0||a->kind==1||(a->kind>=11&&a->kind<=13)||a->kind==20||(a->kind>=30&&a->kind<=32)))return 0;continue;}
   if(a->kind>6)return 0;
   if((a->type==ACTION_OPEN||a->type==ACTION_UNLOCK)&&a->kind!=1)return 0;
   if((a->type==ACTION_ATTACK||a->type==ACTION_CARRY||a->type==ACTION_STEAL||a->type==ACTION_RESCUE||a->type>=ACTION_RESCUED_FOLLOW)&&a->kind!=0)return 0;
   if(a->type==ACTION_EVENT&&a->kind!=6)return 0;}
 }
 for(int j=0;j<CONTRACTS;j++)if(s->jobStage[j]<0||s->jobStage[j]>10000000||s->jobActor[j]<0||s->jobActor[j]>v->actorCount||s->jobBond[j]<0||s->jobWarned[j]<0||s->jobWarned[j]>1||!isfinite(s->jobFar[j])||!isfinite(s->jobWarning[j])||!isfinite(s->jobElapsed[j])||!isfinite(s->jobNear[j])||!isfinite(s->jobPaid[j])||!isfinite(s->jobDistance[j])||!isfinite(s->jobX[j])||!isfinite(s->jobY[j]))return 0;
 return 1;
}
#undef S
