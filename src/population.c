#include "game.h"
#include "settlement_art_ids.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define P (g.population)
extern int conscious(const Actor *a);
extern int hostile(const Actor *a,const Actor *b);
static void life_seed(int group);
static const char *names[POP_TYPES]={
    "SCAVENGER CAMP","REFUGEE CAMP","HUNTERS' CAMP","SMUGGLERS' DEN",
    "TRADERS' REST","ROAD PATROL","RAIDING BAND","BEAST PACK",
    "PILGRIMAGE","WOUNDED TRAVELERS","BATTLEFIELD","MARCHING ARMY",
    "BANDIT AMBUSH","SALVAGE CREW","PRISONER CONVOY","WANDERING DUELISTS"
};
static const char *phases[]={"resting","traveling","foraging","fighting","aftermath"};
static unsigned random_step(unsigned *seed){unsigned x=*seed;x^=x<<13;x^=x>>17;x^=x<<5;return *seed=x?x:73819;}
static int bounded(int n,int lo,int hi){return n<lo?lo:n>hi?hi:n;}
static int actor_no(const Actor *a){return (uintptr_t)a>=(uintptr_t)g.actors&&(uintptr_t)a<(uintptr_t)(g.actors+g.actorCount)?(int)(a-g.actors):-1;}
int population_owned(int n){return n>=0&&n<g.actorCount&&g.living.owned[n]>EVENTS&&g.living.owned[n]<=EVENTS+POP_GROUPS?g.living.owned[n]-EVENTS:0;}
static int passing_group(int k){return k>=WAR_POP_GROUPS;}
static int legion_group(int k){return k>=OLD_POP_GROUPS&&k<WAR_POP_GROUPS;}
static int camp_suitable(int type) {
    return type<=POP_MERCHANTS||type==POP_WOUNDED||type==POP_AMBUSH||type==POP_SALVAGE;
}
static int camp_type(int k){
    return camp_suitable(P.groups[k].type)&&(!passing_group(k)||P.state[k].beds);
}
static int military(int type){return type==POP_PATROL||type==POP_RAIDERS||type==POP_ARMY||type==POP_AMBUSH||type==POP_PRISONERS;}
static int enemies(const WorldEvent*a,const WorldEvent*b){
    if(a->faction==b->faction)return 0;
    if(a->faction==HOSTILE||b->faction==HOSTILE||a->faction==WILDLIFE||b->faction==WILDLIFE)return 1;
    if(war_rivals(a->faction,b->faction)&&military(a->type)&&military(b->type))return 1;
    return (a->type==POP_ARMY||b->type==POP_ARMY)&&military(a->type)&&military(b->type);
}
static float observers(float x,float y){
    float best=100000;for(int n=0;n<g.actorCount;n++)if(g.actors[n].squad&&g.actors[n].active&&!g.actors[n].dead){float d=distance(x,y,g.actors[n].x,g.actors[n].y);if(d<best)best=d;}return best;
}
static void clear_position(float *x,float *y){
    float ox=fmaxf(8,fminf(WORLD-8,*x)),oy=fmaxf(8,fminf(WORLD-8,*y));
    for(int r=0;r<22;r++)for(int i=0;i<16;i++){
        float xx=ox+r*cosf(i*.392699f),yy=oy+r*sinf(i*.392699f);
        if(xx>3&&yy>3&&xx<WORLD-4&&yy<WORLD-4&&can_walk(xx,yy)&&can_walk(xx+.4f,yy)&&can_walk(xx,yy+.4f)){*x=xx;*y=yy;return;}
    }*x=ox;*y=oy;
}
static int nearest_town(float x,float y,int avoid){int found=0;float best=1e9f;for(int t=0;t<TOWNS;t++)if(t!=avoid){float d=distance(x,y,towns[t].x,towns[t].y);if(d<best){best=d;found=t;}}return found;}
static Actor *member(int k,int i){WorldEvent*e=&P.groups[k];if(P.state[k].detached&(1<<i))return NULL;return e->spawned?e->members[i]>=0?&g.actors[e->members[i]]:NULL:&e->reserve[i];}
static DepthActor *member_depth(int k,int i){WorldEvent*e=&P.groups[k];return e->spawned?&g.depth.actors[e->members[i]]:&e->reserveDepth[i];}
static int standing(int k){int n=0;WorldEvent*e=&P.groups[k];for(int i=0;i<e->count;i++){Actor*a=member(k,i);if(a&&!a->dead&&a->unconscious<=0&&a->health[HEAD]>0&&a->health[CHEST]>0&&a->blood>=20)n++;}return n;}
const char *population_name(int k){
    if(k<0||k>=POP_GROUPS)return "TRAVELERS";
    int type=P.groups[k].type;
    if(P.state[k].phase==POP_RUINS&&P.state[k].fights&&type==POP_PATROL)return "FALLEN PATROL";
    if(passing_group(k)&&!camp_type(k)){
        if(type==POP_MERCHANTS)return "ROAD TRADERS";
        if(type==POP_HUNTERS)return "ROVING HUNTERS";
        if(type==POP_SCAVENGERS)return "ROVING SCAVENGERS";
    }
    return names[type];
}
void population_roll_limbs(Actor *a,unsigned seed,int tier){
    if(a->squad||a->dead||a->kind==BEAST||a->lostMask||a->prostheticMask)return;
    for(int l=LARM;l<=RLEG;l++)if(a->health[l]<species_health(a)*.8f)return;
    /* Uncommon ordinary wearers, modest veteran bias; no guaranteed rolled loot. */
    unsigned chance=tier>=3?10:tier==2?7:4;
    if(random_step(&seed)%100>=chance)return;
    int pieces=random_step(&seed)%10==0?2:1;
    for(int i=0;i<pieces;i++){
        int l=LARM+random_step(&seed)%4;if(a->lostMask&(1<<l))continue;
        unsigned r=random_step(&seed)%1000;int grade;
        if(tier<=0)grade=r<850?0:1;
        else if(tier==1)grade=r<450?0:r<950?1:2;
        else if(tier==2)grade=r<500?1:r<950?2:3;
        else grade=r<200?1:r<700?2:r<995?3:4;
        limb_set(a,l,limb_item(grade,l>=LLEG),0);a->health[l]=limb_max_health(a,l);
    }
}
static void social_save(int k,int i,int n){
    PopulationSocial*z=&P.social[k][i];memcpy(z->weaponXP,g.society.weaponXP[n],sizeof(z->weaponXP));memcpy(z->wanted,g.society.wanted[n],sizeof(z->wanted));memcpy(z->recognized,g.society.recognized[n],sizeof(z->recognized));z->cannibal=g.society.cannibal[n];z->captorFaction=g.society.captorFaction[n];
}
static void social_load(int k,int i,int n){
    PopulationSocial*z=&P.social[k][i];society_actor_reset(n);memcpy(g.society.weaponXP[n],z->weaponXP,sizeof(z->weaponXP));memcpy(g.society.wanted[n],z->wanted,sizeof(z->wanted));memcpy(g.society.recognized[n],z->recognized,sizeof(z->recognized));g.society.cannibal[n]=z->cannibal;g.society.captorFaction[n]=z->captorFaction;
}
static int pinned(int n){
    Actor*a=&g.actors[n];int own=population_owned(n),externalCarry=(a->carriedBy>=0&&(!own||population_owned(a->carriedBy)!=own))||(a->carrying>=0&&(!own||population_owned(a->carrying)!=own));if(a->squad||externalCarry||merc_slot(a)>=0||g.society.freed[n]||g.society.rescueHelper[n]||society_rescue_busy(n)||a->attackPhase||a->target>=0)return 1;
    for(int c=0;c<CONTRACTS;c++)if(g.contracts[c].active==1&&g.society.jobActor[c]==n+1)return 1;
    for(int j=0;j<PROJECTILES;j++)if(g.depth.shots[j].active&&g.depth.shots[j].source==n)return 1;
    for(int j=0;j<g.actorCount;j++)if(g.actors[j].active&&(g.actors[j].target==n||(g.actors[j].attackTarget==n&&g.actors[j].attackPhase)))return 1;
    return 0;
}
static void release_slot(int n){
    g.plans.corpseSince[n]=0;
    g.actors[n].active=0;g.living.owned[n]=-1;g.living.anger[n]=0;g.living.command[n]=-1;
    for(int j=0;j<g.actorCount;j++){if(g.actors[j].target==n)g.actors[j].target=-1;if(g.actors[j].attackTarget==n){g.actors[j].attackTarget=-1;g.actors[j].attackPhase=0;}if(g.living.command[j]==n)g.living.command[j]=-1;}
    if(ui.attackFocus==n)ui.attackFocus=-1;
}
void population_prune_corpses(void) {
    for(int n=0;n<g.actorCount;n++){
        Actor*a=&g.actors[n];
        if(!a->active||!a->dead){g.plans.corpseSince[n]=0;continue;}
        if(!g.plans.corpseSince[n])g.plans.corpseSince[n]=fmaxf(.001f,g.time);
        if(a->squad||a->carriedBy>=0||a->carrying>=0||a->bountyReward>0||
           g.time-g.plans.corpseSince[n]<180||observers(a->x,a->y)<32)continue;
        int own=population_owned(n)-1;
        if((living_owned(n)&&own<0)||depth_named(n)>=0||regional_named(n)||civil_elite_index(n)>=0||
           politics_protected(n)||npc_role(a)==ROLE_DEPOT||npc_role(a)==ROLE_MERCENARY||
           (own<0&&a->homeTown>=0&&a->kind!=RAIDER&&a->kind!=BEAST))continue;
        a->target=-1;a->attackPhase=0;
        if(pinned(n))continue;
        if(own>=0){
            WorldEvent*e=&P.groups[own];
            for(int i=0;i<e->count;i++)if(e->members[i]==n){P.state[own].detached|=1u<<i;e->members[i]=-1;e->reserve[i].dead=1;}
        }
        release_slot(n);
    }
}
static void set_destination(int k,int town){
    WorldEvent*e=&P.groups[k];PopulationState*m=&P.state[k];e->town=town;
    float angle=(random_step(&e->seed)%628)/100.f;
    m->targetX=towns[town].x+cosf(angle)*28;m->targetY=towns[town].y+sinf(angle)*28;clear_position(&m->targetX,&m->targetY);
}
static int vendor_member(const WorldEvent *e) {
    if (e->count <= 0)
        return -1;
    if (e->type == POP_MERCHANTS || e->type == POP_SMUGGLERS ||
        (e->type == POP_SCAVENGERS && e->serial % 5 == 0) ||
        (e->type == POP_HUNTERS && e->serial % 7 == 0))
        return e->serial % e->count;
    return -1;
}
int population_vendor(int n) {
    if (n < 0 || n >= g.actorCount)
        return 0;
    int k = population_owned(n) - 1;
    if (k < 0 || g.actors[n].squad)
        return 0;
    WorldEvent *e = &P.groups[k];
    int vendor = vendor_member(e);
    return vendor >= 0 && e->members[vendor] == n && !g.actors[n].dead;
}
static void make_actor(int k,int i){
    WorldEvent*e=&P.groups[k];Actor*a=&e->reserve[i];DepthActor*z=&e->reserveDepth[i];unsigned seed=random_step(&e->seed);
    static const char *first[]={"Ash","Reed","Mara","Flint","Korin","Sable","Iris","Tarn","Vera","Rook","Nox","Dara","Holt","Ren","Edda","Kest"};
    static const char *last[]={"Vale","Kade","Moss","Silt","Rell","Rusk","Grey","Venn","Cairn","Thorn","Soot","Fen","Salt","Ridge","Stone","Wren"};
    memset(a,0,sizeof(*a));memset(z,0,sizeof(*z));z->upkeep=100;for(int q=0;q<4;q++)z->queue[q]=JOB_HOLD;
    snprintf(a->name,sizeof(a->name),"%s %s",first[seed%16],last[(seed>>8)%16]);
    a->active=1;a->faction=e->faction;a->kind=military(e->type)||e->type==POP_DUELISTS||e->type==POP_HUNTERS?GUARD:SETTLER;
    if(e->type==POP_BEASTS){a->kind=BEAST;snprintf(a->name,sizeof(a->name),"%s",biome(e->x,e->y)==3?"Reed stalker":"Dune prowler");}
    if(e->type==POP_RAIDERS||e->type==POP_AMBUSH)a->kind=RAIDER;
    a->target=a->attackTarget=a->carrying=a->carriedBy=-1;a->prisonTown=-1;a->homeTown=e->origin;a->body=seed%3;a->skin=(seed>>4)%8;a->hair=(seed>>7)%7;a->hairColor=(seed>>10)%6;a->scar=(seed>>14)%5;
    a->race=e->faction==SHOGUN?(seed%3==0?4:0):e->faction==MONKS?(seed%2?1:3):seed%5;
    int tier=e->tier-(i>0&&e->tier>0),lv=4+tier*9+random_step(&seed)%7;
    if(e->type==POP_REFUGEES||e->type==POP_WOUNDED)lv=3+random_step(&seed)%6;
    if ((a->kind == RAIDER || a->kind == BEAST) && starter_country(e->x, e->y)) {
        tier = 0;
        if (lv > 12) lv = 12;
    }
    int weak = e->type == POP_RAIDERS && e->tier == 0 && e->modifier == 1;
    if (weak) {
        static const int races[] = {0,1,3};
        a->race = races[seed % 3];
        a->healthScale = 63 + (seed >> 10) % 8;
        lv = 3 + (seed >> 17) % 5;
    }
    for(int s=0;s<SKILLS;s++)a->xp[s]=12*(lv-1)*(lv-1);
    for(int l=0;l<LIMBS;l++)a->health[l]=species_health(a);
    a->x=a->homeX=a->tx=e->x+(i%4-1)*1.8f;a->y=a->homeY=a->ty=e->y+(i/4)*1.8f;clear_position(&a->x,&a->y);
    a->homeX=a->tx=a->x;a->homeY=a->ty=a->y;a->blood=a->hunger=100;a->speechCooldown=20+seed%40;
    int poor = a->kind == RAIDER && e->modifier == 1;
    living_gear(a,i%8,poor ? 0 : tier,(int)(seed%10000));
    if(e->type!=POP_REFUGEES && e->type!=POP_WOUNDED && !poor)armor_wanderer(a,seed,tier);
    if(a->kind==RAIDER)z->upkeep=35+(seed>>10)%36;
    if (i == vendor_member(e)) {
        inventory_add(a, RATION, 3 + tier);
        inventory_add(a, portion_item(MEDKIT, seed % 23 == 0 ? 100 : 40 + seed % 48), 1);
        inventory_add(a, seed % 2 ? JERKY : OATCAKE, 1);
        inventory_add(a, e->type == POP_SMUGGLERS ? PARTS : BEDROLL_ITEM, 1);
        if (e->type == POP_SMUGGLERS) inventory_add(a, CARTRIDGE, 12);
    }
    if(e->type==POP_REFUGEES||e->type==POP_WOUNDED){a->gear[0]=STAFF;a->gear[1]=RAGS;a->gear[2]=0;}
    if((e->type==POP_ARMY||e->type==POP_PATROL)&&war_faction_index(e->faction)>=0){
        war_outfit(a,ROLE_MARSHAL,i,i==0?3:i%4==0?2:1);
    }else if(e->type==POP_ARMY||e->type==POP_PATROL){
        int m=tier>=2?3:1,q=tier>=2?2:1;
        a->gear[0]=gear_id(i%4==3?29:e->faction==SHOGUN?5:16,m,q);
        a->gear[1]=gear_id(e->faction==SHOGUN?38:34,m,q);
        a->gear[2]=gear_id(e->faction==SHOGUN?51:50,m,q);
        a->gear[3]=gear_id(62,m,q);a->gear[4]=gear_id(70,m,q);
        if(i==0)a->gear[5]=gear_id(77,m,q);
        if(weapon_ranged(a->gear[0]))inventory_add(a,CARTRIDGE,20);
    }
    if(e->type==POP_DUELISTS){a->gear[2]=gear_id(53,5,3);a->gear[3]=gear_id(62,5,3);a->gear[4]=gear_id(75,5,3);a->gear[5]=gear_id(80,5,3);}
    if((e->type==POP_RAIDERS||e->type==POP_AMBUSH)&&e->modifier==1){
        a->gear[0]=gear_id(i%2?14:0,0,0);a->gear[1]=i%3?RAGS:0;a->gear[2]=a->gear[3]=a->gear[5]=0;a->gear[4]=gear_id(68,0,0);
    }
    if(poor)armor_wanderer(a,seed,0);
    if ((e->type == POP_PILGRIMS || e->type == POP_MERCHANTS || e->type == POP_REFUGEES) &&
        (i + e->serial) % 4 == 0) {
        int supply = a->race == 2 ? PARTS : MEDKIT;
        if (!inventory_count(a, supply)) inventory_add(a, supply, 1);
    }
    if (weak)
        a->hunger = 48 + (seed >> 19) % 20;
    else {
        if(e->modifier==1)a->hunger=28;
        population_roll_limbs(a,random_step(&seed),tier);
    }
    if(e->type==POP_WOUNDED){a->health[CHEST]=species_health(a)*.38f;a->health[LLEG]=24;a->bleed=.08f;inventory_remove(a,MEDKIT,99);}
    if(e->type==POP_BATTLEFIELD){a->dead=i<e->count-1;a->blood=a->dead?0:42;a->health[CHEST]=a->dead?-80:22;a->unconscious=a->dead?0:120;a->bleed=a->dead?0:.04f;}
    arsenal_outfit(a,seed,tier);
    if(e->type==POP_PRISONERS&&i>=e->count-2){a->kind=SETTLER;a->faction=FREE;a->gear[0]=0;a->gear[1]=RAGS;a->gear[2]=0;z->captive=1;P.social[k][i].captorFaction=e->faction;}
}
typedef struct {
    float x,y;
    int faction, beds, size;
} EncounterSite;
static const int forces[WAR_FACTIONS]={UNION,SHOGUN,MONKS};
static void generate_population(int k,int type,const EncounterSite *site){
    if(k<0||k>=POP_GROUPS)return;WorldEvent*e=&P.groups[k];PopulationState*m=&P.state[k];
    if(e->spawned)return;memset(e,0,sizeof(*e));memset(m,0,sizeof(*m));memset(P.social[k],0,sizeof(P.social[k]));
    int passing=passing_group(k),legion=legion_group(k);
    e->seed=random_step(&P.rng);e->serial=++P.serial;e->type=legion?POP_ARMY:bounded(type,0,POP_TYPES-1);e->modifier=random_step(&e->seed)%4;e->origin=k%OLD_TOWNS;e->tier=bounded(e->origin/5+(int)(random_step(&e->seed)%2),0,3);
    e->faction=e->type==POP_RAIDERS||e->type==POP_AMBUSH?HOSTILE:e->type==POP_BEASTS?WILDLIFE:e->type==POP_SMUGGLERS||e->type==POP_PRISONERS?OUTLAWS:e->type==POP_ARMY||e->type==POP_PATROL?forces[random_step(&e->seed)%WAR_FACTIONS]:e->type==POP_PILGRIMS?MONKS:SCAVENGERS;
    if (site && site->faction) e->faction=site->faction;
    if(legion){
        int home=OLD_TOWNS+k-OLD_POP_GROUPS;
        e->origin=home;e->faction=towns[home].faction;e->modifier=0;
        if(g.townOwner[home]!=e->faction)for(int t=0;t<TOWNS;t++)if(g.townOwner[t]==e->faction){e->origin=t;break;}
    }
    if (site) e->origin=nearest_town(site->x,site->y,-1);
    else if (passing) {
        static const int routes[ROAMING_BANDIT_GROUPS] = {0,1,2,3,5,9,11,15};
        e->origin = routes[k - WAR_POP_GROUPS];
    }
    if (passing) e->tier=bounded(e->tier,0,1);
    if (e->type==POP_RAIDERS&&(passing||site)) {
        e->modifier = 1;
        e->tier = 0;
    }
    e->town=nearest_town(towns[e->origin].x,towns[e->origin].y,e->origin);
    e->count=e->type==POP_ARMY?12+random_step(&e->seed)%5:e->type==POP_PRISONERS?7:e->type==POP_BEASTS?3+random_step(&e->seed)%4:e->type==POP_WOUNDED?2:e->type==POP_DUELISTS?1+random_step(&e->seed)%3:3+random_step(&e->seed)%3;
    if(e->type==POP_ARMY||e->type==POP_DUELISTS)e->tier=2+random_step(&e->seed)%2;
    if(legion){e->count=10+random_step(&e->seed)%3;e->tier=2;}
    if (site && site->size) e->count=site->size;
    if (e->type==POP_RAIDERS&&e->tier==0&&e->modifier==1) e->count=3+random_step(&e->seed)%2;
    for(int i=0;i<POP_MEMBERS;i++)e->members[i]=-1;
    float angle=(random_step(&e->seed)%628)/100.f;
    if(site){e->x=site->x;e->y=site->y;}
    else if(k<16||passing){float r=35+random_step(&e->seed)%25;e->x=towns[e->origin].x+cosf(angle)*r;e->y=towns[e->origin].y+sinf(angle)*r;}
    else if(k<48||k>=64){float u=.2f+(random_step(&e->seed)%60)*.01f;int next=e->town;e->x=towns[e->origin].x*(1-u)+towns[next].x*u+cosf(angle)*18;e->y=towns[e->origin].y*(1-u)+towns[next].y*u+sinf(angle)*18;}
    else{int cell=k-48;e->x=(cell%4+.2f+(random_step(&e->seed)%60)*.01f)*WORLD/4;e->y=(cell/4+.2f+(random_step(&e->seed)%60)*.01f)*WORLD/4;}
    if(!site){
        if(town_near(e->x,e->y,24)>=0){e->x+=cosf(angle)*35;e->y+=sinf(angle)*35;}
        clear_position(&e->x,&e->y);
        for(int tries=0;tries<16&&observers(e->x,e->y)<32;tries++){e->x+=cosf(angle)*10;e->y+=sinf(angle)*10;clear_position(&e->x,&e->y);}
    }
    m->beds=site&&site->beds ? site->beds : !passing&&camp_suitable(e->type) ? 2+(int)(random_step(&e->seed)%2) : 0;
    m->siteX=e->x;m->siteY=e->y;m->opponent=-1;m->generation=e->serial;m->phase=camp_type(k)?POP_REST:e->type==POP_BATTLEFIELD?POP_RUINS:POP_TRAVEL;m->clock=100+random_step(&e->seed)%160;
    m->stock[0]=4+random_step(&e->seed)%6;m->stock[1]=1+random_step(&e->seed)%3;m->stock[2]=1+random_step(&e->seed)%3;
    m->cacheItem=e->type==POP_SALVAGE?PARTS:e->type==POP_HUNTERS?HIDE:e->type==POP_BATTLEFIELD?MEDKIT:e->type==POP_SMUGGLERS?TONIC:RATION;m->cacheQty=1+random_step(&e->seed)%3;
    if(e->type==POP_SALVAGE && e->tier>=1 && e->seed%5==0){m->cacheItem=armor_cache_item(e->seed,e->tier);m->cacheQty=1;}
    for(int i=0;i<5;i++){float x=e->x+(i==0?0:i==4?4:(i-2)*3),y=e->y+(i==0?0:i==4?-3:3);clear_position(&x,&y);m->offsetX[i]=x-e->x;m->offsetY[i]=y-e->y;}
    set_destination(k,e->town);for(int i=0;i<e->count;i++)make_actor(k,i);e->stored=1;life_seed(k);
    warrant_register(k);
}
void population_generate(int k,int type){
    generate_population(k,type,NULL);
}
static int importable(int n){
    Actor*a=&g.actors[n];return a->active&&!a->squad&&!a->dead&&!a->lostMask&&a->unconscious<=0&&!living_owned(n)&&depth_named(n)<0&&!regional_named(n)&&civil_elite_index(n)<0&&(a->kind==RAIDER||a->kind==BEAST)&&town_near(a->x,a->y,25)<0&&!g.depth.actors[n].captive&&a->prisonTime<=0&&!pinned(n)&&observers(a->x,a->y)>65;
}
void population_init(void){
    if(P.version)return;memset(&P,0,sizeof(P));P.version=1;P.rng=g.seed^0x51a17e13u;
    for(int k=0;k<OLD_POP_GROUPS;k++)population_generate(k,k<16?k%5:k%POP_TYPES);
    int imported=0;
    for(int n=0;n<g.actorCount;n++){
        Actor*a=&g.actors[n];if(a->active&&!a->squad&&depth_named(n)<0&&!regional_named(n)&&civil_elite_index(n)<0)population_roll_limbs(a,g.seed^(unsigned)n*19349663u,bounded(level(a,MELEE)/15,0,3));
        if(!importable(n))continue;int k=-1;
        for(int j=64;j<64+imported;j++)if(P.groups[j].faction==a->faction&&P.groups[j].count<POP_MEMBERS&&distance(a->x,a->y,P.groups[j].x,P.groups[j].y)<35){k=j;break;}
        if(k<0){if(imported>=16)continue;k=64+imported++;WorldEvent*e=&P.groups[k];memset(e,0,sizeof(*e));memset(&P.state[k],0,sizeof(P.state[k]));memset(P.social[k],0,sizeof(P.social[k]));e->seed=random_step(&P.rng);e->serial=++P.serial;e->type=a->kind==BEAST?POP_BEASTS:POP_RAIDERS;e->faction=a->faction;e->tier=bounded(level(a,MELEE)/15,0,3);e->origin=nearest_town(a->x,a->y,-1);e->town=nearest_town(a->x,a->y,e->origin);e->x=a->x;e->y=a->y;e->stored=1;for(int j=0;j<POP_MEMBERS;j++)e->members[j]=-1;P.state[k].phase=POP_TRAVEL;P.state[k].clock=150;P.state[k].opponent=-1;P.state[k].generation=e->serial;P.state[k].siteX=e->x;P.state[k].siteY=e->y;P.state[k].imported=1;set_destination(k,e->town);}
        WorldEvent*e=&P.groups[k];int i=e->count++;e->reserve[i]=*a;e->reserveDepth[i]=g.depth.actors[n];social_save(k,i,n);release_slot(n);P.imported++;
    }
}
void population_add_roaming_bandits(void) {
    if (P.version != 1)
        return;
    for (int k = WAR_POP_GROUPS; k < POP_GROUPS; k++)
        population_generate(k,POP_RAIDERS);
    P.version = 2;
}
int population_materialize(int k){
    if(k<0||k>=POP_GROUPS||!P.version)return 0;WorldEvent*e=&P.groups[k];PopulationState*m=&P.state[k];if(e->spawned)return 1;
    int need=0,free=MAX_ACTORS-g.actorCount;for(int i=0;i<e->count;i++)if(!(m->detached&(1<<i)))need++;for(int n=0;n<g.actorCount;n++)if(g.living.owned[n]<0)free++;
    if(free<need)return 0;
    for(int i=0;i<e->count;i++){
        if(m->detached&(1<<i))continue;int n=-1;for(int j=0;j<g.actorCount;j++)if(g.living.owned[j]<0){n=j;break;}if(n<0)n=g.actorCount++;
        social_load(k,i,n);g.actors[n]=e->reserve[i];g.depth.actors[n]=e->reserveDepth[i];Actor*a=&g.actors[n];a->active=1;a->target=a->attackTarget=-1;a->attackPhase=0;a->routeLen=a->routePos=0;a->taskKind=0;
        e->members[i]=n;g.living.owned[n]=EVENTS+k+1;g.living.anger[n]=e->reserveAnger[i];g.living.command[n]=-1;
    }e->spawned=1;
    if(g.life.version)for(int i=0;i<e->count;i++){int j=g.life.groups[k].carry[i];if(j>=0&&j<e->count&&e->members[i]>=0&&e->members[j]>=0){g.actors[e->members[i]].carrying=e->members[j];g.actors[e->members[j]].carriedBy=e->members[i];}}
    return 1;
}
int population_store(int k){
    if(k<0||k>=POP_GROUPS)return 0;WorldEvent*e=&P.groups[k];PopulationState*m=&P.state[k];if(!e->spawned)return 1;
    for(int i=0;i<e->count;i++)if(!(m->detached&(1<<i))){int n=e->members[i];if(n<0||pinned(n)||observers(g.actors[n].x,g.actors[n].y)<80)return 0;}
    for(int i=0;i<e->count;i++)if(!(m->detached&(1<<i))){int n=e->members[i];e->reserve[i]=g.actors[n];e->reserve[i].carrying=e->reserve[i].carriedBy=-1;e->reserveDepth[i]=g.depth.actors[n];e->reserveAnger[i]=g.living.anger[n];social_save(k,i,n);release_slot(n);e->members[i]=-1;}
    e->stored=1;e->spawned=0;return 1;
}
static void detach_followers(int k){
    WorldEvent*e=&P.groups[k];PopulationState*m=&P.state[k];if(!e->spawned)return;
    for(int i=0;i<e->count;i++)if(!(m->detached&(1<<i))){int n=e->members[i];Actor*a=&g.actors[n];if(a->squad||g.society.freed[n]||merc_slot(a)>=0){if(g.life.version){for(int j=0;j<e->count;j++)if(j==i||g.life.groups[k].carry[j]==i){int patient=g.life.groups[k].carry[j];if(patient>=0){int carrier=e->members[j],carried=e->members[patient];if(carrier>=0)g.actors[carrier].carrying=-1;if(carried>=0)g.actors[carried].carriedBy=-1;}g.life.groups[k].carry[j]=-1;}}m->detached|=1<<i;e->members[i]=-1;g.living.owned[n]=0;}}
}
static void physiology(int k,float dt){
    WorldEvent*e=&P.groups[k];PopulationState*m=&P.state[k];
    for(int i=0;i<e->count;i++){
        Actor*a=member(k,i);if(!a||a->dead)continue;
        if(e->spawned)continue;DepthActor*z=member_depth(k,i);
        if (a->unconscious <= 0 && injury_needs_dressing(a, z) && inventory_remove(a, MEDKIT, 1))
            injury_dress(a, z);
        if (injury_tick(a, z, dt, m->phase == POP_REST)) {
            a->dead = 1;
            a->unconscious = 0;
            a->woundFlags = 0;
            P.deaths++;
            continue;
        }
        if(a->race!=2){a->hunger=fmaxf(0,a->hunger-dt*.006f);if(a->hunger<35&&inventory_count(a,RATION)){inventory_remove(a,RATION,1);a->hunger=fminf(100,a->hunger+48);}}
        if (!(a->woundFlags & WOUND_REPAIRS))
            for (int l = 0; l < LIMBS; l++)
                if (!depth_organic(a, l) && !limb_missing(a, l) &&
                    a->health[l] < limb_max_health(a, l) * .5f && inventory_remove(a, PARTS, 1)) {
                    injury_repair(a, z);
                    break;
                }
    }
}
static int forage_node(int k){
    WorldEvent*e=&P.groups[k];int best=-1;float reach=36;
    for(int n=0;n<g.nodeCount;n++){Node*v=&g.nodes[n];if(v->owner||v->type>NODE_WOOD||v->remaining<=0)continue;float d=distance(e->x,e->y,v->x,v->y);if(d<reach){best=n;reach=d;}}
    return best;
}
static void forage_offline(int k,float dt){
    WorldEvent*e=&P.groups[k];PopulationState*m=&P.state[k];
    if(e->spawned||m->phase!=POP_FORAGE||(e->type!=POP_SCAVENGERS&&e->type!=POP_SALVAGE))return;
    m->harvestClock+=dt;if(m->harvestClock<45)return;m->harvestClock=0;int n=forage_node(k);if(n<0)return;Node*v=&g.nodes[n];int item=v->type==NODE_WOOD?WOOD:v->type==NODE_COPPER?COPPER:ORE;
    for(int i=0;i<e->count;i++){Actor*a=member(k,i);if(a&&!a->dead&&a->unconscious<=0&&inventory_add(a,item,1)){v->remaining--;m->harvests++;break;}}
}
static void shift_group(int k,float dx,float dy){
    WorldEvent*e=&P.groups[k];e->x+=dx;e->y+=dy;
    if(!e->spawned)for(int i=0;i<e->count;i++){Actor*a=member(k,i);if(!a||a->dead)continue;a->x+=dx;a->y+=dy;clear_position(&a->x,&a->y);a->homeX=a->tx=a->x;a->homeY=a->ty=a->y;}
}
static void population_clash(int a,int b) {
    PopulationState *m=&P.state[a], *n=&P.state[b];
    if (m->opponent==b && n->opponent==a)
        return;
    const WorldEvent *e=&P.groups[a], *f=&P.groups[b];
    P.conflicts++;
    m->fights++;
    n->fights++;
    m->opponent=b;
    n->opponent=a;
    war_report_clash(e->faction,f->faction,(e->x+f->x)*.5f,(e->y+f->y)*.5f);
}
#include "population_behavior.h"
void population_detain(int n) {
    int k=population_owned(n)-1;
    if (k<0)
        return;
    WorldEvent *e=&P.groups[k];
    GroupLife *z=&g.life.groups[k];
    for (int i=0;i<e->count;i++) {
        if (e->members[i]!=n)
            continue;
        for (int j=0;j<e->count;j++)
            if (z->carry[j]>=0 && (j==i||z->carry[j]==i))
                life_drop(k,j);
        P.state[k].detached|=1<<i;
        e->members[i]=-1;
        if (g.actors[n].dead)
            release_slot(n);
        else
            g.living.owned[n]=0;
        return;
    }
}
static void offline_battle(int a,int b,float dt){
    WorldEvent*e=&P.groups[a],*f=&P.groups[b];PopulationState*m=&P.state[a],*n=&P.state[b];
    if((g.life.groups[a].goal==LIFE_AMBUSH&&!g.life.groups[a].ambushSprung)||(g.life.groups[b].goal==LIFE_AMBUSH&&!g.life.groups[b].ambushSprung))return;
    if(e->spawned||f->spawned||fabsf(e->x-f->x)>9||fabsf(e->y-f->y)>9||
       !enemies(e,f)||distance(e->x,e->y,f->x,f->y)>9||observers(e->x,e->y)<80||observers(f->x,f->y)<80)return;
    int alive[2]={standing(a),standing(b)};if(!alive[0]||!alive[1])return;
    population_clash(a,b);m->phase=n->phase=POP_FIGHT;m->clock=n->clock=15;
    for(int side=0;side<2;side++){
        int own=side?b:a,other=side?a:b;WorldEvent*att=&P.groups[own],*def=&P.groups[other];float strength=0;
        for(int i=0;i<att->count;i++){Actor*x=member(own,i);if(x&&!x->dead&&x->unconscious<=0)strength+=4+level(x,MELEE)*.13f+itemDefs[x->gear[0]].power*.08f;}
        int start=random_step(&att->seed)%def->count;
        for (int q = 0; q < def->count; q++) {
            Actor *x = member(other, (start + q) % def->count);
            if (!x || x->dead || x->unconscious > 0)
                continue;
            int l = random_step(&att->seed) % 5 == 0 ? HEAD : CHEST;
            float damage = fminf(25, strength * dt * .09f / (1 + itemDefs[x->gear[1]].armor * .025f));
            x->health[l] -= damage;
            x->treatedMask &= ~(1 << l);
            x->woundFlags &= ~(1u << (WOUND_REPAIR_SHIFT + l));
            if (depth_organic(x, l))
                x->bleed += damage * .003f;
            if (x->health[l] <= 0)
                x->unconscious = 90;
            break;
        }
    }
}

static float refill_delay(const WorldEvent *e) {
    return DAY_SECONDS/8 + (unsigned)e->serial * 37u % 150;
}

static int open_encounter_site(EncounterSite site,int slot) {
    const float x=site.x,y=site.y;
    const WorldEvent *old=&P.groups[slot];
    if (x<12||y<12||x>WORLD-12||y>WORLD-12||town_near(x,y,28)>=0||
        (g.baseExists&&distance(x,y,g.baseX,g.baseY)<35)||
        distance(x,y,old->x,old->y)<38||observers(x,y)<68)
        return 0;
    for(int k=0;k<POP_GROUPS;k++){
        if(k==slot)continue;
        const WorldEvent *e=&P.groups[k];
        if(fabsf(x-e->x)<32&&fabsf(y-e->y)<32&&distance(x,y,e->x,e->y)<32&&
           (e->presence<refill_delay(e)||life_alive(k)))return 0;
        const PopulationState *m=&P.state[k];
        if(fabsf(x-m->siteX)<26&&fabsf(y-m->siteY)<26&&population_object_count(k)&&
           distance(x,y,m->siteX,m->siteY)<26)return 0;
    }
    for(int k=0;k<EVENTS;k++){
        const WorldEvent *e=&g.living.events[k];
        if(!e->status&&e->count&&fabsf(x-e->x)<32&&fabsf(y-e->y)<32&&
           distance(x,y,e->x,e->y)<32)return 0;
    }
    return can_walk(x,y)&&can_walk(x-4,y)&&can_walk(x+4,y)&&
           can_walk(x,y-3)&&can_walk(x,y+3);
}

static void single_encounter(int slot,EncounterSite site) {
    static const int traffic[]={POP_RAIDERS,POP_RAIDERS,POP_RAIDERS,POP_RAIDERS,POP_BEASTS,POP_BEASTS,POP_RAIDERS,POP_BEASTS,
        POP_MERCHANTS,POP_MERCHANTS,POP_PILGRIMS,POP_PILGRIMS,POP_PATROL,POP_PATROL,POP_HUNTERS,
        POP_SCAVENGERS,POP_SMUGGLERS,POP_REFUGEES,POP_BATTLEFIELD,POP_PRISONERS,POP_DUELISTS,POP_ARMY};
    static const int camps[]={POP_SCAVENGERS,POP_REFUGEES,POP_HUNTERS,POP_HUNTERS,POP_SMUGGLERS,
        POP_MERCHANTS,POP_WOUNDED,POP_AMBUSH,POP_SALVAGE,POP_SALVAGE};
    unsigned roll=random_step(&P.rng);
    int type=traffic[roll%(sizeof(traffic)/sizeof(*traffic))];
    int safe=starter_country(site.x,site.y);
    if ((roll>>24)%(passing_group(slot)?5:4)==0) {
        type=camps[(roll>>8)%(sizeof(camps)/sizeof(*camps))];
        if (safe && type==POP_AMBUSH) type=POP_SCAVENGERS;
        site.beds=2+(roll>>16)%2;
    } else if (safe && (type==POP_ARMY||type==POP_DUELISTS||type==POP_PRISONERS)) {
        type=POP_RAIDERS;
    }
    generate_population(slot,type,&site);
}

static int quiet_route(const Actor *lead,float dx,float dy,int road) {
    int nearby=0;
    for(int k=0;k<POP_GROUPS+EVENTS;k++){
        const WorldEvent *e=k<POP_GROUPS?&P.groups[k]:&g.living.events[k-POP_GROUPS];
        float x=e->x-lead->x,y=e->y-lead->y;
        if(fabsf(x)>140||fabsf(y)>140||x*x+y*y>140*140)continue;
        if(k<POP_GROUPS?!life_alive(k):e->status||!e->count)continue;
        float forward=x*dx+y*dy,side=x*dy-y*dx;
        if (++nearby >= 4 || (forward >= -10 && forward < 105 && fabsf(side) < (road ? 25 : 33)))
            return 0;
    }
    return 1;
}

static int passing_can_leave(int k) {
    const WorldEvent *e=&P.groups[k];
    const GroupLife *z=&g.life.groups[k];
    const FieldSighting *seen=&g.fieldNotes.group[k];
    if(!passing_group(k)||P.state[k].phase==POP_FIGHT||e->age<DAY_SECONDS/10||
       (camp_type(k)&&z->camp!=CAMP_ABANDONED)||
       (!z->arrivals&&e->age<DAY_SECONDS/6)||
       (z->goal!=LIFE_ROAM&&(z->goal!=LIFE_RECOVER||!z->arrivals)&&z->camp!=CAMP_ABANDONED)||
       (seen->serial==e->serial&&g.time-seen->time<DAY_SECONDS/3))
        return 0;
    for(int i=0;i<e->count;i++){
        const Actor *a=member(k,i);
        if(a&&!a->dead&&(a->unconscious>0||a->prisonTime>0||
                         e->reserveDepth[i].captive||e->reserveDepth[i].room))
            return 0;
    }
    return 1;
}
static int reusable(int k,float near) {
    const WorldEvent *e=&P.groups[k];
    if(e->spawned||near<=105||legion_group(k)||warrant_preserve(k))
        return 0;
    if(P.state[k].phase==POP_RUINS)
        return e->presence>=refill_delay(e);
    return near>150&&passing_can_leave(k);
}
static int encounter_partner(const float *near,int first) {
    for (int k=0;k<POP_GROUPS;k++)
        if (k!=first&&reusable(k,near[k]))
            return k;
    return -1;
}
static void pursue_group(int k,int target) {
    GroupLife *z=&g.life.groups[k];
    WorldEvent *other=&P.groups[target];
    z->goal=LIFE_HUNT;
    z->targetGroup=target;
    z->targetSerial=other->serial;
    P.state[k].targetX=other->x;
    P.state[k].targetY=other->y;
}
static int encounter_pair(int first,int second,EncounterSite site,unsigned roll) {
    int starter=starter_country(site.x,site.y);
    int scenario=(roll>>9)%3;
    if (starter && scenario==1)
        scenario=0;
    EncounterSite other={0};
    int placed=0;
    for (int try=0;try<8;try++) {
        float angle=(roll%628)*.01f+try*.785398f;
        float dx=cosf(angle)*21, dy=sinf(angle)*21;
        other.x=site.x+dx;
        other.y=site.y+dy;
        if (!open_encounter_site(other,second) ||
            (scenario==1 && starter_country(other.x,other.y)) ||
            terrain_water(site.x+dx*.25f,site.y+dy*.25f) ||
            terrain_water(site.x+dx*.50f,site.y+dy*.50f) ||
            terrain_water(site.x+dx*.75f,site.y+dy*.75f))
            continue;
        placed=1;
        break;
    }
    if (!placed)
        return 0;
    if (scenario==0) {
        site.size=3+(roll>>15)%3;
        other.size=2+(roll>>19)%4;
        generate_population(first,POP_HUNTERS,&site);
        generate_population(second,POP_BEASTS,&other);
        pursue_group(first,second);
    } else if (scenario==1) {
        int side=(roll>>15)%WAR_FACTIONS;
        site.faction=forces[side];
        other.faction=forces[(side+1+(roll>>19)%2)%WAR_FACTIONS];
        site.size=4+(roll>>20)%3;
        other.size=4+(roll>>23)%3;
        generate_population(first,POP_PATROL,&site);
        generate_population(second,POP_PATROL,&other);
        pursue_group(first,second);
        pursue_group(second,first);
    } else {
        other.size=3+(roll>>15)%3;
        generate_population(first,POP_RAIDERS,&site);
        generate_population(second,(roll>>19)%2 ? POP_MERCHANTS : POP_PILGRIMS,&other);
        pursue_group(first,second);
    }
    return 1;
}
static void roll_encounter(int first,EncounterSite site,const float *near,int road) {
    unsigned roll=random_step(&P.rng);
    if (roll % 100 < (road ? 46 : 36)) {
        int second=encounter_partner(near,first);
        if (second>=0&&encounter_pair(first,second,site,roll))
            return;
    }
    single_encounter(first,site);
}

static void refresh_encounters(const float *near) {
    int cleared=-1,passing=-1;
    for(int k=0;k<POP_GROUPS;k++){
        const WorldEvent *e=&P.groups[k];
        if(!reusable(k,near[k]))continue;
        if(P.state[k].phase==POP_RUINS){
            if(cleared<0||e->presence>P.groups[cleared].presence)
                cleared=k;
        }else{
            if(passing<0||e->age>P.groups[passing].age)passing=k;
        }
    }
    const Actor *lead=&g.actors[g.selected];
    int slot=cleared>=0?cleared:passing;
    if(slot>=0&&lead->movedTimer>0&&conscious(lead)&&!actor_inside(lead)&&
       town_near(lead->x,lead->y,24)<0){
        float sx,sy,dx,dy;
        walk_heading(lead,&sx,&sy);
        view_to_world(sx,sy,&dx,&dy);
        float len=sqrtf(dx*dx+dy*dy);
        if(len>.01f){
            dx/=len;dy/=len;
            int road = terrain_road(lead->x, lead->y);
            if(quiet_route(lead,dx,dy,road)){
                for(int tries=0;tries<10;tries++){
                    float ahead=62+random_step(&P.rng)%29;
                    float side=(int)(random_step(&P.rng)%37)-18;
                    EncounterSite site={lead->x+dx*ahead-dy*side,lead->y+dy*ahead+dx*side};
                    if(!open_encounter_site(site,slot))continue;
                    int reachable=1;
                    for(int sample=1;sample<5;sample++){
                        float u=sample*.2f;
                        if(terrain_water(lead->x+(site.x-lead->x)*u,lead->y+(site.y-lead->y)*u)){
                            reachable=0;break;
                        }
                    }
                    if(reachable){
                        roll_encounter(slot,site,near,road);
                        return;
                    }
                }
            }
        }
    }
    if(cleared<0)return;
    const WorldEvent *old=&P.groups[cleared];
    int town=random_step(&P.rng)%TOWNS;
    for(int tries=0;tries<12;tries++){
        float x=tries<8?old->x:towns[town].x,y=tries<8?old->y:towns[town].y;
        float angle=(random_step(&P.rng)%628)*.01f,r=46+random_step(&P.rng)%50;
        EncounterSite site={x+cosf(angle)*r,y+sinf(angle)*r};
        if(open_encounter_site(site,cleared)){
            roll_encounter(cleared,site,near,0);
            return;
        }
    }
}

void population_tick(float dt){
    if(!P.version)return;if(!g.life.version)life_init();P.pulse+=dt;P.offlinePulse+=dt;if(P.pulse<1)return;float step=fminf(5,P.pulse);P.pulse=0;
    /* Unload before allocating; all members of all player squads keep their area alive. */
    for(int k=0;k<POP_GROUPS;k++){WorldEvent*e=&P.groups[k];detach_followers(k);if(e->spawned&&observers(e->x,e->y)>95&&(!population_object_count(k)||observers(P.state[k].siteX,P.state[k].siteY)>80))population_store(k);}
    int order[POP_GROUPS];float near[POP_GROUPS];
    for(int k=0;k<POP_GROUPS;k++){order[k]=k;near[k]=observers(P.groups[k].x,P.groups[k].y);
        if(population_object_count(k))near[k]=fminf(near[k],observers(P.state[k].siteX,P.state[k].siteY));
        for(int i=0;i<P.groups[k].count;i++){Actor*a=member(k,i);if(a&&(a->dead||a->unconscious>0))near[k]=fminf(near[k],observers(a->x,a->y));}}
    for(int i=1;i<POP_GROUPS;i++){int k=order[i],j=i;while(j>0&&near[order[j-1]]>near[k]){order[j]=order[j-1];j--;}order[j]=k;}
    for(int j=0;j<POP_GROUPS;j++){int k=order[j];if(near[k]<55){population_materialize(k);if(near[k]<14)field_observe(k);}}
    for(int k=0;k<POP_GROUPS;k++){WorldEvent*e=&P.groups[k];physiology(k,step);group_behavior(k,step);forage_offline(k,step);
        if(legion_group(k)&&P.state[k].phase==POP_RUINS&&e->presence>DAY_SECONDS*4&&
           !e->spawned&&near[k]>120)population_generate(k,POP_ARMY);
        if(e->spawned&&e->type==POP_PRISONERS&&P.state[k].phase==POP_TRAVEL)for(int i=e->count-2;i<e->count;i++){Actor*a=member(k,i);if(a&&!a->dead&&a->unconscious<=0&&g.depth.actors[e->members[i]].captive){float x=e->x-2+i%2,y=e->y+3;route_toward(a,x,y,step*.55f);}}
    }
    if(P.offlinePulse>=5){
        float battleStep=fminf(5,P.offlinePulse);P.offlinePulse=0;
        for(int a=0;a<POP_GROUPS;a++)for(int b=a+1;b<POP_GROUPS;b++)offline_battle(a,b,battleStep);
        refresh_encounters(near);
    }
}
int population_walk(int n,float dt){
    int owned=population_owned(n);if(!owned)return 0;int k=owned-1;WorldEvent*e=&P.groups[k];PopulationState*m=&P.state[k];Actor*a=&g.actors[n];
    if(a->squad||g.society.freed[n]||merc_slot(a)>=0)return 0;int i=0;while(i<e->count&&e->members[i]!=n)i++;if(i==e->count)return 1;
    float x,y;
    if(e->type==POP_ARMY||e->type==POP_PATROL){float dx=m->targetX-e->x,dy=m->targetY-e->y,d=fmaxf(1,sqrtf(dx*dx+dy*dy));float side=(i%3-1)*2.2f,back=(i/3)*2.3f;x=e->x-dy/d*side-dx/d*back;y=e->y+dx/d*side-dy/d*back;}
    else {float angle=i*2.39996f+e->serial;x=e->x+cosf(angle)*(2+i%3);y=e->y+sinf(angle)*(2+i%3);}
    if(camp_type(k)&&g.life.groups[k].goal==LIFE_ROAM&&g.life.groups[k].camp==CAMP_READY){
        if (m->phase == POP_REST && i < m->beds - 1) {
            x = m->siteX + m->offsetX[i + 1];
            y = m->siteY + m->offsetY[i + 1];
            if (distance(a->x, a->y, x, y) < .8f && room_bed_free(a, x, y, 0, 0)) {
                place_in_bed(a, -1000 - k * 4 - i, x, y, 0);
                a->action = 180;
                return 1;
            }
        }
        else if(m->phase==POP_FORAGE){float angle=(i*1.7f+e->serial);x=e->x+cosf(angle)*(8+i%3*3);y=e->y+sinf(angle)*(8+i%3*3);
            if(i<2&&(e->type==POP_SCAVENGERS||e->type==POP_SALVAGE)){int node=forage_node(k);if(node>=0){x=g.nodes[node].x;y=g.nodes[node].y;if(distance(a->x,a->y,x,y)<=WORK_REACH){if(a->taskKind!=1){a->taskKind=1;a->taskIndex=node;a->action=2;}return 1;}}}
        }
        else {float angle=i*1.7f+e->serial;x=e->x+cosf(angle)*4;y=e->y+sinf(angle)*4;}
    }
    if(distance(a->x,a->y,x,y)>1){a->taskKind=0;route_toward(a,x,y,dt);}return 1;
}
int population_hostile(const Actor*a,const Actor*b){
    int ai=actor_no(a),bi=actor_no(b),pa=population_owned(ai),pb=population_owned(bi);
    if(pa&&g.life.version&&g.life.groups[pa-1].goal==LIFE_RAID&&g.life.groups[pa-1].targetTown==b->homeTown&&b->homeTown>=0&&b->homeTown<TOWNS&&b->faction==g.townOwner[b->homeTown]&&(b->kind==GUARD||!war_rivals(a->faction,b->faction)))return 1;
    if(pb&&g.life.version&&g.life.groups[pb-1].goal==LIFE_RAID&&g.life.groups[pb-1].targetTown==a->homeTown&&a->homeTown>=0&&a->homeTown<TOWNS&&a->faction==g.townOwner[a->homeTown]&&(a->kind==GUARD||!war_rivals(a->faction,b->faction)))return 1;
    if(pa&&pb)return enemies(&P.groups[pa-1],&P.groups[pb-1]);return 0;
}
int population_bandits(float x,float y,float *rx,float *ry){
    float nearest=42;
    int found=0;
    for(int k=0;k<POP_GROUPS;k++){
        WorldEvent *e=&P.groups[k];
        if(e->type!=POP_RAIDERS)continue;
        float d=distance(x,y,e->x,e->y);
        if(d>=nearest||!standing(k))continue;
        nearest=d;*rx=e->x;*ry=e->y;found=1;
    }
    for(int i=0;i<g.actorCount;i++){
        Actor *a=&g.actors[i];
        if(a->kind!=RAIDER||a->faction!=HOSTILE||a->squad||!conscious(a)||population_owned(i)||actor_inside(a))continue;
        float d=distance(x,y,a->x,a->y);
        if(d<nearest){nearest=d;*rx=a->x;*ry=a->y;found=1;}
    }
    return found;
}
void population_anger(int n){int pa=population_owned(n);if(!pa)return;WorldEvent*e=&P.groups[pa-1];for(int i=0;i<e->count;i++)if(e->members[i]>=0)g.living.anger[e->members[i]]=1;}
int population_object_count(int k){
 if(k<0||k>=POP_GROUPS||!P.version)return 0;WorldEvent*e=&P.groups[k];
 if(g.life.version&&g.life.groups[k].camp>=CAMP_AWAY)return 0;
 return camp_type(k)?P.state[k].beds+5:e->type==POP_BATTLEFIELD?1:0;
}
int population_object(int k,int object,float*x,float*y){
 int count=population_object_count(k);if(object<0||object>=count)return 0;PopulationState*m=&P.state[k];
  if(P.groups[k].type==POP_BATTLEFIELD){*x=m->siteX+m->offsetX[4];*y=m->siteY+m->offsetY[4];return 3;}
 if(object<=m->beds){*x=m->siteX+m->offsetX[object];*y=m->siteY+m->offsetY[object];return object?2:1;}
 if(object==m->beds+1){*x=m->siteX+m->offsetX[4];*y=m->siteY+m->offsetY[4];return 3;}
 int style=g.life.groups[k].layout;
 if(object==m->beds+2){*x=m->siteX-4;*y=m->siteY-2;return 4;}
 *x=m->siteX+(object==m->beds+3?5:-5);*y=m->siteY+(style==1?2:-3);return object==m->beds+3?5:6;
}
int population_prop_art(int k,int object){float x,y;int type=population_object(k,object,&x,&y);return type==1?27:type==2?ART_BEDROLL:type==4?26:type==5?(P.groups[k].type==POP_HUNTERS?18:P.groups[k].type==POP_SALVAGE?17:20):31;}
int population_prop_style(int k,int object){float x,y;return population_object(k,object,&x,&y)==6?(military(P.groups[k].type)?2:3):0;}
int population_sleep_position(const Actor *a, float *x, float *y) {
    if (a->taskIndex > -1000)
        return 0;
    int raw = -1000 - a->taskIndex, k = raw / 4, bed = raw % 4 + 1;
    return k < POP_GROUPS && bed <= P.state[k].beds &&
        population_object(k, bed, x, y) == 2 && distance(a->x, a->y, *x, *y) < 2;
}
int population_sleep_valid(const Actor *a) {
    float x, y;
    return population_sleep_position(a, &x, &y);
}
int population_action_valid(int k,int action){
    if(k<0||k>=POP_GROUPS||!P.version||depth_room(&g.actors[g.selected]))return 0;Actor*a=&g.actors[g.selected];WorldEvent*e=&P.groups[k];float x=e->x,y=e->y;
    if(!conscious(a))return 0;if(!action)return distance(a->x,a->y,e->x,e->y)<=8;
    int object=action>=11&&action<=13?action-10:action==20?(camp_type(k)?P.state[k].beds+1:0):0;
    if(!population_object(k,object,&x,&y))return 0;
    if(action>=30&&(e->type!=POP_MERCHANTS&&e->type!=POP_SMUGGLERS))return 0;
    return distance(a->x,a->y,x,y)<=1.7f;
}
void population_detail(int k,char*out,int size){
    if(k<0||k>=POP_GROUPS){snprintf(out,size,"No travelers here.");return;}WorldEvent*e=&P.groups[k];PopulationState*m=&P.state[k];int live=0,down=0;for(int i=0;i<e->count;i++){Actor*a=member(k,i);if(a&&!a->dead){live++;down+=a->unconscious>0;}}
    snprintf(out,size,"%s: %d alive, %d down; %s. %s",population_name(k),live,down,g.life.version?life_goal_name(g.life.groups[k].goal):phases[m->phase],m->beds?"Camp beds are available when unoccupied.":e->type==POP_PRISONERS?"Prisoners can be freed after evading or defeating guards.":e->type==POP_WOUNDED?"Dressings may save the wounded.":e->type==POP_BATTLEFIELD?"The survivors and fallen carry their actual equipment.":"These travelers follow their own route.");
}
static int cache_owner(const WorldEvent *e) {
    return e->type == POP_BATTLEFIELD || e->faction == HOSTILE ? FREE : e->faction;
}
static const int traderItems[] = {RATION, MEDKIT, PARTS}, traderPrices[] = {40, 80, 100};
const char *population_action_reason(int k, int action) {
    if (!population_action_valid(k, action))
        return "Out of reach";
    Actor *a = &g.actors[g.selected];
    WorldEvent *e = &P.groups[k];
    PopulationState *m = &P.state[k];
    if (action == 1) {
        if (inventory_count(a, MEAT) < 2)
            return "Need 2 meat";
        Actor test = *a;
        int owner = inventory_input_owner(&test, MEAT, 2);
        inventory_remove(&test, MEAT, 2);
        if (!inventory_add_owned(&test, RATION, 1, owner))
            return "Pack full";
    } else if (action >= 11 && action <= 13) {
        float x, y;
        population_object(k, action - 10, &x, &y);
        if (!room_bed_free(a, x, y, 0, 0))
            return "Bedroll occupied";
    } else if (action == 20) {
        if (!m->cacheQty)
            return "Supplies already taken";
        Actor test = *a;
        if (!inventory_add_owned(&test, m->cacheItem, m->cacheQty, cache_owner(e)))
            return "Pack full";
    } else if (action >= 30 && action <= 32) {
        int i = action - 30;
        if (g.rep[e->faction] < -25 || !standing(k))
            return "No willing trader";
        if (!m->stock[i])
            return "Out of stock";
        if (g.money < traderPrices[i])
            return "Not enough silver";
        Actor test = *a;
        if (!inventory_add(&test, traderItems[i], 1))
            return "Pack full";
    }
    return NULL;
}
void population_action_label(int k,int action,char*out,int size){
    if (!action) snprintf(out,size,"Observe %s",P.groups[k].type == POP_SMUGGLERS ? "smugglers" : population_name(k));
    else if(action>=11&&action<=13)snprintf(out,size,"Rest on camp bedroll %d",action-10);
    else if(action==1)snprintf(out,size,"Cook at fire: 2 meat > 1 ration");
    else if (action == 20) {
        int owner = cache_owner(&P.groups[k]);
        snprintf(out, size, owner ? "Steal %s camp supplies" : "Search abandoned camp supplies",
                 factionNames[owner]);
    } else {
        int i = bounded(action - 30, 0, 2);
        snprintf(out, size, "Buy %s: %d silver (%d left)", itemDefs[traderItems[i]].name,
                 traderPrices[i], P.state[k].stock[i]);
    }
}
int population_action(int k, int action) {
    const char *reason = population_action_reason(k, action);
    if (reason) {
        log_msg("%s.", reason);
        return 0;
    }
    Actor *a = &g.actors[g.selected];
    WorldEvent *e = &P.groups[k];
    PopulationState *m = &P.state[k];
    if (action && !e->spawned && standing(k) && !population_materialize(k)) {
        log_msg("Camp activity is unavailable.");
        return 0;
    }
    if (!action) {
        log_msg("%s: %d standing / %s / %d beds.",
                e->type == POP_SMUGGLERS && !population_object_count(k) ? "Smugglers" : population_name(k), standing(k),
                g.life.version ? life_goal_name(g.life.groups[k].goal) : phases[m->phase], m->beds);
    } else if (action >= 11 && action <= 13) {
        float x, y;
        if (population_object(k, action - 10, &x, &y) != 2)
            return 0;
        return rest_at(a, -1000 - k * 4 - (action - 11), x, y, 0, 0);
    } else if (action == 1) {
        int owner = inventory_input_owner(a, MEAT, 2);
        inventory_remove(a, MEAT, 2);
        inventory_add_owned(a, RATION, 1, owner);
        gain(a, COOKING, 12);
        log_msg("Cooked 1 ration from 2 meat.");
    } else if (action == 20) {
        int owner = cache_owner(e);
        inventory_add_owned(a, m->cacheItem, m->cacheQty, owner);
        m->cacheQty = 0;
        if (owner)
            witnessed_theft(a, owner, 7, 12);
        else
            log_msg("Took the remaining camp supplies.");
    } else if (action >= 30 && action <= 32) {
        int i = action - 30;
        inventory_add(a, traderItems[i], 1);
        g.money -= traderPrices[i];
        m->stock[i]--;
        trade_goodwill(e->faction, traderPrices[i]);
        log_msg("Bought %s for %d silver.", itemDefs[traderItems[i]].name, traderPrices[i]);
    } else
        return 0;
    return 1;
}
const char *population_actor_role(int n){
 static const char *roles[POP_TYPES]={"SCAVENGER","REFUGEE","HUNTER","SMUGGLER","TRADER","PATROL SOLDIER","BANDIT","BEAST","PILGRIM","WOUNDED TRAVELER","FALLEN SOLDIER","SOLDIER","BANDIT","SALVAGER","CONVOY GUARD","ELITE DUELIST"};
 int k=population_owned(n);if(!k)return NULL;WorldEvent*e=&P.groups[k-1];
 if(g.depth.actors[n].captive)return "PRISONER";
 if((e->type==POP_RAIDERS||e->type==POP_AMBUSH)&&e->modifier==1)return "STARVING BANDIT";
 return roles[e->type];
}
const char *population_dialogue(int n) {
    int k = population_owned(n) - 1;
    if (k < 0)
        return NULL;
    const WorldEvent *e = &P.groups[k];
    if (population_vendor(n))
        return e->type == POP_SMUGGLERS ? "Keep your voice down. I've a few things the watch doesn't need to see." :
               "I can spare some of what's in my pack. Have a look before we move on.";
    static const char *lines[] = {
        "We're looking for scrap the other crews missed.", "We left with what we could carry.",
        "Keep your voice down. Something's been following us.", "I'm here to watch the road. Talk to the one with the pack.",
        "We're moving on soon. The road gets worse after dark.", "We've got orders to keep this road open.",
        "Keep walking.", "The animal bares its teeth.", "There should be shelter beyond the next ridge.",
        "Got a dressing to spare?", "I haven't counted the dead. I don't want to.",
        "Stay clear of the column.", "You've come a long way out here.",
        "Mind the ruins. The floor gives out in places.", "Keep back from the prisoners.",
        "Give me room. I'm waiting for someone."
    };
    return lines[e->type];
}
int population_validate(const Game*s){
    const Population*p=&s->population;if(!p->version){for(int n=0;n<s->actorCount;n++)if(s->living.owned[n]>EVENTS)return 0;return 1;}if(p->version<1||p->version>2||p->serial<0||!isfinite(p->pulse)||!isfinite(p->offlinePulse)||p->pulse<0||p->offlinePulse<0)return 0;
    int used[MAX_ACTORS]={0};
    int groups=p->version==2?POP_GROUPS:s->war.version?WAR_POP_GROUPS:OLD_POP_GROUPS;
    for(int k=0;k<groups;k++){const WorldEvent*e=&p->groups[k];const PopulationState*m=&p->state[k];if(e->type<0||e->type>=POP_TYPES||e->count<1||e->count>POP_MEMBERS||e->tier<0||e->tier>4||e->faction<0||e->faction>=FACTIONS||e->town<0||e->town>=TOWNS||e->origin<0||e->origin>=TOWNS||e->spawned<0||e->spawned>1||e->stored!=1||e->serial<=0||m->generation!=e->serial||m->phase<0||m->phase>POP_RUINS||m->beds<0||m->beds>3||m->detached&~65535||m->cacheItem<0||m->cacheItem>=ITEMS||m->cacheQty<0||m->cacheQty>999)return 0;
        const float coords[]={e->x,e->y,m->targetX,m->targetY,m->siteX,m->siteY};for(int j=0;j<6;j++)if(!isfinite(coords[j])||coords[j]<0||coords[j]>WORLD)return 0;if(m->opponent < -1||m->opponent>=POP_GROUPS||!isfinite(m->harvestClock)||m->harvestClock<0||!isfinite(e->age)||e->age<0||!isfinite(e->presence)||e->presence<0||!isfinite(m->clock))return 0;
        for(int j=0;j<3;j++)if(m->stock[j]<0||m->stock[j]>999)return 0;for(int j=0;j<5;j++)if(!isfinite(m->offsetX[j])||!isfinite(m->offsetY[j])||fabsf(m->offsetX[j])>50||fabsf(m->offsetY[j])>50)return 0;
        for(int i=0;i<e->count;i++){if(m->detached&(1<<i)){if(e->members[i]!=-1)return 0;continue;}const Actor*a=&e->reserve[i];const DepthActor*z=&e->reserveDepth[i];if(e->spawned){int n=e->members[i];if(n<0||n>=s->actorCount||used[n]++||s->living.owned[n]!=EVENTS+k+1)return 0;a=&s->actors[n];z=&s->depth.actors[n];}else if(e->members[i]!=-1)return 0;
            if(!memchr(a->name,0,16)||a->kind<0||a->kind>CARAVAN||a->race<0||a->race>=SPECIES||a->faction<0||a->faction>=FACTIONS||!isfinite(a->blood)||!isfinite(a->bleed)||!isfinite(a->hunger)||!isfinite(a->unconscious)||!isfinite(a->x)||!isfinite(a->y)||a->x<0||a->y<0||a->x>WORLD||a->y>WORLD||z->layer<0||z->layer>=ITEMS||!limb_actor_valid(a))return 0;
            for(int l=0;l<LIMBS;l++)if(!isfinite(a->health[l])||!isfinite(z->trauma[l])||z->trauma[l]<0||z->trauma[l]>80)return 0;
            for(int j=0;j<PACK;j++)if(a->pack[j].item<0||a->pack[j].item>=ITEMS||a->pack[j].qty<0||a->pack[j].qty>999||a->stolen[j]<0||a->stolen[j]>=FACTIONS)return 0;
            for(int j=0;j<GEARS;j++)if(a->gear[j]<0||a->gear[j]>=ITEMS||a->gearStolen[j]<0||a->gearStolen[j]>=FACTIONS)return 0;
            for(int j=0;j<SKILLS;j++)if(!isfinite(a->xp[j])||a->xp[j]<0)return 0;
            const PopulationSocial*so=&p->social[k][i];for(int j=0;j<WEAPON_XP_SLOTS;j++)if(!isfinite(so->weaponXP[j])||so->weaponXP[j]<0||so->weaponXP[j]>SKILL_XP_CAP)return 0;for(int j=0;j<FACTIONS;j++)if(so->wanted[j]<0||so->wanted[j]>100000||!isfinite(so->recognized[j]))return 0;if(so->cannibal<0||so->cannibal>2||so->captorFaction<0||so->captorFaction>=FACTIONS)return 0;
        }
    }
    for(int n=0;n<s->actorCount;n++)if(s->living.owned[n]>EVENTS&&!used[n])return 0;return 1;
}
