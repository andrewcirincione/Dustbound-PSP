#include "game.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int conscious(const Actor*);
#define L (g.life)
static int bound(int x,int lo,int hi){return x<lo?lo:x>hi?hi:x;}
static int guarded(int t,int faction){int n=0;for(int i=0;i<g.actorCount;i++){Actor*a=&g.actors[i];if(!a->squad&&!population_owned(i)&&a->active&&!a->dead&&a->unconscious<=0&&a->kind==GUARD&&a->homeTown==t&&a->faction==faction)n++;}return n;}
static void notice(int t,const char *event){snprintf(g.worldNews,sizeof(g.worldNews),"%s: %s",towns[t].name,event);if(g.discovered[t]||town_near(g.actors[g.selected].x,g.actors[g.selected].y,25)==t)log_warn("%s",g.worldNews);}
void politics_begin(int elite){
 if(!L.version||elite<0||elite>=6)return;int t=eliteDefs[elite].town;TownPolitics*p=&L.towns[t];if(p->stage)return;
 p->stage=1;p->elite=elite;p->original=g.townOwner[t];p->pledge=-1;p->winner=0;p->age=0;p->support[0]=40;p->support[1]=28;
 int rival=p->original==UNION?SHOGUN:p->original==SHOGUN?UNION:p->original==HOSTILE?UNION:p->original==OUTLAWS?SCAVENGERS:OUTLAWS;p->rival=rival;
 for(int k=0;k<POP_GROUPS;k++){WorldEvent*e=&g.population.groups[k];if(e->type==POP_ARMY&&e->faction==rival&&distance(e->x,e->y,towns[t].x,towns[t].y)<180){p->support[1]+=8;break;}}
 notice(t,"a caretaker and a rival claimant contest the vacant seat");
}
static void appoint(int t,int faction){
 TownPolitics*p=&L.towns[t];p->leader=0;
 for(int n=0;n<g.actorCount;n++){Actor*a=&g.actors[n];if(a->squad||!a->active||a->dead||g.society.freed[n]||merc_slot(a)>=0||a->kind!=GUARD||a->homeTown!=t||population_owned(n)||civil_elite_index(n)>=0||depth_named(n)>=0||regional_named(n))continue;
  p->leader=n+1;snprintf(a->name,sizeof(a->name),"%s",politics_claimant(t,faction));a->voice=ROLE_MARSHAL;break;
 }
}
void politics_occupy(int t,int faction){
 if(t<0||t>=TOWNS||faction<=FREE||faction>=FACTIONS)return;int old=g.townOwner[t];g.townOwner[t]=faction;
 for(int n=0;n<g.actorCount;n++){Actor*a=&g.actors[n];if(!a->squad&&!a->dead&&!population_owned(n)&&a->homeTown==t&&a->faction==old&&!g.society.freed[n]&&merc_slot(a)<0)a->faction=faction;}
 for(int b=0;b<g.buildingCount;b++)if(g.buildings[b].owner<0&&town_near(g.buildings[b].x,g.buildings[b].y,20)==t)g.buildings[b].faction=faction;
 TownPolitics*p=&L.towns[t];p->stage=2;p->winner=faction;p->policy=faction==MONKS?1:faction==UNION||faction==SCAVENGERS?3:2;appoint(t,faction);
 g.prosperity[t]=bound(g.prosperity[t]+(faction==old?4:-5),10,95);L.succession++;notice(t,faction==old?"the caretaker has secured the succession":"a new government controls the settlement");
}
void politics_tick(float dt){
 if(!L.version)return;L.pulse+=dt;if(L.pulse<1)return;float step=fminf(5,L.pulse);L.pulse=0;
 for(int e=0;e<6;e++){int n=g.civil.eliteActor[e]-1;if(n>=0&&g.actors[n].dead)politics_begin(e);}
 for(int t=0;t<TOWNS;t++){TownPolitics*p=&L.towns[t];if(p->stage==2&&p->winner!=g.townOwner[t]){p->winner=g.townOwner[t];p->policy=p->winner==MONKS?1:p->winner==UNION||p->winner==SCAVENGERS?3:2;appoint(t,p->winner);}if(p->stage!=1)continue;if(g.townOwner[t]!=p->original){politics_occupy(t,g.townOwner[t]);continue;}p->age+=step;p->pulse+=step;
  if(p->pulse>=30){p->pulse=0;if(g.stock[t][RATION]>0){g.stock[t][RATION]--;p->support[0]=bound(p->support[0]+1,0,100);}else p->support[0]=bound(p->support[0]-2,0,100);
   if(!guarded(t,p->original))p->support[1]=bound(p->support[1]+2,0,100);
  }
  if(p->age>=420||(p->age>=180&&abs(p->support[0]-p->support[1])>=30)){int faction=p->support[0]>=p->support[1]?p->original:p->rival;politics_occupy(t,faction);}
 }
}
float politics_price(int town,int item){
 if(!L.version||town<0||town>=TOWNS)return 1;TownPolitics*p=&L.towns[town];if(p->stage==1)return 1.15f;if(p->stage!=2)return 1;
 if(p->policy==1&&(item==RATION||item==MEDKIT||item==WATER))return .85f;
 if(p->policy==2&&itemDefs[item].slot>=0)return 1.12f;
 if(p->policy==3&&itemDefs[item].slot<0)return .93f;return 1;
}
int politics_action(int t,int action){
 if(t<0||t>=TOWNS||!L.version)return 0;Actor*a=&g.actors[g.selected];TownPolitics*p=&L.towns[t];
 if(!conscious(a)||actor_inside(a)||distance(a->x,a->y,towns[t].x,towns[t].y)>24||p->stage!=1){log_msg("Visit the contested settlement to take part.");return 0;}
 if(action==0||action==1){if(p->pledge>=0){log_msg("Your pledge has already been recorded.");return 0;}if(g.money<200){log_msg("Backing a claimant costs 200 silver.");return 0;}g.money-=200;p->support[action]=bound(p->support[action]+30,0,100);p->pledge=action;g.rep[action?p->rival:p->original]+=5;g.rep[action?p->original:p->rival]-=5;log_msg("Your pledge strengthens the %s by 30 influence.",action?"challenger":"caretaker");return 1;}
 if(action==2){if(p->relief){log_msg("This relief pledge has already been delivered.");return 0;}if(!inventory_remove(a,RATION,4)){log_msg("Relief needs 4 trail rations.");return 0;}p->relief=1;g.stock[t][RATION]+=4;p->support[0]=bound(p->support[0]+12,0,100);g.prosperity[t]=bound(g.prosperity[t]+4,10,95);log_msg("Four rations delivered. The caretaker gains 12 influence.");return 1;}return 0;
}
void politics_open(int t){if(t<0||t>=TOWNS)return;ui.tradeTown=t;ui.screen=SCREEN_POLITICS;ui.selection=0;ui.menuReturn=0;}
const char *politics_claimant(int town,int faction){
 static const char *names[TOWNS]={"Maren Vale","Jiro Kade","Edda Salt","Sister Moss","Ren Flint","Vera Silt","Tarn Reed","Nox Cairn","Iris Grey","Holt Ash","Sable Fen","Korin Stone","Dara Rusk","Rook Venn","Kest Thorn","Flint Wren"};
 return names[(town+faction*3)%TOWNS];
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
void life_ledger_label(int row,char*out,int size){int n=life_ledger_item(row);if(n<0){snprintf(out,size,"No sightings yet");return;}if(n>=POP_GROUPS){int t=n-POP_GROUPS;snprintf(out,size,"%s / %s",towns[t].name,L.towns[t].stage==1?"SUCCESSION":"NEW GOVERNMENT");}else{FieldSighting*f=&g.fieldNotes.group[n];snprintf(out,size,"%s / %s",f->name,life_goal_name(f->goal));}}
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
 for(int k=0;k<POP_GROUPS;k++){const GroupLife*z=&l->groups[k];const WorldEvent*e=&s->population.groups[k];if(z->serial!=e->serial||z->goal<0||z->goal>=LIFE_GOALS||z->camp<0||z->camp>CAMP_ABANDONED||z->targetTown < -1||z->targetTown>=TOWNS||z->sourceTown < -1||z->sourceTown>=TOWNS||z->targetGroup < -1||z->targetGroup>=POP_GROUPS||z->baseline<0||z->baseline>POP_MEMBERS||z->losses<0||z->losses>POP_MEMBERS||z->layout<0||z->layout>4||z->ambushSprung<0||z->ambushSprung>2)return 0;
  if((z->goal==LIFE_RESUPPLY||z->goal==LIFE_REFUGE||z->goal==LIFE_REINFORCE||z->goal==LIFE_DELIVER||z->goal==LIFE_RAID)&&z->targetTown<0)return 0;
  const float f[]={z->decision,z->clock,z->emptyClock,z->travelClock,z->stalled,z->lastX,z->lastY};for(int j=0;j<7;j++)if(!isfinite(f[j]))return 0;for(int j=0;j<3;j++)if(z->cargo[j]<0||z->cargo[j]>99)return 0;
  int used[POP_MEMBERS]={0};for(int i=0;i<POP_MEMBERS;i++){int j=z->carry[i];if(j < -1||j>=e->count||(j>=0&&(i>=e->count||i==j||z->carry[j]>=0||used[j]++||(s->population.state[k].detached&((1<<i)|(1<<j))))))return 0;}
 }
 for(int t=0;t<TOWNS;t++){const TownPolitics*p=&l->towns[t];if(p->stage<0||p->stage>2||p->elite < -1||p->elite>=6||p->pledge < -1||p->pledge>1||p->original<0||p->original>=FACTIONS||p->rival<0||p->rival>=FACTIONS||p->winner<0||p->winner>=FACTIONS||p->leader<0||p->leader>s->actorCount||p->policy<0||p->policy>3||!isfinite(p->age)||p->age<0||!isfinite(p->pulse)||p->pulse<0)return 0;for(int j=0;j<2;j++)if(p->support[j]<0||p->support[j]>100)return 0;}
 return 1;
}
#undef L
