/* Bounded simulation of an unending frontier. Only director-owned actors recycle. */
extern void init_actor(int,const char*,int,int,float,float,int);
extern int hostile(const Actor*,const Actor*);
#define LW (g.living)
static unsigned event_random(WorldEvent *e){unsigned x=e->seed;x^=x<<13;x^=x>>17;x^=x<<5;return e->seed=x?x:31;}
static const char *eventTypes[8]={"CARAVAN","RAIDING COLUMN","RELIEF APPEAL","LOST CACHE","STRANDED PILGRIM","HUNTING PACK","MARCHING ARMY","WARLORD RETINUE"};
static const char *eventMods[4]={"DUST-WORN","STARVING","IRONCLAD","VETERAN"};
int living_owned(int n){return n>=0&&n<MAX_ACTORS&&LW.owned[n]!=0;}
static int actor_index(const Actor*a){return (uintptr_t)a>=(uintptr_t)g.actors&&(uintptr_t)a<(uintptr_t)(g.actors+g.actorCount)?(int)(a-g.actors):-1;}
static int event_for(int n){return n>=0&&n<MAX_ACTORS&&LW.owned[n]>0&&LW.owned[n]<=EVENTS?LW.owned[n]-1:-1;}
void living_gear(Actor*a,int role,int tier,int n){
    tier=(int)limit(tier,0,4);role%=8;
    if (a->kind == RAIDER && tier <= 1) {
        static const int scraps[]={0,1,3,9,10,14,16,19,21,23};
        a->gear[0] = gear_id(scraps[(unsigned)n%10],0,(unsigned)n%23==0?1:0);
        a->gear[1] = n % 3 ? RAGS : NONE;
        a->gear[2] = a->gear[5] = NONE;
        a->gear[3] = n % 4 ? NONE : SANDALS;
        a->gear[4] = gear_id(68, 0, 0);
        memset(a->pack, 0, sizeof(a->pack));
        memset(a->stolen, 0, sizeof(a->stolen));
        raider_scraps(a,g.seed+(unsigned)n*37u);
        armor_wanderer(a,(unsigned)n*7919u,tier);
        return;
    }
    static const int weapons[8]={8,3,85,29,5,87,0,16},armor[8]={45,43,38,32,34,36,47,39};
    int q=tier>=3?3:tier,m=tier>=3?5:tier==2?3:1;
    a->gear[0]=gear_id(weapons[role],m,q);a->gear[1]=gear_id(armor[role],m,q);
    a->gear[2]=gear_id(48+role,m,q);a->gear[3]=gear_id(60+role,m,q);a->gear[4]=gear_id(68+role,m,q);
    if(tier==2)a->gear[role%2]=CATALOG_ITEMS+(role%2?8+role:role);
    if(tier>=3)a->gear[role>=4?1:0]=CATALOG_ITEMS+16+role;
    if(role==0&&tier<3)a->gear[0]=SCIMITAR_ITEMS+(tier*7+n)%24;
    if(role==1&&tier<3)a->gear[0]=RAPIER_ITEMS+(tier*7+n)%24;
    armor_wanderer(a,(unsigned)n*7919u,tier);
    inventory_add(a,RATION,1+tier);inventory_add(a,MEDKIT,1+tier/2);
    if(weapon_ranged(a->gear[0]))inventory_add(a,gear_family(a->gear[0])>=29?CARTRIDGE:BOLTS,12+tier*8);
    if(role==2)inventory_add(a,IRONBAR,3+tier);
    if(role==3)inventory_add(a,PARTS,1+tier);
    if(role==7)inventory_add(a,TONIC,1+tier);
    if(tier>=3)inventory_add(a,RELIC,tier-2);
    if(a->kind==BEAST){memset(a->gear,0,sizeof(a->gear));memset(a->pack,0,sizeof(a->pack));inventory_add(a,HIDE,2+tier);inventory_add(a,MEAT,2+tier);if(tier>=3)inventory_add(a,CATALOG_ITEMS+22,1);}
}
void living_loot(int n,int role,int tier){living_gear(&g.actors[n],role,tier,n);}
void living_title(int slot,char*out,int size){WorldEvent*e=&LW.events[slot];snprintf(out,size,"%s %s",eventMods[e->modifier],eventTypes[e->type]);}
void living_detail(int slot,char*out,int size){
    WorldEvent*e=&LW.events[slot];
    static const char *verbs[8]={"Escort to destination; stay close to earn pay.","Stop the column before it plunders the town.","Deliver supplies at the destination town.","Search the marked site for equipment.","Bandage the pilgrim; help them depart safely.","Hunt the pack before it strips local supplies.","Army advances on the destination garrison.","Defeat the warlord; signature equipment is worn."};
    if(e->status)snprintf(out,size,"%s. New incidents continue elsewhere.",e->status==1?"Resolved":"Opportunity passed");
    else if(e->type==2)snprintf(out,size,"Bring %d %s to %s. %d silver; improves local stock.",3+e->tier,itemDefs[e->modifier==1?RATION:e->modifier==2?PARTS:MEDKIT].name,towns[e->town].name,180+e->tier*80);
    else snprintf(out,size,"%s. %s",factionNames[e->faction],verbs[e->type]);
}
static int release_event(int slot){
    WorldEvent*e=&LW.events[slot];Actor*p=&g.actors[g.selected];
    for(int i=0;i<16;i++){int n=e->members[i];if(n>=0&&(g.actors[n].carriedBy>=0||g.actors[n].squad||g.society.freed[n]||g.depth.actors[n].captive||distance(p->x,p->y,g.actors[n].x,g.actors[n].y)<55))return 0;}
    for(int i=0;i<16;i++){int n=e->members[i];if(n<0)continue;
        g.actors[n].active=0;LW.owned[n]=-1;LW.anger[n]=0;
        for(int j=0;j<g.actorCount;j++){if(g.actors[j].target==n)g.actors[j].target=-1;if(g.actors[j].attackTarget==n){g.actors[j].attackTarget=-1;g.actors[j].attackPhase=0;}if(LW.command[j]==n)LW.command[j]=-1;}
        if(ui.attackFocus==n)ui.attackFocus=-1;
    }return 1;
}
void living_generate(int slot){
    WorldEvent*e=&LW.events[slot];if(e->serial&&!release_event(slot))return;
    memset(e,0,sizeof(*e));for(int i=0;i<16;i++)e->members[i]=-1;
    e->seed=wrand();e->serial=++LW.serial;
    int signature;
    for(int tries=0;;tries++){
        e->type=event_random(e)%8;e->modifier=event_random(e)%4;signature=e->type*4+e->modifier;
        int recent=0;for(int i=0;i<16;i++)if(LW.history[i]==signature)recent=1;
        if(!recent||tries>80)break;
    }
    LW.history[LW.historyPos++%16]=signature;LW.historyPos%=16;
    e->town=event_random(e)%TOWNS;e->origin=(e->town+1+event_random(e)%(TOWNS-1))%TOWNS;
    static const int banners[WAR_FACTIONS]={UNION,SHOGUN,MONKS};
    e->faction=e->type==1||e->type==7?HOSTILE:e->type==5?WILDLIFE:e->type==6?banners[event_random(e)%WAR_FACTIONS]:g.townOwner[e->town];
    if(e->type==0||e->type==4||e->type==2)e->faction=SCAVENGERS;
    if(e->type==6){
        for(int tries=0;tries<TOWNS;tries++){if(e->town>4&&war_rivals(e->faction,g.townOwner[e->town])&&!(e->town==12&&g.regions.outcome[2]))break;e->town=(e->town+1)%TOWNS;}
    }
    e->tier=(int)limit((e->town/4)+(g.day/12),0,3);if(e->type==7)e->tier=4;
    e->count=e->type==6?16:e->type==1?10:e->type==5?7:e->type==7?9:e->type==0?6:e->type==4?1:0;
    float u=.25f+(event_random(e)%50)/100.f;
    e->x=towns[e->origin].x+(towns[e->town].x-towns[e->origin].x)*u;
    e->y=towns[e->origin].y+(towns[e->town].y-towns[e->origin].y)*u;
    if(e->type==2){e->x=towns[e->town].x;e->y=towns[e->town].y;}
    regional_clear(&e->x,&e->y);
}
void living_init(void){
    if(LW.version)return;memset(&LW,0,sizeof(LW));LW.version=1;
    for(int i=0;i<MAX_ACTORS;i++)LW.command[i]=-1;
    for(int i=0;i<16;i++)LW.history[i]=-1;
    for(int i=0;i<EVENTS;i++)living_generate(i);
    /* Upgrade existing strong opponents once. Named stories keep their quest items. */
    for(int n=0;n<g.actorCount;n++)if(!g.actors[n].squad&&depth_named(n)<0&&!regional_named(n)&&(g.actors[n].kind==RAIDER||g.actors[n].kind==BEAST)){
        int lv=level(&g.actors[n],MELEE),tier=lv>=45?4:lv>=30?3:lv>=18?2:lv>=10?1:0;
        living_loot(n,n%8,tier);
    }
}
int living_validate(const Game*s){
    const LivingWorld*d=&s->living;if(!d->version)return 1;if(d->version!=1||d->serial<0||d->historyPos<0||d->historyPos>=16||!isfinite(d->pulse))return 0;
    for(int i=0;i<MAX_ACTORS;i++)if(d->owned[i]<-1||d->owned[i]>EVENTS+POP_GROUPS||d->command[i]<-1||d->command[i]>=s->actorCount)return 0;
    for(int i=0;i<EVENTS;i++){const WorldEvent*e=&d->events[i];if(e->type<0||e->type>7||e->modifier<0||e->modifier>3||e->tier<0||e->tier>4||e->count<0||e->count>16||e->status<0||e->status>2||e->town<0||e->town>=TOWNS||e->origin<0||e->origin>=TOWNS||e->faction<0||e->faction>=FACTIONS||!isfinite(e->x)||!isfinite(e->y)||!isfinite(e->age)||!isfinite(e->presence)||e->x<0||e->y<0||e->x>WORLD||e->y>WORLD)return 0;
        for(int j=0;j<16;j++)if(e->members[j]<-1||e->members[j]>=s->actorCount)return 0;
        if(e->stored<0||e->stored>1||e->spawned<0||e->spawned>1)return 0;
        if(e->stored)for(int j=0;j<e->count;j++){
            const Actor*a=&e->reserve[j];const DepthActor*z=&e->reserveDepth[j];
            if(!memchr(a->name,0,16)||a->faction<0||a->faction>=FACTIONS||a->kind<0||a->kind>CARAVAN||!isfinite(a->blood)||!isfinite(a->unconscious)||!isfinite(a->bleed)||z->layer<0||z->layer>=ITEMS)return 0;
            for(int t=0;t<PACK;t++)if(a->pack[t].item<0||a->pack[t].item>=ITEMS||a->pack[t].qty<0||a->pack[t].qty>999)return 0;
            for(int t=0;t<GEARS;t++)if(a->gear[t]<0||a->gear[t]>=ITEMS)return 0;
            for(int t=0;t<LIMBS;t++)if(!isfinite(a->health[t])||!isfinite(z->trauma[t])||z->trauma[t]<0||z->trauma[t]>80)return 0;
            for(int t=0;t<SKILLS;t++)if(!isfinite(a->xp[t])||a->xp[t]<0)return 0;
        }

    }return 1;
}
static int materialize(int slot){
    WorldEvent*e=&LW.events[slot];int freeSlots=MAX_ACTORS-g.actorCount;
    for(int n=0;n<g.actorCount;n++)if(LW.owned[n]<0)freeSlots++;
    if(freeSlots<e->count)return 0;
    for(int i=0;i<e->count;i++){
        int n=-1;for(int j=0;j<g.actorCount;j++)if(LW.owned[j]<0){n=j;break;}
        if(n<0)n=g.actorCount++;
        float x=e->x+(i%4)*1.6f,y=e->y+(i/4)*1.7f;regional_clear(&x,&y);
        static const char *roles[8]={"VANGUARD","SCOUT","BREAKER","GUNNER","SPEAR GUARD","REAVER","CAPTAIN","MEDIC"};
        char name[16];snprintf(name,sizeof(name),"%s %s",e->modifier==3?"VET":"ROAD",i==0&&e->type==7?"WARLORD":roles[i%8]);
        if(e->type==6)snprintf(name,sizeof(name),i==0?"ARMY CAPTAIN":i%4==0?"ARMY VETERAN":"ARMY SOLDIER");
        if(e->stored){
            society_actor_reset(n);g.actors[n]=e->reserve[i];g.depth.actors[n]=e->reserveDepth[i];
            Actor*a=&g.actors[n];a->x=x;a->y=y;a->active=1;a->target=a->attackTarget=-1;a->attackPhase=0;a->routeLen=a->routePos=0;
            e->members[i]=n;LW.owned[n]=slot+1;LW.command[n]=-1;LW.anger[n]=e->reserveAnger[i]?1:0;continue;
        }
        int tier=i==0?e->tier:(int)fmaxf(0,e->tier-1),skill=5+tier*12+(e->modifier==3?7:0);
        init_actor(n,name,e->type==5?BEAST:e->type==4?SETTLER:GUARD,e->faction,x,y,skill);
        memset(&g.depth.actors[n],0,sizeof(DepthActor));g.depth.actors[n].upkeep=100;
        living_loot(n,i%8,tier);
        Actor*a=&g.actors[n];population_roll_limbs(a,e->seed^(i*73819u),tier);
        if(e->modifier==1)a->hunger=25;
        if(e->modifier==2&&a->kind!=BEAST)a->gear[1]=gear_id(38,3,tier>=3?3:tier);
        if(e->type==6)armor_unit_outfit(a,i==0?2:i%4==0?1:0);
        if(e->type==4){a->health[CHEST]=35;a->health[LLEG]=25;a->bleed=.1f;}
        e->members[i]=n;LW.owned[n]=slot+1;LW.anger[n]=0;LW.command[n]=-1;
    }e->spawned=1;return 1;
}
static void finish_event(WorldEvent*e,int success){
    e->status=success?1:2;e->age=0;
    int delta=success?6:-6;g.prosperity[e->town]=(int)limit(g.prosperity[e->town]+delta,10,95);
    if(success&&e->participated){g.money+=180+e->tier*80;g.rep[e->faction]+=e->faction==HOSTILE||e->faction==WILDLIFE?0:5;log_msg("Incident resolved: +%d silver.",180+e->tier*80);}
}
void living_tick(float dt){
    LW.pulse+=dt;if(LW.pulse<1)return;float step=LW.pulse;LW.pulse=0;Actor*p=&g.actors[g.selected];
    for(int k=0;k<EVENTS;k++){
        WorldEvent*e=&LW.events[k];e->age+=step;
        if(e->status){if(e->age>120){
            int relief=e->status==2&&(e->type==1||e->type==6),town=e->town,serial=e->serial;
            living_generate(k);
            if(relief&&e->serial!=serial){e->type=2;e->count=0;e->town=town;e->x=towns[town].x;e->y=towns[town].y;e->faction=g.townOwner[town];}
        }continue;}
        float near=distance(p->x,p->y,e->x,e->y);
        if(near<22){e->presence+=step;if(e->presence>=15)e->participated=1;}
        if(e->count&&!e->spawned&&near<45)materialize(k);
        int standing=0,credited=0;
        for(int i=0;i<e->count&&e->spawned;i++){Actor*a=&g.actors[e->members[i]];if(a->defeatCredited)credited=1;if(!a->dead&&a->unconscious<=0)standing++;}
        if(e->spawned&&!standing&&e->type!=4){e->participated=credited;finish_event(e,e->type!=0);continue;}
        if(e->age>1500){finish_event(e,0);continue;}
        int moving=e->type==0||e->type==1||e->type==5||e->type==6||e->type==7;
        if(moving){float dx=towns[e->town].x-e->x,dy=towns[e->town].y-e->y,d=sqrtf(dx*dx+dy*dy);
            int arrived=d<(e->type==6?2:9);
            if(arrived&&e->spawned){arrived=0;for(int i=0;i<e->count;i++){Actor*a=&g.actors[e->members[i]];if(a->active&&!a->dead&&a->unconscious<=0&&distance(a->x,a->y,towns[e->town].x,towns[e->town].y)<8)arrived=1;}}
            /* A visible invasion must defeat the defending garrison before occupation. */
            if(arrived&&e->type==6)for(int n=0;n<g.actorCount;n++){Actor*a=&g.actors[n];if(!living_owned(n)&&!a->squad&&a->active&&!a->dead&&a->unconscious<=0&&!g.depth.actors[n].captive&&a->kind==GUARD&&a->homeTown==e->town&&a->faction!=e->faction)arrived=0;}
            if(arrived){
                if(e->type==6 && war_rivals(e->faction,g.townOwner[e->town]) && !(e->town==12&&g.regions.outcome[2]) &&
                   (!g.war.heldSince[e->town] || g.time-g.war.heldSince[e->town]>=650))war_conquer(e->town,e->faction,-1);
                finish_event(e,e->type==0);if(e->type==0){g.stock[e->town][RATION]+=12;g.stock[e->town][MEDKIT]+=8;}continue;}
            /* Stop the formation anchor during nearby battle; members catch up. */
            int fighting=0;for(int i=0;i<e->count&&e->spawned;i++)if(g.actors[e->members[i]].target>=0)fighting=1;
            if(e->spawned)for(int i=0;i<e->count;i++){Actor*a=&g.actors[e->members[i]];if(a->active&&!a->dead&&a->unconscious<=0){if(distance(a->x,a->y,e->x,e->y)>7)fighting=1;break;}}
            if(!fighting){float speed=e->modifier==1?.35f:e->modifier==2?.5f:.7f;e->x+=dx/fmaxf(.1f,d)*speed*step;e->y+=dy/fmaxf(.1f,d)*speed*step;}
        }
        if(e->spawned&&near>80){
            int safe=1;for(int i=0;i<e->count;i++){int n=e->members[i];Actor*a=&g.actors[n];if(a->squad||g.society.freed[n]||a->carriedBy>=0||a->carrying>=0||g.depth.actors[n].captive||merc_slot(a)>=0||distance(p->x,p->y,a->x,a->y)<65)safe=0;}
            if(safe){for(int i=0;i<e->count;i++){int n=e->members[i];e->reserve[i]=g.actors[n];e->reserveDepth[i]=g.depth.actors[n];e->reserveAnger[i]=LW.anger[n];}
                if(release_event(k)){for(int i=0;i<16;i++)e->members[i]=-1;e->stored=1;e->spawned=0;}
            }
        }
        else if(e->spawned){for(int i=0;i<e->count;i++){Actor*a=&g.actors[e->members[i]];if(!a->active&&!a->dead){a->active=1;a->x=e->x+(i%4)*1.6f;a->y=e->y+(i/4)*1.7f;}}}
    }
}
int living_walk(int n,float dt){if(population_walk(n,dt))return 1;int k=event_for(n);if(k<0)return 0;WorldEvent*e=&LW.events[k];Actor*a=&g.actors[n];
    if(e->type==4){if(e->status==1)route_toward(a,a->homeX,a->homeY,dt);return 1;}
    if(e->status)return 1;
    int i=0;while(i<16&&e->members[i]!=n)i++;float x=e->x+(i%4)*1.6f,y=e->y+(i/4)*1.7f;
    if(distance(a->x,a->y,x,y)>1)route_toward(a,x,y,dt);return 1;
}
int living_hostile(const Actor*a,const Actor*b){if(population_hostile(a,b))return 1;int ai=actor_index(a),bi=actor_index(b);if(ai<0||bi<0)return 0;
    if((LW.anger[ai]&&b->squad)||(LW.anger[bi]&&a->squad))return 1;
    int k=event_for(ai);if(k>=0&&LW.events[k].type==6&&!b->squad&&b->kind==GUARD&&b->faction!=a->faction)return 1;
    k=event_for(bi);return k>=0&&LW.events[k].type==6&&!a->squad&&a->kind==GUARD&&b->faction!=a->faction;
}
void living_action(int k){if(k<0||k>=EVENTS)return;WorldEvent*e=&LW.events[k];Actor*a=&g.actors[g.selected];
    if(e->status){log_msg("This incident has ended.");return;}
    if(!conscious(a)||distance(a->x,a->y,e->x,e->y)>(e->type==2?14:3)){log_msg("Reach the marked incident before acting.");return;}
    if(e->type==2){int it=e->modifier==1?RATION:e->modifier==2?PARTS:MEDKIT;if(!inventory_remove(a,it,3+e->tier)){log_msg("Bring %d %s.",3+e->tier,itemDefs[it].name);return;}g.stock[e->town][it]+=12+e->tier*3;}
    else if(e->type==3){int it=armor_cache_item(g.seed^(unsigned)e->serial*2654435761u,e->tier);if(!inventory_add(a,it,1)){log_msg("Make room in your pack first.");return;}}
    else if(e->type==4){if(!e->spawned){log_msg("Approach the pilgrim first.");return;}Actor*b=&g.actors[e->members[0]];if(b->dead||b->health[CHEST]<65||b->bleed>.01f){log_msg("Bandage the pilgrim before helping them depart.");return;}b->homeX=towns[e->town].x;b->homeY=towns[e->town].y;}
    else{log_msg("This incident resolves through escort or combat.");return;}
    e->participated=1;finish_event(e,1);
}
int combat_candidate(int row){int ids[MAX_ACTORS],count=0;float ds[MAX_ACTORS];Actor*a=&g.actors[g.selected];
    for(int i=0;i<g.actorCount;i++){Actor*b=&g.actors[i];if(b->squad||!b->active||!conscious(b)||depth_room(a)!=depth_room(b)||depth_distance(a,b)>14)continue;
        float d=depth_distance(a,b);int j=count++;while(j>0&&ds[j-1]>d){ds[j]=ds[j-1];ids[j]=ids[j-1];j--;}ds[j]=d;ids[j]=i;}
    return row==-1?count:row>=0&&row<count?ids[row]:-1;
}
int combat_candidates(void){return combat_candidate(-1);}
int actor_relation(int n){if(n<0||n>=g.actorCount)return 0;Actor*a=&g.actors[n],*p=&g.actors[g.selected];return hostile(a,p)?-1:a->squad||a->faction==p->faction||g.rep[a->faction]>=15?1:0;}
void command_attack(int n){Actor*a=&g.actors[g.selected];if(n<0||n>=g.actorCount||g.actors[n].squad||!conscious(a)||!conscious(&g.actors[n])||depth_distance(a,&g.actors[n])>14)return;
    g.field.playingDead[g.selected]=0;
    Actor*b=&g.actors[n];if(!hostile(a,b)&&!warrant_authorizes(n)){crime_charge(a,b->faction,100,CRIME_ASSAULT);g.rep[b->faction]-=20;log_msg("Assault: +100 bounty; faction standing -20.");}
    population_anger(n);LW.anger[n]=1;int k=event_for(n);if(k>=0)for(int i=0;i<LW.events[k].count;i++){int j=LW.events[k].members[i];if(j>=0)LW.anger[j]=1;}
    LW.command[g.selected]=n;a->target=n;a->autoattack=1;ui.attackFocus=n;
}
#undef LW
