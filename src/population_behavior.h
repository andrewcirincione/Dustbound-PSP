/* Population decisions share the same actors, supplies and casualties online/offline. */
#define L (g.life)
static const int supply_items[3]={RATION,MEDKIT,PARTS};
const char *life_goal_name(int goal){static const char*n[]={"traveling","tracking prey","lying in ambush","rescuing wounded","retreating","seeking supplies","returning to camp","seeking refuge","settled","reinforcing","delivering supplies","raiding","recovering"};return goal>=0&&goal<LIFE_GOALS?n[goal]:"traveling";}
int life_group_goal(int k){return k>=0&&k<POP_GROUPS&&L.version?L.groups[k].goal:LIFE_ROAM;}
static int life_alive(int k){int count=0;for(int i=0;i<P.groups[k].count;i++){Actor*a=member(k,i);count+=a&&!a->dead;}return count;}
static int life_town(int k,int safe,int avoid){
 WorldEvent*e=&P.groups[k];int found=-1;float best=1e9f;
 for(int t=0;t<TOWNS;t++){int f=g.townOwner[t];if(t==avoid)continue;
  if(safe){if(f==HOSTILE||f==OUTLAWS||f==WILDLIFE)continue;}else if(f!=e->faction&&!(e->faction==SCAVENGERS&&(f==UNION||f==MONKS))&&!(e->faction==FREE&&f!=HOSTILE&&f!=WILDLIFE))continue;
  float d=distance(e->x,e->y,towns[t].x,towns[t].y);if(d<best){best=d;found=t;}}
 return found;
}
static void life_target(int k,int goal,int t){GroupLife*z=&L.groups[k];z->goal=goal;z->targetTown=t;z->clock=0;z->travelClock=0;z->stalled=0;if(t>=0)set_destination(k,t);}
static void life_layout(int k){
 WorldEvent*e=&P.groups[k];PopulationState*m=&P.state[k];GroupLife*z=&L.groups[k];
 /* Separate footprints: scattered refugees, hidden smugglers, a hunter arc and ordered trading yards. */
 static const float layouts[5][5][2]={{{0,0},{-3,3},{0,4},{3,3},{4,-2}},{{0,0},{-3,1},{1,4},{4,2},{-4,-1}},{{0,0},{-4,1},{-1,4},{3,4},{3,-3}},{{0,0},{-3,-1},{-1,3},{3,1},{-5,3}},{{0,0},{-3,3},{1,3},{4,3},{4,-3}}};
 int style=e->type==POP_REFUGEES||e->type==POP_WOUNDED?1:e->type==POP_HUNTERS?2:e->type==POP_SMUGGLERS||e->type==POP_AMBUSH?3:4;z->layout=style;
 for(int j=0;j<5;j++){float x=m->siteX+layouts[style][j][0],y=m->siteY+layouts[style][j][1];clear_position(&x,&y);m->offsetX[j]=x-m->siteX;m->offsetY[j]=y-m->siteY;}
}
static void life_seed(int k){
 if(!L.version)return;WorldEvent*e=&P.groups[k];GroupLife*z=&L.groups[k];memset(z,0,sizeof(*z));z->serial=e->serial;z->targetGroup=-1;z->targetTown=z->sourceTown=-1;z->baseline=life_alive(k);z->lastX=e->x;z->lastY=e->y;
 for(int i=0;i<POP_MEMBERS;i++)z->carry[i]=-1;
 z->goal=e->type==POP_AMBUSH?LIFE_AMBUSH:LIFE_ROAM;life_layout(k);
}
void life_init(void){if(L.version)return;memset(&L,0,sizeof(L));L.version=1;for(int k=0;k<POP_GROUPS;k++)life_seed(k);for(int t=0;t<TOWNS;t++){L.towns[t].elite=-1;L.towns[t].pledge=-1;}}
static void life_announce(int k,const char*line){if(observers(P.groups[k].x,P.groups[k].y)<28)log_msg("%s: %s",population_name(k),line);}
static int life_threat(int k, float reach) {
    const WorldEvent *group = &P.groups[k];
    int found = -1;
    for (int j = 0; j < POP_GROUPS; j++) {
        const WorldEvent *other = &P.groups[j];
        if (j == k || fabsf(group->x-other->x) >= reach ||
            fabsf(group->y-other->y) >= reach || !enemies(group,other))
            continue;
        float d = distance(group->x,group->y,other->x,other->y);
        if (d < reach && standing(j)) {
            reach = d;
            found = j;
        }
    }
    return found;
}
static int life_bandage(int k,int patient,int medic){
 Actor*a=member(k,patient),*b=member(k,medic);if(!a||!b||a->dead||fatal_injury(a)||b->dead||b->unconscious>0||!inventory_remove(b,MEDKIT,1))return 0;
 injury_dress(a,member_depth(k,patient));gain(b,MEDICINE,24);L.groups[k].rescues++;L.rescues++;return 1;
}
static void life_drop(int k,int i){
 GroupLife*z=&L.groups[k];int j=z->carry[i];if(j<0)return;Actor*a=member(k,i),*b=member(k,j);if(P.groups[k].spawned){if(a)a->carrying=-1;if(b)b->carriedBy=-1;}if(a&&b){b->x=a->x+.6f;b->y=a->y+.4f;clear_position(&b->x,&b->y);}z->carry[i]=-1;
}
static int life_carried(int k,int patient){for(int i=0;i<P.groups[k].count;i++)if(L.groups[k].carry[i]==patient)return 1;return 0;}
static int life_casualties(int k,float dt){
 WorldEvent*e=&P.groups[k];PopulationState*m=&P.state[k];GroupLife*z=&L.groups[k];int alive=0,down=0,dead=0,hurt=0;
 for(int i=0;i<e->count;i++){Actor*a=member(k,i);if(!a)continue;if(a->dead){dead++;continue;}alive++;down+=a->unconscious>0||a->health[CHEST]<=0||a->health[HEAD]<=0;hurt+=a->bleed>.01f;}
 if(!alive){m->phase=POP_RUINS;e->presence+=dt;return 1;}e->presence=0;
 if(dead>z->losses){z->losses=dead;z->clock=12;z->goal=LIFE_RECOVER;life_announce(k,"regrouping after casualties");}
 for(int i=0;i<e->count;i++)if(z->carry[i]>=0){Actor*a=member(k,i),*b=member(k,z->carry[i]);if(!a||!b||a->dead||a->unconscious>0||b->dead||(b->unconscious<=0&&b->health[CHEST]>15&&b->health[HEAD]>15))life_drop(k,i);}
 int threat = life_threat(k, 24), able = standing(k);
 int routed = threat >= 0 && dead + down >= 2 &&
              able * 3 < z->baseline && standing(threat) > able * 2;
 if((down||hurt)&&standing(k)>0){
  for(int j=0;j<e->count;j++){Actor*b=member(k,j);if(!b||b->dead||(b->bleed<=.01f&&b->unconscious<=0))continue;
   int carried=0;for(int i=0;i<e->count;i++)carried|=z->carry[i]==j;if(carried)continue;
   for(int i=0;i<e->count;i++){Actor*a=member(k,i);if(!a||a->dead||a->unconscious>0||member_depth(k,i)->captive||z->carry[i]>=0||life_carried(k,i)||a->carrying>=0||a->carriedBy>=0)continue;
    if(e->spawned&&distance(a->x,a->y,b->x,b->y)>TALK_REACH)continue;
    if(b->bleed>.01f||b->health[CHEST]<20)life_bandage(k,j,i);
    if(i!=j&&b->unconscious>0){life_drop(k,j);z->carry[i]=j;if(e->spawned){a->carrying=e->members[j];b->carriedBy=e->members[i];}z->goal=LIFE_RESCUE;}
    break;
   }
  }
 }
 if(routed && able && z->goal != LIFE_RECOVER){
  if(z->goal!=LIFE_RETREAT){int t=life_town(k,e->faction!=HOSTILE&&e->faction!=OUTLAWS,-1);if(t>=0){life_target(k,LIFE_RETREAT,t);z->camp=CAMP_AWAY;life_announce(k,"withdrawing with survivors");}}
 }
 if(z->goal==LIFE_RESCUE){if(down==0){z->goal=LIFE_RECOVER;z->clock=35;}else if(threat<0){m->phase=POP_REST;return 1;}}
 if(z->goal==LIFE_RECOVER){m->phase=POP_REST;z->clock-=dt;if(z->clock>0)return 1;z->goal=LIFE_ROAM;}
 if(!standing(k)){m->phase=POP_REST;return 1;}return 0;
}
static int life_take_pack(Actor*from,Actor*to,int item,int limit){int got=0;for(int p=0;p<PACK&&got<limit;p++)if(from->pack[p].item==item){int n=from->pack[p].qty;if(n>limit-got)n=limit-got;if(inventory_transfer(from,to,p,n))got+=n;}return got;}
static int life_harvest_prey(int k,int prey){
 GroupLife*z=&L.groups[k];WorldEvent*e=&P.groups[k],*p=&P.groups[prey];int taken=0,pending=0;
 for(int j=0;j<p->count;j++){Actor*dead=member(prey,j);if(!dead||(!dead->dead&&dead->unconscious<=0)||(!inventory_count(dead,MEAT)&&!inventory_count(dead,HIDE)))continue;
  for(int i=0;i<e->count;i++){Actor*a=member(k,i);if(!a||a->dead||a->unconscious>0)continue;if(e->spawned&&distance(a->x,a->y,dead->x,dead->y)>TALK_REACH){pending=1;continue;}if(!dead->dead&&p->type==POP_BEASTS){dead->dead=1;dead->unconscious=0;P.deaths++;}if(!dead->dead)continue;taken+=life_take_pack(dead,a,MEAT,3);taken+=life_take_pack(dead,a,HIDE,2);}}
 if(taken){z->hunts++;L.hunts++;life_announce(k,"prey butchered; returning with meat and hides");}return pending;
}
static int life_enemy_town(int k){
 WorldEvent *e=&P.groups[k];int found=-1;float score=-1e9f;
 for(int t=0;t<TOWNS;t++){
  int f=g.townOwner[t];
  if(e->faction==HOSTILE?(f==HOSTILE||f==FREE||f==SCAVENGERS||f==WILDLIFE):!war_rivals(e->faction,f))continue;
  if((t==0&&g.day==1)||(t==2&&g.regions.outcome[0])||(t==12&&g.regions.outcome[2]))continue;
  if(g.war.heldSince[t]>0&&g.time-g.war.heldSince[t]<650)continue;
  float value=(100-g.prosperity[t])*.45f-distance(e->x,e->y,towns[t].x,towns[t].y)*.23f+(t>=OLD_TOWNS?14:0);
  if(value>score){score=value;found=t;}
 }
 return found;
}
static void life_army_mission(int k){
 WorldEvent*e=&P.groups[k];GroupLife*z=&L.groups[k];int t=-1,base=-1;float best=-1e9f,nearest=1e9f;
 for(int j=0;j<TOWNS;j++)if(g.townOwner[j]==e->faction){
  float d=distance(e->x,e->y,towns[j].x,towns[j].y);
  float need=(100-g.prosperity[j])*2+(L.towns[j].stage==1?80:0)-d*.12f;
  if(need>best){best=need;t=j;}
  if(d<nearest&&g.prosperity[j]>=55&&g.stock[j][RATION]>=4){nearest=d;base=j;}
 }
 int side=war_faction_index(e->faction);
 if(base>=0&&standing(k)>=7&&(side<0||g.time>=g.war.nextRaid[side])){
  int attack=life_enemy_town(k);
  if(attack>=0){
   if(side>=0){g.war.nextRaid[side]=g.time+780+(e->serial%6)*35;g.stock[base][RATION]-=4;}
   life_target(k,LIFE_RAID,attack);return;
  }
 }
 if(t>=0&&g.stock[t][RATION]<4){int source=-1;float d=1e9f;for(int j=0;j<TOWNS;j++)if(j!=t&&g.townOwner[j]==e->faction&&g.stock[j][RATION]>=8){float n=distance(e->x,e->y,towns[j].x,towns[j].y);if(n<d){d=n;source=j;}}if(source>=0){z->sourceTown=t;life_target(k,LIFE_RESUPPLY,source);return;}}
 if(t>=0)life_target(k,LIFE_REINFORCE,t);
}

static int town_defender(int n,int town,int faction){
 Actor *a=&g.actors[n];
 return !a->squad&&!living_owned(n)&&a->kind==GUARD&&a->homeTown==town&&a->faction==faction&&
        !g.depth.actors[n].captive&&a->active&&!a->dead&&a->unconscious<=0&&a->carriedBy<0;
}
static int post_occupier(int k,int town){
 WorldEvent *e=&P.groups[k];PopulationState *m=&P.state[k];GroupLife *z=&L.groups[k];
 for(int i=0;i<e->count;i++){
  Actor *a=member(k,i);
  if(!a||!conscious(a)||member_depth(k,i)->captive||a->carrying>=0||life_carried(k,i)||z->carry[i]>=0)continue;
  int n=e->members[i];
  if(!e->spawned){
   for(int j=0;j<g.actorCount;j++)if(g.living.owned[j]<0){n=j;break;}
   if(n<0){if(g.actorCount>=MAX_ACTORS)return -1;n=g.actorCount++;}
   social_load(k,i,n);
   g.actors[n]=*a;
   g.depth.actors[n]=*member_depth(k,i);
   a=&g.actors[n];
   war_yard(town,&a->x,&a->y);
   a->y-=2;
   clear_position(&a->x,&a->y);
  }
  m->detached|=1<<i;e->members[i]=-1;g.living.owned[n]=0;
  g.living.anger[n]=0;g.living.command[n]=-1;
  a->homeTown=town;a->homeX=a->tx=a->x;a->homeY=a->ty=a->y;
  a->target=a->attackTarget=-1;a->attackPhase=a->taskKind=0;a->routeLen=a->routePos=0;
  return n;
 }
 return -1;
}
static int life_siege(int k,int t,float dt){
 WorldEvent *e=&P.groups[k];PopulationState *m=&P.state[k];GroupLife *z=&L.groups[k];
 int owner=g.townOwner[t],first=-1,defenders=0;
 for(int n=0;n<g.actorCount;n++)if(town_defender(n,t,owner)){if(first<0)first=n;defenders++;}
 if(!defenders){
  if(e->spawned){
   int present=0;
   for(int i=0;i<e->count;i++){
    Actor *a=member(k,i);
    if(a&&conscious(a)&&distance(a->x,a->y,towns[t].x,towns[t].y)<16){present=1;break;}
   }
   if(!present)return 0;
  }
  int guard=g.war.version&&war_rivals(e->faction,owner)?post_occupier(k,t):-1;
  war_conquer(t,e->faction,guard);
  z->missionDone++;L.missions++;z->goal=LIFE_RECOVER;z->clock=240;
  return 1;
 }
 m->phase=POP_FIGHT;m->clock=0;
 if(e->spawned)return 1;
 z->clock+=dt;
 if(z->clock<5)return 1;
 z->clock=0;
 war_report_clash(e->faction,owner,e->x,e->y);
 Actor *d=&g.actors[first];
 d->health[CHEST]-=fminf(22,standing(k)*1.7f/(1+itemDefs[d->gear[1]].armor*.02f));
 if(d->health[CHEST]<=0){d->unconscious=180;d->target=-1;}
 for(int i=0;i<e->count;i++){
  Actor *a=member(k,(i+e->serial)%e->count);
  if(!a||a->dead||a->unconscious>0)continue;
  a->health[CHEST]-=fminf(18,5+defenders*1.5f);
  if(a->health[CHEST]<=0)a->unconscious=120;
  break;
 }
 return 1;
}
static void life_choose(int k){
 WorldEvent*e=&P.groups[k];GroupLife*z=&L.groups[k];
 if(z->goal==LIFE_RETREAT||z->goal==LIFE_RESCUE||z->goal==LIFE_RECOVER||z->goal==LIFE_RESUPPLY||z->goal==LIFE_RETURN||z->goal==LIFE_REFUGE||z->goal==LIFE_SETTLED||z->goal==LIFE_DELIVER||z->goal==LIFE_REINFORCE||z->goal==LIFE_RAID)return;
 if(e->type==POP_ARMY){life_army_mission(k);return;}
 if(e->type==POP_REFUGEES||e->type==POP_WOUNDED){if(e->age>240&&standing(k)==life_alive(k)){int t=life_town(k,1,-1);if(t>=0){z->camp=CAMP_PACKING;life_target(k,LIFE_REFUGE,t);z->clock=12;life_announce(k,"packing for a safer settlement");}}return;}
 if(camp_type(k)&&(z->emptyClock>90||(e->type==POP_SMUGGLERS&&(g.dayTime>19||g.dayTime<5)&&e->age>120))){int t=life_town(k,0,-1);if(t>=0){z->camp=CAMP_PACKING;life_target(k,LIFE_RESUPPLY,t);z->clock=12;life_announce(k,"packing up for a supply run");}return;}
 if(e->type==POP_HUNTERS){if(z->goal==LIFE_HUNT)return;int prey=-1;float d=70;for(int j=0;j<POP_GROUPS;j++)if(P.groups[j].type==POP_BEASTS&&life_alive(j)){float n=distance(e->x,e->y,P.groups[j].x,P.groups[j].y);if(n<d){d=n;prey=j;}}if(prey>=0){z->goal=LIFE_HUNT;z->targetGroup=prey;z->targetSerial=P.groups[prey].serial;z->clock=0;}return;}
 if(e->type==POP_AMBUSH&&!z->ambushSprung)return;
 if(e->type==POP_PATROL){int threat=life_threat(k,65);if(threat>=0){z->targetGroup=threat;z->targetSerial=P.groups[threat].serial;z->goal=LIFE_HUNT;}return;}
 if(e->type==POP_RAIDERS){int victim=-1;float d=80;for(int j=0;j<POP_GROUPS;j++)if(P.groups[j].type==POP_MERCHANTS||P.groups[j].type==POP_REFUGEES){float n=distance(e->x,e->y,P.groups[j].x,P.groups[j].y);if(life_alive(j)&&n<d){d=n;victim=j;}}if(victim>=0){z->targetGroup=victim;z->targetSerial=P.groups[victim].serial;z->goal=LIFE_HUNT;}return;}
}
static int life_player_near_site(int k){PopulationState*m=&P.state[k];for(int n=0;n<g.actorCount;n++)if(g.actors[n].squad&&g.actors[n].active&&observers(m->siteX,m->siteY)<12&&g.actors[n].taskKind==3)return 1;return 0;}
static void life_arrive(int k){
 WorldEvent*e=&P.groups[k];PopulationState*m=&P.state[k];GroupLife*z=&L.groups[k];int t=z->targetTown;
 m->phase=POP_REST;z->arrivals++;P.arrivals++;z->clock=60;
 if(z->goal==LIFE_RETREAT){for(int i=0;i<e->count;i++)life_drop(k,i);z->baseline=life_alive(k);z->goal=LIFE_RECOVER;z->clock=90;return;}
 if(z->goal==LIFE_REFUGE){if(!z->settled){L.towns[t].refugees+=life_alive(k);L.settled+=life_alive(k);z->settled=1;g.prosperity[t]=bounded(g.prosperity[t]+2,10,95);for(int i=0;i<e->count;i++){Actor*a=member(k,i);if(a&&!a->dead){a->homeTown=t;a->kind=RECRUIT;a->faction=g.townOwner[t];}}life_announce(k,"reached refuge; survivors may now be recruited");}z->goal=LIFE_SETTLED;z->camp=CAMP_ABANDONED;return;}
 if(z->goal==LIFE_RESUPPLY){
  int fetched=0;for(int v=0;v<3;v++){int want=e->type==POP_ARMY?(v?0:6):(v?2:6)-m->stock[v];if(want<0)want=0;int n=g.stock[t][supply_items[v]];if(n>want)n=want;g.stock[t][supply_items[v]]-=n;if(e->type==POP_ARMY)z->cargo[v]+=n;else m->stock[v]+=n;fetched+=n;}
  if(fetched){z->supplies+=fetched;L.resupplies++;L.towns[t].shipments++;z->emptyClock=0;}
  if(e->type==POP_ARMY){life_target(k,LIFE_DELIVER,z->sourceTown);return;}
  /* Crew provisions come from the same stock. They are never conjured on reload. */
  for(int i=0;i<e->count;i++){Actor*a=member(k,i);if(!a||a->dead)continue;for(int v=0;v<2;v++)if(m->stock[v]>1&&!inventory_count(a,supply_items[v])&&inventory_add(a,supply_items[v],1))m->stock[v]--;}
  if(!m->cacheQty&&m->cacheItem==RATION&&m->stock[0]>2){m->stock[0]--;m->cacheQty=1;}
  if(!fetched){z->camp=CAMP_ABANDONED;z->goal=LIFE_SETTLED;life_announce(k,"could not restock; abandoning the old camp");return;}
  float angle=(e->serial%16)*.392699f;float x=towns[t].x+cosf(angle)*38,y=towns[t].y+sinf(angle)*38;clear_position(&x,&y);m->siteX=x;m->siteY=y;m->targetX=x;m->targetY=y;z->goal=LIFE_RETURN;z->clock=0;z->relocations++;L.relocations++;return;
 }
 if(z->goal==LIFE_RETURN){z->goal=LIFE_ROAM;z->camp=CAMP_READY;z->clock=60;life_layout(k);life_announce(k,"camp established");return;}
 if(z->goal==LIFE_DELIVER){for(int v=0;v<3;v++){g.stock[t][supply_items[v]]+=z->cargo[v];z->cargo[v]=0;}L.towns[t].shipments++;z->missionDone++;L.missions++;z->goal=LIFE_RECOVER;z->clock=120;return;}
 if(z->goal==LIFE_REINFORCE){if(g.townOwner[t]==e->faction){TownPolitics*p=&L.towns[t];if(p->stage==1){int side=e->faction==p->original?0:1;p->support[side]=bounded(p->support[side]+12,0,100);p->reinforcements[side]++;}g.prosperity[t]=bounded(g.prosperity[t]+3,10,95);z->missionDone++;L.missions++;}z->goal=LIFE_RECOVER;z->clock=240;return;}
 if(e->type==POP_PRISONERS){z->goal=LIFE_SETTLED;z->settled=1;for(int i=e->count-2;i<e->count;i++){Actor*a=member(k,i);if(a&&!a->dead&&member_depth(k,i)->captive){a->prisonTown=e->town;a->prisonTime=300;}}return;}
 if(e->type==POP_PILGRIMS){for(int i=0;i<e->count;i++){Actor*a=member(k,i);if(a&&!a->dead&&inventory_remove(a,MEDKIT,1)){g.stock[e->town][MEDKIT]++;L.towns[e->town].shipments++;break;}}}
 int next=life_town(k,e->type==POP_PILGRIMS,e->town);if(next<0)next=nearest_town(e->x,e->y,e->town);e->origin=e->town;set_destination(k,next);z->goal=LIFE_RECOVER;z->clock=70+e->serial%80;
}
static void group_behavior(int k,float dt){
 WorldEvent*e=&P.groups[k];PopulationState*m=&P.state[k];GroupLife*z=&L.groups[k];e->age+=dt;m->clock-=dt;if(z->serial!=e->serial)life_seed(k);
 int targetThreat=life_threat(k,9);if(life_casualties(k,dt))return;
 if(z->goal==LIFE_AMBUSH){
  float trigger=observers(m->siteX,m->siteY);int target=life_threat(k,8);if((trigger<6&&e->spawned)||target>=0||z->ambushSprung){if(!z->ambushSprung){z->ambushSprung=1;z->clock=25;z->targetGroup=target;z->targetSerial=target>=0?P.groups[target].serial:0;life_announce(k,"the ambush closes in");}z->clock-=dt;m->phase=POP_FIGHT;if(z->clock<=0){z->goal=LIFE_ROAM;z->ambushSprung=2;}}else m->phase=POP_REST;
  for(int i=0;i<e->count;i++){Actor*a=member(k,i);if(a)a->sneaking=!z->ambushSprung;}return;
 }
 int fighting=0;for(int i=0;i<e->count;i++){Actor*a=member(k,i);if(a&&e->spawned&&a->target>=0)fighting=1;}
 if((fighting||targetThreat>=0)&&z->goal!=LIFE_RETREAT&&z->goal!=LIFE_RAID){m->phase=POP_FIGHT;m->clock=8;return;}
 if(m->phase==POP_FIGHT&&m->clock>0&&z->goal!=LIFE_RETREAT&&z->goal!=LIFE_RAID)return;
 if(camp_type(k)&&z->camp==CAMP_READY)for(int i=0;i<e->count;i++){Actor*a=member(k,i);if(a&&!a->dead&&a->hunger<40&&!inventory_count(a,RATION)){
   if(inventory_count(a,MEAT)>=2&&inventory_add(a,RATION,1))inventory_remove(a,MEAT,2);
   else if(m->stock[0]>0&&inventory_add(a,RATION,1))m->stock[0]--;
   else if(m->cacheItem==RATION&&m->cacheQty>0&&inventory_add(a,RATION,1))m->cacheQty--;
  }}
 if(camp_type(k)&&m->stock[0]<=1&&(m->cacheItem!=RATION||!m->cacheQty))z->emptyClock+=dt;else z->emptyClock=fmaxf(0,z->emptyClock-dt);
 z->decision-=dt;if(z->decision<=0){life_choose(k);z->decision=10;}
 if(z->camp==CAMP_PACKING){m->phase=POP_REST;if(life_player_near_site(k))return;z->clock-=dt;if(z->clock>0)return;z->camp=CAMP_AWAY;}
 if(z->goal==LIFE_SETTLED){m->phase=POP_REST;return;}
 if(z->goal==LIFE_HUNT){int prey=z->targetGroup;if(prey<0||P.groups[prey].serial!=z->targetSerial){z->goal=LIFE_ROAM;return;}
  m->targetX=P.groups[prey].x;m->targetY=P.groups[prey].y;z->clock+=dt;
  if(!standing(prey)&&distance(e->x,e->y,m->targetX,m->targetY)<8&&e->type==POP_HUNTERS){if(life_harvest_prey(k,prey)&&z->clock<240){m->phase=POP_REST;return;}z->clock=241;}
  if((!life_alive(prey)&&e->type!=POP_HUNTERS)||z->clock>240){m->targetX=m->siteX;m->targetY=m->siteY;z->goal=camp_type(k)?LIFE_RETURN:LIFE_ROAM;z->clock=0;}
 }
 if(camp_type(k)&&z->goal==LIFE_ROAM){m->phase=g.dayTime<6||g.dayTime>19?POP_REST:(int)(e->age/110)%3==0?POP_FORAGE:POP_REST;return;}
 if(z->goal==LIFE_RAID){
  int t=z->targetTown;
  if(t<0||g.townOwner[t]==e->faction||(g.war.heldSince[t]>0&&g.time-g.war.heldSince[t]<650)){z->goal=LIFE_ROAM;return;}
  m->targetX=towns[t].x;m->targetY=towns[t].y;clear_position(&m->targetX,&m->targetY);
  if(distance(e->x,e->y,towns[t].x,towns[t].y)<12&&life_siege(k,t,dt))return;
 }
 m->phase=POP_TRAVEL;
 float tx=m->targetX,ty=m->targetY;
 if(distance(e->x,e->y,tx,ty)<3){life_arrive(k);return;}
 if(e->type==POP_ARMY)army_ford_waypoint(e->x,e->y,&tx,&ty);
 float dx=tx-e->x,dy=ty-e->y,d=fmaxf(.01f,sqrtf(dx*dx+dy*dy));
 float speed=z->goal==LIFE_RETREAT?.9f:z->goal==LIFE_HUNT?1.05f:e->type==POP_ARMY?.55f:.75f;
 if(e->spawned){for(int i=0;i<e->count;i++){Actor*a=member(k,i);if(a&&!a->dead&&a->unconscious<=0){if(distance(a->x,a->y,e->x,e->y)>8)return;break;}}}
 float step=fminf(d,speed*dt),x=e->x+dx/d*step,y=e->y+dy/d*step;
 if(!can_walk(x,y)){int found=0;float angle=atan2f(dy,dx);for(int i=1;i<=12;i++){float turn=(i%2?1:-1)*((i+1)/2)*.35f;x=e->x+cosf(angle+turn)*step;y=e->y+sinf(angle+turn)*step;if(can_walk(x,y)){found=1;break;}}if(!found)return;}
 shift_group(k,x-e->x,y-e->y);z->travelClock+=dt;
 if(distance(e->x,e->y,z->lastX,z->lastY)<.08f)z->stalled+=dt;else z->stalled=0;z->lastX=e->x;z->lastY=e->y;
}
int population_tactics(int n,float dt){
 int owned=population_owned(n);if(!owned||!L.version)return 0;int k=owned-1;GroupLife*z=&L.groups[k];WorldEvent*e=&P.groups[k];Actor*a=&g.actors[n];int i=0;while(i<e->count&&e->members[i]!=n)i++;if(i>=e->count||a->squad||g.society.freed[n])return 0;
 if(z->goal==LIFE_RETREAT){a->target=-1;a->attackPhase=0;route_toward(a,e->x+(i%4-1)*1.8f,e->y+(i/4)*1.8f,dt);return 1;}
 if(z->goal==LIFE_AMBUSH&&!z->ambushSprung){float angle=i*6.283185f/e->count;route_toward(a,P.state[k].siteX+cosf(angle)*4,P.state[k].siteY+sinf(angle)*4,dt*.6f);a->sneaking=1;return 1;}
 if(z->goal==LIFE_AMBUSH&&z->ambushSprung==1&&z->clock>20){float x=g.actors[g.selected].x,y=g.actors[g.selected].y;if(z->targetGroup>=0&&P.groups[z->targetGroup].serial==z->targetSerial){x=P.groups[z->targetGroup].x;y=P.groups[z->targetGroup].y;}float side=i%2?1:-1;route_toward(a,x+side*3,y+(i%3-1)*2,dt);return 1;}
 if(life_threat(k,10)<0&&a->target<0){for(int j=0;j<e->count;j++){Actor*b=member(k,j);if(i==j||!b||b->dead||b->carriedBy>=0||(b->bleed<=.01f&&b->unconscious<=0))continue;if(inventory_count(a,MEDKIT)||b->unconscious>0){if(distance(a->x,a->y,b->x,b->y)>TALK_REACH)route_toward(a,b->x,b->y,dt);else{life_bandage(k,j,i);if(b->unconscious>0&&a->carrying<0&&!life_carried(k,i)){life_drop(k,j);z->carry[i]=j;a->carrying=e->members[j];b->carriedBy=n;}}return 1;}}}
 if(z->goal==LIFE_HUNT&&e->type==POP_HUNTERS&&z->targetGroup>=0&&P.groups[z->targetGroup].serial==z->targetSerial&&!standing(z->targetGroup)){WorldEvent*p=&P.groups[z->targetGroup];for(int j=0;j<p->count;j++){Actor*b=member(z->targetGroup,j);if(b&&(b->dead||b->unconscious>0)&&(inventory_count(b,MEAT)||inventory_count(b,HIDE))){route_toward(a,b->x,b->y,dt);return 1;}}}
 if(z->goal==LIFE_RAID&&z->targetTown>=0){int t=z->targetTown;if(distance(e->x,e->y,towns[t].x,towns[t].y)<30)for(int j=0;j<g.actorCount;j++)if(town_defender(j,t,g.townOwner[t])){Actor*b=&g.actors[j];if(depth_distance(a,b)>5){route_toward(a,b->x,b->y,dt);return 1;}break;}}
 return 0;
}
int population_hidden(int n){int k=population_owned(n)-1;if(k<0||!L.version)return 0;GroupLife*z=&L.groups[k];Actor*a=&g.actors[n];return z->goal==LIFE_AMBUSH&&!z->ambushSprung&&!a->dead&&observers(a->x,a->y)>5+level(&g.actors[g.selected],PERCEPTION)*.03f;}
#undef L
