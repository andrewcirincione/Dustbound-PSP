#include "game.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int conscious(const Actor*);
extern void init_actor(int,const char*,int,int,float,float,int);
#define L (g.life)
static int bound(int x,int lo,int hi){return x<lo?lo:x>hi?hi:x;}
static int guarded(int t,int faction){int n=0;for(int i=0;i<g.actorCount;i++){Actor*a=&g.actors[i];if(!a->squad&&!population_owned(i)&&a->active&&!a->dead&&a->unconscious<=0&&a->kind==GUARD&&a->homeTown==t&&a->faction==faction)n++;}return n;}
static void notice(int t,const char *event){snprintf(g.worldNews,sizeof(g.worldNews),"%s: %s",towns[t].name,event);if(g.discovered[t]||town_near(g.actors[g.selected].x,g.actors[g.selected].y,25)==t)log_warn("%s",g.worldNews);}
static void succession_begin(int t,int elite){
 TownPolitics*p=&L.towns[t];if(p->stage==1)return;
 p->stage=1;p->elite=elite;p->original=g.townOwner[t];p->pledge=-1;p->relief=0;p->winner=0;p->age=p->pulse=0;p->support[0]=40;p->support[1]=28;
 int rival=p->original==UNION?SHOGUN:p->original==SHOGUN?UNION:p->original==MONKS?SHOGUN:p->original==HOSTILE?UNION:p->original==OUTLAWS?SCAVENGERS:OUTLAWS;p->rival=rival;
 for(int k=0;k<POP_GROUPS;k++){WorldEvent*e=&g.population.groups[k];if(e->type==POP_ARMY&&e->faction==rival&&distance(e->x,e->y,towns[t].x,towns[t].y)<180){p->support[1]+=8;break;}}
 notice(t,"a caretaker and a rival claimant contest the vacant seat");
}

static const char *warNames[WAR_FACTIONS]={"The Compact","The Shogunate","The Ash order"};
int war_faction_index(int faction){
 switch(faction){case UNION:return 0;case SHOGUN:return 1;case MONKS:return 2;default:return -1;}
}
int war_rivals(int a,int b){return a!=b && war_faction_index(a)>=0 && war_faction_index(b)>=0;}

static void war_record(WarReport report){
 g.war.report[g.war.reportNext]=report;
 g.war.reportNext=(g.war.reportNext+1)%WAR_REPORTS;
}
static void war_record_takeover(int town,int winner,int other,int prisoners){
 if(!g.war.version || winner==other)return;
 g.war.heldSince[town]=g.time;
 war_record((WarReport){WAR_TAKEOVER,town,winner,other,prisoners,towns[town].x,towns[town].y,g.time});
}
void war_report_clash(int a,int b,float x,float y){
 if(!g.war.version||!war_rivals(a,b))return;
 for(int i=0;i<WAR_REPORTS;i++){
  const WarReport *r=&g.war.report[i];
  if(r->kind==WAR_CLASH && g.time-r->time<180 && ((r->winner==a&&r->other==b)||(r->winner==b&&r->other==a)) &&
     distance(x,y,r->x,r->y)<65)return;
 }
 int town=0;
 for(int t=1;t<TOWNS;t++)if(distance(x,y,towns[t].x,towns[t].y)<distance(x,y,towns[town].x,towns[town].y))town=t;
 war_record((WarReport){WAR_CLASH,town,a,b,0,x,y,g.time});
}
static int founder(int town,int *elite){
 *elite=-1;
 if(town>=OLD_TOWNS)return g.war.resident[town-OLD_TOWNS][0]-1;
 for(int e=0;e<ELITES;e++)if(eliteDefs[e].town==town){
  *elite=e;
  return g.civil.eliteActor[e]-1;
 }
 return -1;
}
static int heir_candidate(int t,int faction){
 int best=-1,rank=-1;
 for(int n=0;n<g.actorCount;n++){
  const Actor *a=&g.actors[n];
  if(!a->active||a->dead||a->squad||a->faction!=faction||a->homeTown!=t||a->kind!=GUARD||
     a->unconscious>0||a->prisonTime>0||a->carriedBy>=0||a->carrying>=0||g.depth.actors[n].captive||
     g.society.freed[n]||living_owned(n)||merc_slot(a)>=0||civil_elite_index(n)>=0||
     depth_named(n)>=0||regional_named(n))continue;
  int employed=0;
  for(int i=0;i<CONTRACTS;i++)if(g.contracts[i].active==1&&g.society.jobActor[i]==n+1){employed=1;break;}
  if(employed)continue;
  int merit=level(a,DEFENCE);
  if(merit>rank){rank=merit;best=n;}
 }
 return best;
}
static const char *outside_claimant(int town,int faction);
static int dispatch_heir(int t,int faction){
 int n=-1;
 for(int i=0;i<g.actorCount;i++)if(g.living.owned[i]<0){n=i;break;}
 if(n<0&&g.actorCount>=MAX_ACTORS)return -1;
 float x=towns[t].x,y=towns[t].y;
 for(int i=0;!can_walk(x,y)&&i<25;i++){
  x=towns[t].x+i%5-2;
  y=towns[t].y+3+i/5;
 }
 if(!can_walk(x,y))return -1;
 const char *name=outside_claimant(t,faction);
 if(n<0)n=g.actorCount++;
 init_actor(n,name,GUARD,faction,x,y,28);
 Actor *a=&g.actors[n];
 a->homeTown=t;
 if(war_faction_index(faction)>=0)war_outfit(a,ROLE_MARSHAL,n%4,3);
 g.living.owned[n]=0;
 g.living.anger[n]=0;
 g.living.command[n]=-1;
 return n;
}
static void seat_heir(int t,int n){
 L.towns[t].leader=n+1;
 if(n>=0)g.actors[n].voice=ROLE_SEATED_LEADER;
}
static int current_heir(int t){
 const TownPolitics *p=&L.towns[t];
 if(p->stage!=2||!p->leader)return -1;
 int n=p->leader-1;
 const Actor *a=&g.actors[n];
 return a->active&&!a->dead&&!a->squad&&a->homeTown==t&&a->faction==g.townOwner[t]&&
        !g.depth.actors[n].captive?n:-1;
}
const char *politics_leader_title(int n){
 int t=g.actors[n].homeTown;
 if(t<0||t>=TOWNS||current_heir(t)!=n)return NULL;
 switch(g.actors[n].faction){
  case UNION:return "COMPACT WARDEN";
  case SHOGUN:return "IRON STEWARD";
  case MONKS:return "ASH PRIOR";
  default:return "TOWN STEWARD";
 }
}
int politics_protected(int n){
 int t=g.actors[n].homeTown;
 if(g.actors[n].voice==ROLE_SEATED_LEADER || (t>=0&&t<TOWNS&&L.towns[t].leader==n+1))return 1;
 for(int k=0;k<WAR_FACTIONS;k++)if(g.war.resident[k][0]==n+1)return 1;
 return 0;
}
static void announce_heir(int t,int n){
 if(n<0){notice(t,"the seat remains vacant");return;}
 char line[76];
 snprintf(line,sizeof(line),"%s now leads the settlement",g.actors[n].name);
 notice(t,line);
 if(g.war.version)war_record((WarReport){WAR_SUCCESSION,t,g.townOwner[t],0,0,towns[t].x,towns[t].y,g.time});
}
void politics_occupy(int t,int faction){
 if(t<0||t>=TOWNS||faction<=FREE||faction>=FACTIONS)return;int old=g.townOwner[t];
 TownPolitics *p=&L.towns[t];
 int election=p->stage==1;
 int nominee=election?heir_candidate(t,faction):-1;
 g.townOwner[t]=faction;
 for(int n=0;n<g.actorCount;n++){Actor*a=&g.actors[n];if(!a->squad&&!a->dead&&!population_owned(n)&&a->homeTown==t&&a->faction==old&&!g.society.freed[n]&&merc_slot(a)<0&&!g.depth.actors[n].captive)a->faction=faction;}
 for(int b=0;b<g.buildingCount;b++)if(g.buildings[b].owner<0&&town_near(g.buildings[b].x,g.buildings[b].y,20)==t)g.buildings[b].faction=faction;
 p->stage=2;p->winner=faction;p->policy=faction==MONKS?1:faction==UNION||faction==SCAVENGERS?3:2;
 if(!election)nominee=heir_candidate(t,faction);
 else if(nominee<0)nominee=dispatch_heir(t,faction);
 seat_heir(t,nominee);
 /* Physical aftermath uses surviving residents, never respawns unique loot. */
 int posts=0,displaced=0;
 for(int n=0;n<g.actorCount;n++){Actor*a=&g.actors[n];if(!a->active||a->dead||a->squad||a->homeTown!=t||population_owned(n)||g.society.freed[n]||merc_slot(a)>=0||civil_elite_index(n)>=0||depth_named(n)>=0||regional_named(n)||g.depth.actors[n].captive)continue;
  if(a->kind==GUARD){float x=towns[t].x+(posts%2?-6:6),y=towns[t].y+(posts/2%2?6:-6);posts++;if(can_walk(x,y)){a->homeX=a->tx=x;a->homeY=a->ty=y;a->routeLen=a->routePos=0;}}
  else if(old!=faction&&a->kind==SETTLER&&npc_role(a)==ROLE_CITIZEN&&displaced<2){float x=towns[t].x+17+displaced*2,y=towns[t].y+10;if(can_walk(x,y)){a->homeX=a->tx=x;a->homeY=a->ty=y;a->faction=FREE;displaced++;snprintf(a->name,sizeof(a->name),"Displaced local");}}
  else if(p->policy==1&&npc_role(a)==ROLE_MEDIC){float x=towns[t].x+6,y=towns[t].y+6;if(can_walk(x,y)){a->homeX=a->tx=x;a->homeY=a->ty=y;}}
 }
 g.prosperity[t]=bound(g.prosperity[t]+(faction==old?4:-5),10,95);L.succession++;
 if(election)announce_heir(t,nominee);
 else notice(t,faction==old?"the caretaker has secured the succession":"a new government controls the settlement");
 if(old!=faction){
  society_release_war_prisoners(t);
  frontier_restock(t);regional_restock(t);civil_stock(t);
  war_record_takeover(t,faction,old,0);
 }
}

void war_conquer(int t,int faction,int guard){
 int old=g.townOwner[t];
 if(old==faction)return;
 int count=0;
 float x,y;war_yard(t,&x,&y);
 if(g.war.version && guard>=0 && war_rivals(old,faction)){
  for(int n=0;n<g.actorCount&&count<2;n++){
   Actor *a=&g.actors[n];DepthActor *z=&g.depth.actors[n];
   if(!a->active||a->dead||a->squad||a->kind!=GUARD||a->faction!=old||a->homeTown!=t||a->unconscious<=0||
      z->captive||z->room||a->prisonTime>0||a->carrying>=0||a->carriedBy>=0||living_owned(n)||
      depth_named(n)>=0||regional_named(n)||merc_slot(a)>=0)continue;
   float px=x+count*1.4f,py=y;
   if(!can_walk(px,py))continue;
   z->confiscated=a->gear[0];z->confiscatedOwner=a->gearStolen[0];z->captive=1;
   a->gear[0]=a->gearStolen[0]=0;
   a->x=px;a->y=py;a->target=a->attackTarget=-1;a->attackPhase=a->taskKind=0;
   a->routeLen=a->routePos=0;
   g.living.command[n]=-1;
   g.society.captorFaction[n]=faction;
   g.war.prisonTown[n]=t+1;
   count++;
  }
 }
 politics_occupy(t,faction);
 if(g.war.version){
  WarReport *r=&g.war.report[(g.war.reportNext+WAR_REPORTS-1)%WAR_REPORTS];
  r->prisoners=count;
  if(count&&g.discovered[t])log_warn("%s: %d defeated soldiers were put to work in chains.",towns[t].name,count);
 }
 if(guard>=0 && can_walk(x+1,y-1.7f)){
  Actor *a=&g.actors[guard];
  a->homeX=a->tx=x+1;a->homeY=a->ty=y-1.7f;
  a->routeLen=a->routePos=0;
 }
}

void war_init(void){
 if(g.war.version)return;
 g.war.version=1;
 for(int t=OLD_TOWNS;t<TOWNS;t++){L.towns[t].elite=-1;L.towns[t].pledge=-1;}
 war_build_capitals();
 war_add_residents();
 for(int i=0;i<WAR_FACTIONS;i++){
  g.war.nextRaid[i]=g.time+90+i*55;
  population_generate(OLD_POP_GROUPS+i,POP_ARMY);
  L.groups[OLD_POP_GROUPS+i].goal=LIFE_RECOVER;
  L.groups[OLD_POP_GROUPS+i].clock=90+i*55;
 }
}

void war_tick_labor(float dt){
 if(!g.war.version)return;
 g.war.yardClock+=dt;
 int payment=g.war.yardClock>=60;
 if(payment)g.war.yardClock-=60;
 for(int n=0;n<g.actorCount;n++){
  int t=g.war.prisonTown[n]-1;
  if(t<0)continue;
  Actor *a=&g.actors[n];
  if(a->dead||!g.depth.actors[n].captive){g.war.prisonTown[n]=0;continue;}
  if(g.society.captorFaction[n]!=g.townOwner[t]){society_release_war_prisoners(t);continue;}
  if(!payment||!conscious(a)||g.society.rescueHelper[n])continue;
  if(!guarded(t,g.townOwner[t]))continue;
  int item=g.townOwner[t]==UNION?PARTS:g.townOwner[t]==SHOGUN?IRONBAR:CLOTH;
  if(g.stock[t][item]<60)g.stock[t][item]++;
 }
}

enum { GOSSIP_BANDIT=24, GOSSIP_REPORT=32 };
static int war_choose_rumor(const Actor *a,int spontaneous){
 int best=-1;float recent=-1;
 for(int j=0;j<WAR_REPORTS;j++){
  int i=(g.war.reportNext+WAR_REPORTS-1-j)%WAR_REPORTS;
  const WarReport *r=&g.war.report[i];
  if(!r->kind)continue;
  float dist=distance(a->x,a->y,r->x,r->y),age=g.time-r->time;
  float reach=r->kind==WAR_CLASH?240:r->town>=OLD_TOWNS?WORLD*1.5f:750;
  if(dist>reach||age<dist*.08f||age>(r->kind==WAR_CLASH?450:1800))continue;
  if(r->kind==WAR_TAKEOVER && g.townOwner[r->town]!=r->winner)continue;
  if(r->kind==WAR_SUCCESSION && (g.townOwner[r->town]!=r->winner||current_heir(r->town)<0))continue;
  float score=r->time+(r->kind==WAR_CLASH?0:300);
  if(score>recent){best=i;recent=score;}
 }
 if(best>=0&&town_near(a->x,a->y,18)>=0&&g.war.report[best].kind!=WAR_CLASH&&
    g.time-g.war.report[best].time<480&&(!spontaneous||g.war.lastSpoken!=GOSSIP_REPORT+best))return GOSSIP_REPORT+best;
 float x,y;
 if(population_bandits(a->x,a->y,&x,&y)){
  int octant=(int)floorf(atan2f(y-a->y,x-a->x)*1.27323954f+.5f);
  return GOSSIP_BANDIT+((octant+8)%8);
 }
 return best>=0?GOSSIP_REPORT+best:0;
}
static const char *war_side_name(int f){int side=war_faction_index(f);return side<0?factionNames[f]:warNames[side];}
const char *war_ambient(int speech,int faction){
 static char line[96];
 if(speech>=GOSSIP_BANDIT && speech<GOSSIP_REPORT){
  static const char *directions[]={"east","southeast","south","southwest","west","northwest","north","northeast"};
  snprintf(line,sizeof(line),"Saw bandits %s of here. Watch the road.",directions[speech-GOSSIP_BANDIT]);
  return line;
 }
 if(speech<GOSSIP_REPORT || speech>=GOSSIP_REPORT+WAR_REPORTS)return NULL;
 const WarReport *r=&g.war.report[speech-GOSSIP_REPORT];
 if(r->kind==WAR_CLASH)snprintf(line,sizeof(line),"%s and %s clashed near %s.",war_side_name(r->winner),war_side_name(r->other),towns[r->town].name);
 else if(r->kind==WAR_SUCCESSION){
  int n=current_heir(r->town);
  if(n<0||g.townOwner[r->town]!=r->winner)return NULL;
  snprintf(line,sizeof(line),"%s now leads %s.",g.actors[n].name,towns[r->town].name);
 }else if(r->kind==WAR_TAKEOVER){
  const char *town=towns[r->town].name;
  if(faction==r->winner)snprintf(line,sizeof(line),"We took %s.%s",town,r->prisoners?" The prisoners are in chains.":"");
  else if(faction==r->other)snprintf(line,sizeof(line),"We lost %s to %s.%s",town,war_side_name(r->winner),r->prisoners?" They took prisoners.":"");
  else snprintf(line,sizeof(line),"%s took %s.%s",war_side_name(r->winner),town,r->prisoners?" They took prisoners.":"");
 }else return NULL;
 return line;
}
int war_chatter(int n){
 Actor *a=&g.actors[n];
 if(!g.war.version||g.time<g.war.nextGossip||!conscious(a)||a->target>=0||g.depth.actors[n].captive||a->prisonTime>0||
    a->kind==RAIDER||a->faction==HOSTILE||a->faction==WILDLIFE||actor_inside(a))return 0;
 int key=war_choose_rumor(a,1);
 if(!key){g.war.nextGossip=g.time+8;return 0;}
 if(key==g.war.lastSpoken){g.war.nextGossip=g.time+45;g.war.lastSpoken=0;return 0;}
 g.war.nextGossip=g.time+28;
 g.war.lastSpoken=key;
 return key;
}
const char *war_rumor(const Actor *speaker){
 if(!g.war.version)return NULL;
 return war_ambient(war_choose_rumor(speaker,0),speaker->faction);
}

int war_validate(const Game *s){
 const FrontierWar *w=&s->war;
 if(!w->version)return 1;
 if(w->version!=1||w->reportNext<0||w->reportNext>=WAR_REPORTS||w->lastSpoken<0||w->lastSpoken>=GOSSIP_REPORT+WAR_REPORTS||
    !isfinite(w->nextGossip)||!isfinite(w->yardClock)||w->yardClock<0)return 0;
 for(int k=0;k<WAR_FACTIONS;k++){
  if(!isfinite(w->nextRaid[k]))return 0;
  for(int j=0;j<WAR_RESIDENTS;j++)if(w->resident[k][j]<0||w->resident[k][j]>s->actorCount)return 0;
 }
 for(int t=0;t<TOWNS;t++)if(!isfinite(w->heldSince[t])||w->heldSince[t]<0)return 0;
 for(int n=0;n<MAX_ACTORS;n++)if(w->prisonTown[n]<0||w->prisonTown[n]>TOWNS||(w->prisonTown[n]&&n>=s->actorCount))return 0;
 for(int i=0;i<WAR_REPORTS;i++){
  const WarReport *r=&w->report[i];
  if(r->kind<0||r->kind>WAR_SUCCESSION||r->town<0||r->town>=TOWNS||r->winner<0||r->winner>=FACTIONS||r->other<0||r->other>=FACTIONS||
     r->prisoners<0||r->prisoners>2||!isfinite(r->x)||!isfinite(r->y)||!isfinite(r->time))return 0;
 }
 return 1;
}
void politics_tick(float dt){
 if(!L.version)return;
 L.pulse+=dt;
 if(L.pulse<1)return;
 float step=fminf(5,L.pulse);
 L.pulse=0;
 if(g.war.version){war_add_residents();war_tick_labor(step);}
 for(int t=0;t<TOWNS;t++){
  TownPolitics *p=&L.towns[t];
  int owner=g.townOwner[t],elite;
  int first=founder(t,&elite);
  if(p->stage==2&&p->winner!=owner){
   p->winner=owner;
   p->policy=owner==MONKS?1:owner==UNION||owner==SCAVENGERS?3:2;
   seat_heir(t,heir_candidate(t,owner));
  }
  if(p->stage==0&&owner>FREE&&first>=0&&g.actors[first].dead&&g.actors[first].faction==owner)
   succession_begin(t,elite);
  if(p->stage==2){
   int incumbent=p->leader-1;
   if(incumbent>=0){
    g.actors[incumbent].voice=ROLE_SEATED_LEADER;
    if(g.actors[incumbent].dead&&owner>FREE)succession_begin(t,elite);
   }
   else if(owner>FREE&&(first>=0||p->age>=180)){
    int n=heir_candidate(t,owner);
    if(n<0)n=dispatch_heir(t,owner);
    if(n>=0){seat_heir(t,n);announce_heir(t,n);}
   }
  }
  if(p->stage!=1)continue;
  if(owner!=p->original){politics_occupy(t,owner);continue;}
  p->age+=step;
  p->pulse+=step;
  if(p->pulse>=30){
   p->pulse=0;
   if(g.stock[t][RATION]>0){g.stock[t][RATION]--;p->support[0]=bound(p->support[0]+1,0,100);}
   else p->support[0]=bound(p->support[0]-2,0,100);
   if(!guarded(t,p->original))p->support[1]=bound(p->support[1]+2,0,100);
  }
  if(p->age>=420||(p->age>=180&&abs(p->support[0]-p->support[1])>=30)){
   int faction=p->support[0]>=p->support[1]?p->original:p->rival;
   politics_occupy(t,faction);
  }
 }
}
float politics_price(int town,int item){
 if(!L.version||town<0||town>=TOWNS)return 1;
 TownPolitics*p=&L.towns[town];if(p->stage==1)return 1.15f;
 int f=g.townOwner[town];
 int policy=p->stage==2?p->policy:town<OLD_TOWNS?0:f==MONKS?1:f==UNION?3:2;
 if(policy==1&&(item==RATION||item==MEDKIT||item==WATER))return .85f;
 if(policy==2&&itemDefs[item].slot>=0)return 1.12f;
 if(policy==3&&itemDefs[item].slot<0)return .93f;return 1;
}
int politics_action(int t,int action){
 if(t<0||t>=TOWNS||!L.version)return 0;Actor*a=&g.actors[g.selected];TownPolitics*p=&L.towns[t];
 if(!conscious(a)||actor_inside(a)||distance(a->x,a->y,towns[t].x,towns[t].y)>24||p->stage!=1){log_msg("Visit the contested settlement to take part.");return 0;}
 if(action==0||action==1){if(p->pledge>=0){log_msg("Your pledge has already been recorded.");return 0;}if(g.money<200){log_msg("Backing a claimant costs 200 silver.");return 0;}g.money-=200;p->support[action]=bound(p->support[action]+30,0,100);p->pledge=action;g.rep[action?p->rival:p->original]+=5;g.rep[action?p->original:p->rival]-=5;log_msg("Your pledge strengthens the %s by 30 influence.",action?"challenger":"caretaker");return 1;}
 if(action==2){if(p->relief){log_msg("This relief pledge has already been delivered.");return 0;}if(!inventory_remove(a,RATION,4)){log_msg("Relief needs 4 trail rations.");return 0;}p->relief=1;g.stock[t][RATION]+=4;p->support[0]=bound(p->support[0]+12,0,100);g.prosperity[t]=bound(g.prosperity[t]+4,10,95);log_msg("Four rations delivered. The caretaker gains 12 influence.");return 1;}return 0;
}
void politics_open(int t){if(t<0||t>=TOWNS)return;ui.tradeTown=t;ui.screen=SCREEN_POLITICS;ui.selection=0;ui.menuReturn=0;}
static const char *outside_claimant(int town,int faction){
 static const char *names[TOWNS]={"Maren Vale","Jiro Kade","Edda Salt","Sister Moss","Ren Flint","Vera Silt","Tarn Reed","Nox Cairn","Iris Grey","Holt Ash","Sable Fen","Korin Stone","Dara Rusk","Rook Venn","Kest Thorn","Flint Wren","Hana Reed","Oren Kade","Sister Iri"};
 for(int i=0;i<TOWNS;i++){
  const char *name=names[(town+faction*3+i)%TOWNS];
  int taken=0;
  for(int j=0;j<g.actorCount;j++)if(!strcmp(g.actors[j].name,name)){taken=1;break;}
  if(!taken)return name;
 }
 static char name[16];
 snprintf(name,sizeof(name),"Heir %u",(unsigned)L.succession);
 return name;
}
const char *politics_claimant(int town,int faction){
 int n=heir_candidate(town,faction);
 return n>=0?g.actors[n].name:outside_claimant(town,faction);
}
const char *politics_policy_text(int f){return f==MONKS?"Relief: food, water and medicine -15%.":f==UNION||f==SCAVENGERS?"Trade: supply prices -7%.":"Military rule: equipment prices +12%.";}
void field_observe(int k){
 if(k<0||k>=POP_GROUPS)return;WorldEvent*e=&g.population.groups[k];if(!e->spawned)return;
 int visible=-1;for(int i=0;i<e->count;i++){int n=e->members[i];if(n>=0&&!population_hidden(n)&&g.actors[n].active&&!g.actors[n].dead&&depth_room(&g.actors[n])==depth_room(&g.actors[g.selected])&&depth_distance(&g.actors[g.selected],&g.actors[n])<14){visible=n;break;}}
 if(visible<0)return;
 FieldSighting*f=&g.fieldNotes.group[k];f->serial=e->serial;f->x=g.actors[visible].x;f->y=g.actors[visible].y;f->time=g.time;f->goal=L.groups[k].goal;f->faction=e->faction;f->type=e->type;f->town=0;
 float best=1e9f;for(int t=0;t<TOWNS;t++){float d=distance(f->x,f->y,towns[t].x,towns[t].y);if(d<best){best=d;f->town=t;}}
 char name[32]={0};snprintf(name,sizeof(name),"%.20s's group",e->reserve[0].name);memcpy(f->name,name,sizeof(name));
 g.population.state[k].seen=1;
}
int field_notes_valid(const Game*s){for(int k=0;k<POP_GROUPS;k++){const FieldSighting*f=&s->fieldNotes.group[k];if(!f->serial)continue;if(f->serial<0||f->goal<0||f->goal>=LIFE_GOALS||f->town<0||f->town>=TOWNS||f->faction<0||f->faction>=FACTIONS||f->type<0||f->type>=POP_TYPES||!isfinite(f->x)||!isfinite(f->y)||f->x<0||f->x>WORLD||f->y<0||f->y>WORLD||!isfinite(f->time)||f->time<0||!memchr(f->name,0,sizeof(f->name)))return 0;}return 1;}
int life_ledger_count(void){int count=0;for(int t=0;t<TOWNS;t++)if(L.towns[t].stage&&g.discovered[t])count++;for(int k=0;k<POP_GROUPS;k++)if(g.fieldNotes.group[k].serial)count++;return count;}
int life_ledger_item(int row){for(int t=0;t<TOWNS;t++)if(L.towns[t].stage&&g.discovered[t]&&row--==0)return POP_GROUPS+t;for(int k=0;k<POP_GROUPS;k++)if(g.fieldNotes.group[k].serial&&row--==0)return k;return -1;}
void life_ledger_label(int row,char*out,int size){int n=life_ledger_item(row);if(n<0){snprintf(out,size,"No sightings yet");return;}if(n>=POP_GROUPS){int t=n-POP_GROUPS;snprintf(out,size,"%s / %s",towns[t].name,L.towns[t].stage==1?"SUCCESSION":"GOVERNMENT");}else{FieldSighting*f=&g.fieldNotes.group[n];snprintf(out,size,"%s / %s",f->name,life_goal_name(f->goal));}}
void life_ledger_detail(int row,char*out,int size){int n=life_ledger_item(row);if(n<0){snprintf(out,size,"Meet travelers to record sightings. Reports retain the place you saw them; groups can move on.");return;}if(n>=POP_GROUPS){TownPolitics*p=&L.towns[n-POP_GROUPS];snprintf(out,size,"%s. %s",towns[n-POP_GROUPS].name,p->stage==1?"Visit the council to meet the claimants and choose whom to support.":politics_policy_text(g.townOwner[n-POP_GROUPS]));return;}
 FieldSighting*f=&g.fieldNotes.group[n];float dx=f->x-towns[f->town].x,dy=f->y-towns[f->town].y;const char*dir=fabsf(dx)>fabsf(dy)?(dx<0?"west":"east"):(dy<0?"north":"south");
 snprintf(out,size,"%s. Last seen %.0fm %s of %s, %.1f min ago: %s. Pin this sighting; they may have moved.",factionNames[f->faction],distance(f->x,f->y,towns[f->town].x,towns[f->town].y),dir,towns[f->town].name,fmaxf(0,g.time-f->time)/60,life_goal_name(f->goal));
}
int politics_input(Input in){
 if(ui.screen!=SCREEN_LIFE&&ui.screen!=SCREEN_POLITICS)return 0;int p=in.pressed;
 if(p&(KEY_CIRCLE|KEY_START)){ui.screen=SCREEN_WORLD;ui.menuReturn=0;return 1;}
 if(ui.screen==SCREEN_POLITICS&&L.towns[ui.tradeTown].stage!=1){if(p&KEY_CROSS)ui.screen=SCREEN_WORLD;return 1;}
 int count=ui.screen==SCREEN_POLITICS?4:life_ledger_count();if(count<1)count=1;
 if(p&KEY_UP)ui.selection=(ui.selection+count-1)%count;if(p&KEY_DOWN)ui.selection=(ui.selection+1)%count;
 if(p&KEY_CROSS){if(ui.screen==SCREEN_POLITICS){if(ui.selection==3)ui.screen=SCREEN_WORLD;else politics_action(ui.tradeTown,ui.selection);}else{int n=life_ledger_item(ui.selection);if(n>=POP_GROUPS)politics_open(n-POP_GROUPS);else if(n>=0){FieldSighting*f=&g.fieldNotes.group[n];ui.mapX=f->x;ui.mapY=f->y;ui.mapZoom=1;ui.screen=SCREEN_MAP;ui.menuReturn=0;}}}
 return 1;
}
int life_validate(const Game*s){
 const FrontierLife*l=&s->life;if(!l->version)return 1;if(l->version!=1||!isfinite(l->pulse)||l->pulse<0)return 0;
 int groups=s->population.version==2?POP_GROUPS:s->war.version?WAR_POP_GROUPS:OLD_POP_GROUPS;
 for(int k=0;k<groups;k++){const GroupLife*z=&l->groups[k];const WorldEvent*e=&s->population.groups[k];if(z->serial!=e->serial||z->goal<0||z->goal>=LIFE_GOALS||z->camp<0||z->camp>CAMP_ABANDONED||z->targetTown < -1||z->targetTown>=TOWNS||z->sourceTown < -1||z->sourceTown>=TOWNS||z->targetGroup < -1||z->targetGroup>=POP_GROUPS||z->baseline<0||z->baseline>POP_MEMBERS||z->losses<0||z->losses>POP_MEMBERS||z->layout<0||z->layout>4||z->ambushSprung<0||z->ambushSprung>2)return 0;
  if((z->goal==LIFE_RESUPPLY||z->goal==LIFE_REFUGE||z->goal==LIFE_REINFORCE||z->goal==LIFE_DELIVER||z->goal==LIFE_RAID)&&z->targetTown<0)return 0;
  const float f[]={z->decision,z->clock,z->emptyClock,z->travelClock,z->stalled,z->lastX,z->lastY};for(int j=0;j<7;j++)if(!isfinite(f[j]))return 0;for(int j=0;j<3;j++)if(z->cargo[j]<0||z->cargo[j]>99)return 0;
  int used[POP_MEMBERS]={0};for(int i=0;i<POP_MEMBERS;i++){int j=z->carry[i];if(j < -1||j>=e->count||(j>=0&&(i>=e->count||i==j||z->carry[j]>=0||used[j]++||(s->population.state[k].detached&((1<<i)|(1<<j))))))return 0;}
 }
 for(int t=0;t<TOWNS;t++){const TownPolitics*p=&l->towns[t];if(p->stage<0||p->stage>2||p->elite < -1||p->elite>=ELITES||p->pledge < -1||p->pledge>1||p->original<0||p->original>=FACTIONS||p->rival<0||p->rival>=FACTIONS||p->winner<0||p->winner>=FACTIONS||p->leader<0||p->leader>s->actorCount||p->policy<0||p->policy>3||!isfinite(p->age)||p->age<0||!isfinite(p->pulse)||p->pulse<0)return 0;for(int j=0;j<2;j++)if(p->support[j]<0||p->support[j]>100)return 0;}
 return 1;
}
#undef L
