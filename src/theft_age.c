#include "game.h"
#include <math.h>
#include <stdint.h>

static int actor_index(const Actor *a) {
    uintptr_t p=(uintptr_t)a, start=(uintptr_t)g.actors;
    return p>=start && p<start+sizeof(g.actors) ? (int)(a-g.actors) : -1;
}
static int raw_owner(const Actor *a,int slot) {
    int n=actor_index(a);
    if(slot<0||slot>=LOOT_SLOTS)return 0;
    if(slot<PACK)return a->stolen[slot];
    if(slot<PACK+GEARS)return a->gearStolen[slot-PACK];
    if(n<0)return 0;
    if(slot==LOOT_LAYER_SLOT)return g.depth.actors[n].layerOwner;
    return limb_owner(a,LARM+slot-PACK-GEARS);
}
static void clear_owner(Actor *a,int slot) {
    int n=actor_index(a);
    if(slot<PACK)a->stolen[slot]=0;
    else if(slot<PACK+GEARS)a->gearStolen[slot-PACK]=0;
    else if(slot==LOOT_LAYER_SLOT)g.depth.actors[n].layerOwner=0;
    else {int l=LARM+slot-PACK-GEARS;limb_set(a,l,limb_installed(a,l),0);}
}
static float stamp(TheftStamp *s,int item,int owner,float until) {
    unsigned key=item>0&&owner>0?(unsigned)item*FACTIONS+owner:0;
    if(!key){*s=(TheftStamp){0};return 0;}
    if(until>0||s->key!=key){s->key=key;s->until=until>0?until:g.time+2*DAY_SECONDS;}
    return s->until;
}
float stolen_until(const Actor *actor,int slot) {
    int n=actor_index(actor);
    if(n<0||slot<0||slot>=LOOT_SLOTS)return 0;
    Actor *a=&g.actors[n];
    TheftStamp *s=&g.repairs.actor[n][slot];
    float until=stamp(s,limb_loot_item(a,slot),raw_owner(a,slot),0);
    if(until>0&&g.time>=until){clear_owner(a,slot);*s=(TheftStamp){0};return 0;}
    return until;
}
void stolen_set(Actor *a,int slot,float until) {
    int n=actor_index(a);if(n<0||slot<0||slot>=LOOT_SLOTS)return;
    stamp(&g.repairs.actor[n][slot],limb_loot_item(a,slot),raw_owner(a,slot),until);
}
float stolen_home_until(int slot) {
    if(slot<0||slot>=HOME_STASH)return 0;
    TheftStamp *s=&g.repairs.home[slot];
    float until=stamp(s,g.homes.stash[slot].item,g.homes.stolen[slot],0);
    if(until>0&&g.time>=until){g.homes.stolen[slot]=0;*s=(TheftStamp){0};return 0;}
    return until;
}
void stolen_home_set(int slot,float until) {
    if(slot>=0&&slot<HOME_STASH)stamp(&g.repairs.home[slot],g.homes.stash[slot].item,g.homes.stolen[slot],until);
}
float stolen_drop_until(int slot) {
    if(slot<0||slot>=g.nodeCount)return 0;
    Node *n=&g.nodes[slot];TheftStamp *s=&g.repairs.drop[slot];
    float until=stamp(s,n->type==NODE_DROPPED&&n->remaining?n->item:0,n->owner,0);
    if(until>0&&g.time>=until){n->owner=0;*s=(TheftStamp){0};return 0;}
    return until;
}
void stolen_drop_set(int slot,float until) {
    if(slot>=0&&slot<g.nodeCount)stamp(&g.repairs.drop[slot],g.nodes[slot].item,g.nodes[slot].owner,until);
}
int stolen_age_owner(int owner,float *until){
 if(owner<=0){*until=0;return owner;}
 if(*until<=0)*until=g.time+2*DAY_SECONDS;
 if(g.time>=*until){*until=0;return 0;}return owner;
}
float stolen_property_until(int camp,int slot){
 if(camp<0||camp>=SLAVE_CAMPS||slot<0||slot>=SEIZED_STACKS)return 0;
 float *until=&g.repairs.seizedUntil[camp][slot];
 g.field.seizedOwner[camp][slot]=stolen_age_owner(g.field.seizedOwner[camp][slot],until);
 return *until;
}
void stolen_tick(void) {
    /* Ownership is also expired on access; the sweep handles unattended items. */
    static float next=-1;static unsigned seed;
    if(seed==g.seed&&g.time<next&&next-g.time<1.1f)return;
    seed=g.seed;next=g.time+1;
    for(int n=0;n<g.actorCount;n++)if(g.actors[n].active)
        for(int slot=0;slot<LOOT_SLOTS;slot++)stolen_until(&g.actors[n],slot);
    for(int slot=0;slot<HOME_STASH;slot++)stolen_home_until(slot);
    for(int slot=0;slot<g.nodeCount;slot++)stolen_drop_until(slot);
    for(int b=0;b<g.buildingCount;b++)if(g.field.rolls[b]>1)g.field.rolls[b]=1+stolen_age_owner(g.field.rolls[b]-1,&g.repairs.rollUntil[b]);
    for(int c=0;c<SLAVE_CAMPS;c++)for(int slot=0;slot<SEIZED_STACKS;slot++)stolen_property_until(c,slot);
    for(int n=0;n<g.actorCount;n++)g.depth.actors[n].confiscatedOwner=stolen_age_owner(g.depth.actors[n].confiscatedOwner,&g.repairs.confiscatedUntil[n]);
}
static int stamps_valid(const TheftStamp *s,int count){
 for(int i=0;i<count;i++)if(s[i].key>=(unsigned)ITEMS*FACTIONS||!isfinite(s[i].until)||s[i].until<0)return 0;
 return 1;
}
int stolen_validate(const Game *save) {
 for(int n=0;n<MAX_ACTORS;n++)if(!stamps_valid(save->repairs.actor[n],LOOT_SLOTS))return 0;
 return stamps_valid(save->repairs.home,HOME_STASH)&&stamps_valid(save->repairs.drop,MAX_NODES)&&save->repairs.interiorsVersion>=0&&save->repairs.interiorsVersion<=1;
}

float inventory_input_until(const Actor*a,int item,int qty){
 float until=0;for(int i=0;i<PACK&&qty>0;i++)if(a->pack[i].item==item){int take=a->pack[i].qty<qty?a->pack[i].qty:qty;if(take)until=fmaxf(until,stolen_until(a,i));qty-=take;}return until;
}
