#include "game.h"
#include "arsenal_data.h"
#include "armor_sets_data.h"
#include <stdio.h>
#include <string.h>
static char arsenalDescriptions[ARSENAL_WEAPONS][112];
static unsigned arand(unsigned *s){unsigned x=*s;x^=x<<13;x^=x>>17;x^=x<<5;return *s=x?x:37;}
int arsenal_type(int item){
    if(item>=WEAPON_ITEMS&&item<ITEMS)return arsenalDefs[item-WEAPON_ITEMS].type;
    if((item>=RAPIER_ITEMS&&item<RAPIER_ITEMS+24)||item==UNIQUE_ITEMS+1)return 37;
    if(item>=SCIMITAR_ITEMS&&item<SCIMITAR_ITEMS+24)return 36;
    int f=gear_family(item);
    if(f<0)f=item==STAFF?16:item==CROSSBOW?25:item==CLEAVER?11:item==SABRE?8:0;
    return f<32?f:f>=84&&f<88?f-52:-1;
}
int arsenal_index(int item){
    if(item>=WEAPON_ITEMS&&item<ITEMS)return item-WEAPON_ITEMS;
    int owner=regalia_owner(item);
    if(owner<0 || (item-REGALIA_ITEMS)%2)return -1;
    int best=-1,tier=-1;
    for(int i=0;i<ARSENAL_WEAPONS;i++)if(arsenalDefs[i].family==eliteDefs[owner].weaponFamily&&arsenalDefs[i].tier>tier){best=i;tier=arsenalDefs[i].tier;}
    return best;
}
int arsenal_tier(int item){
    int i=arsenal_index(item);return i>=0?arsenalDefs[i].tier:itemDefs[item].quality;
}
int arsenal_level_tier(const Actor*a){int lv=level(a,MELEE);return lv>=60?4:lv>=38?3:lv>=23?2:lv>=10?1:0;}
void arsenal_content(void){
    for(int i=0;i<ARSENAL_WEAPONS;i++){
        const ArsenalDef*z=&arsenalDefs[i];ItemDef*d=&itemDefs[WEAPON_ITEMS+i];
        *d=itemDefs[gear_id(z->family,z->tier==4?5:z->tier>=2?3:0,z->tier>3?3:z->tier)];
        d->name=z->name;d->desc=arsenalDescriptions[i];d->slot=0;d->family=z->family;
        d->power=z->power;d->weight=z->weight;d->range=z->range;d->cooldown=z->cooldown;
        d->penetration=z->penetration;d->accuracy=z->accuracy;d->price=z->price;
        d->quality=z->tier>3?3:z->tier;d->material=z->tier==4?5:z->tier>=2?3:0;
        d->effect=z->type==37||z->type==5||z->type==12?1:z->type==14||z->type==15||z->type==16||z->type==33?3:z->type==21||z->type==36?0:z->type==3||z->type==9?5:2;
        d->potency=3+z->tier*3;
        snprintf(arsenalDescriptions[i],112,"%s. Damage %d; pierce %d; weight %d. %s.",z->name,z->power,z->penetration,z->weight,z->tier==4?"Legendary":z->tier==3?"Veteran":z->tier==2?"Military":z->tier==1?"Field":"Salvaged");
    }
    /* Existing personal trophies retain their item IDs, owners and descriptions.
       Their new drawn model and stats follow the best local construction. */
    for(int owner=0;owner<ELITES;owner++){
        int item=REGALIA_ITEMS+owner*2,index=arsenal_index(item);if(index<0)continue;
        ItemDef*d=&itemDefs[item],*best=&itemDefs[WEAPON_ITEMS+index];
        if(d->power<best->power+2)d->power=best->power+2;
        if(d->penetration<best->penetration)d->penetration=best->penetration;
        if(d->price<best->price+1500)d->price=best->price+1500;
    }
}
void arsenal_outfit(Actor*a,unsigned seed,int tier){
    if(a->kind==BEAST||a->kind==PLAYER||a->squad||!a->gear[0]||regalia_owner(a->gear[0])>=0)return;
    if(a->gear[0]>=UNIQUE_ITEMS&&a->gear[0]<UNIQUE_ITEMS+8)return;
    int type=arsenal_type(a->gear[0]),best=-1,rank=-1;
    for(int i=0;i<ARSENAL_WEAPONS;i++)if(arsenalDefs[i].type==type&&arsenalDefs[i].tier<=tier){
        int score=arsenalDefs[i].tier*100+(int)((seed+i*37u)%31);
        if(score>rank){best=i;rank=score;}
    }
    if(best>=0){a->gear[0]=WEAPON_ITEMS+best;a->gearStolen[0]=0;}
}
static int smith_budget(int town){return 15+10*town_design(town)->importance;}
static int smith_max_tier(int town){int i=town_design(town)->importance;return i>=TOWN_CAPITAL?4:i+1;}
static void clear_equipment(int town,int venue){
    for(int it=1;it<ITEMS;it++)if(*market_venue(town,it)==venue&&itemDefs[it].slot>=0&&itemDefs[it].slot<=4)*market_stock(town,it)=0;
}
static int choose_weapon(int town,int maxTier,unsigned*seed,int rare){
    int best=-1,score=-100000;
    for(int i=0;i<ARSENAL_WEAPONS;i++){
        const ArsenalDef*z=&arsenalDefs[i];int it=WEAPON_ITEMS+i;
        if(z->tier>maxTier||*market_stock(town,it)>0||(!rare&&z->tier==4)||(rare&&z->tier!=4))continue;
        int n=(int)(arand(seed)%100)+(z->faction==g.townOwner[town]?70:z->faction==SCAVENGERS?20:0);
        if(n>score){score=n;best=it;}
    }
    return best;
}
static int choose_armor(int town,int maxTier,unsigned*seed){
    int best=-1,score=-100000;
    for(int s=0;s<ARMOR_SETS;s++)for(int p=0;p<4;p++){
        const ArmorSetDef*z=&armorSets[s];int it=armor_set_item(s,p);
        if(z->tier>maxTier||(z->tier==4&&p==0)||*market_stock(town,it)>0)continue;
        int n=(int)(arand(seed)%100)+(z->faction==g.townOwner[town]?75:z->faction==SCAVENGERS||z->faction==FREE?20:0);
        if(n>score){score=n;best=it;}
    }
    return best;
}
void arsenal_stock(int town){
    const TownDesign *plan=town_design(town);int budget=smith_budget(town),maxTier=smith_max_tier(town);
    unsigned seed=g.seed^(town+1u)*2654435761u^(g.day+1u)*2246822519u;
    if(plan->smith){
        clear_equipment(town,TRADE_SMITH);
        int count=0,weaponCount=budget*3/5,rare=maxTier==4&&arand(&seed)%3==0;
        for(int i=0;i<weaponCount;i++){
            int it=choose_weapon(town,maxTier,&seed,rare&&i==0);
            if(it<0)it=choose_weapon(town,maxTier,&seed,0);
            if(it<0)break;*market_stock(town,it)=1;*market_venue(town,it)=TRADE_SMITH;count++;
        }
        for(;count<budget;count++){
            int it=choose_armor(town,maxTier>3?3:maxTier,&seed);if(it<0)break;
            *market_stock(town,it)=1;*market_venue(town,it)=TRADE_SMITH;
        }
        /* One rare legendary armor accessory may replace an ordinary armor
           row in a capital, within the same 45-option budget. */
        if(maxTier==4 && seed%13==0){
            int set=g.townOwner[town]==SHOGUN?26:g.townOwner[town]==MONKS?27:28;
            int it=armor_set_item(set,1+(seed>>8)%3),replace=-1;
            for(int old=ARMOR_SET_ITEMS;old<WEAPON_ITEMS;old++)if(*market_stock(town,old)>0&&*market_venue(town,old)==TRADE_SMITH){replace=old;break;}
            if(replace>=0){*market_stock(town,replace)=0;*market_stock(town,it)=1;*market_venue(town,it)=TRADE_SMITH;}
        }
    }
    int armorer=0;
    for(int b=0;b<g.buildingCount;b++)if(g.frontier.variant[b]==BUILD_ARMORER&&building_town(b)==town)armorer=1;
    if(armorer){
        clear_equipment(town,TRADE_ARMORER);
        for(int i=0;i<budget;i++){
            int it=choose_armor(town,maxTier>3?3:maxTier,&seed);if(it<0)break;
            *market_stock(town,it)=1;*market_venue(town,it)=TRADE_ARMORER;
        }
    }
    g.arsenal.stockDay[town]=g.day;
}
void arsenal_world_init(void){
    if(g.arsenal.version)return;g.arsenal.version=1;
    for(int n=0;n<g.actorCount;n++){
        Actor*a=&g.actors[n];
        if(!a->active||a->dead||a->unconscious>0||a->prisonTime>0||g.depth.actors[n].captive||a->gearStolen[0])continue;
        arsenal_outfit(a,g.seed+(unsigned)n*7919u,arsenal_level_tier(a));
    }
    for(int k=0;k<POP_GROUPS+EVENTS;k++){
        WorldEvent*e=k<POP_GROUPS?&g.population.groups[k]:&g.living.events[k-POP_GROUPS];
        if(!e->stored)continue;
        for(int n=0;n<e->count;n++){Actor*a=&e->reserve[n];if(!a->dead&&a->unconscious<=0&&!a->gearStolen[0])arsenal_outfit(a,e->seed+n*7919u,arsenal_level_tier(a));}
    }
    for(int t=0;t<TOWNS;t++)arsenal_stock(t);
}
