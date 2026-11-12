#include "game.h"
#include "armor_sets_data.h"
#include <stdio.h>
#include <string.h>

static char armorNames[ARMOR_EXTRA_ITEMS][40],armorDescriptions[ARMOR_EXTRA_ITEMS][112];

int armor_item_set(int item) {
    if(item>=ARMOR_SET_ITEMS && item<ITEMS)return (item-ARMOR_SET_ITEMS)/4;
    if(item>=REGALIA_ITEMS && item<REGALIA_ITEMS+36 && (item-REGALIA_ITEMS)%2)
        for(int s=0;s<ARMOR_SETS;s++)if(armorSets[s].owner==(item-REGALIA_ITEMS)/2)return s;
    return -1;
}
int armor_set_item(int set,int piece) {
    if(set<0||set>=ARMOR_SETS||piece<0||piece>=4)return NONE;
    /* Legendary bodies retain their existing unique owner/item identity. */
    if(piece==0 && armorSets[set].owner>=0)return REGALIA_ITEMS+armorSets[set].owner*2+1;
    return ARMOR_SET_ITEMS+set*4+piece;
}
int armor_set_faction(int set) {return set>=0&&set<ARMOR_SETS?armorSets[set].faction:-1;}
int armor_coverage(int item) {
    int s=armor_item_set(item);return s<0?65:armorSets[s].coverage;
}
void armor_content(void) {
    static const char *parts[]={"body","headwear","legwear","boots"};
    static const int slots[]={1,2,4,3};
    for(int s=0;s<ARMOR_SETS;s++)for(int p=0;p<4;p++){
        int n=s*4+p,id=ARMOR_SET_ITEMS+n;const ArmorSetDef *z=&armorSets[s];
        ItemDef *d=&itemDefs[id];*d=itemDefs[gear_id(z->family[p],1,1)];
        snprintf(armorNames[n],sizeof(armorNames[n]),"%s %s",z->name,parts[p]);
        d->name=armorNames[n];d->desc=armorDescriptions[n];d->slot=slots[p];
        d->color=0x808080;d->trim=z->tier==4?0xc9ab6b:0x9b8669;d->quality=0;
        d->armor=p==0?z->armor:p==1?z->armor*55/100:p==2?z->armor*60/100:z->armor*28/100;
        if(d->armor<1)d->armor=1;
        d->weight=p==0?z->weight:p==1?(z->weight+4)/5:p==2?(z->weight+3)/4:(z->weight+7)/8;
        d->speed=p==0?z->speed:0;d->stealth=p==0?z->stealth:0;
        d->protection=p==0?z->weather:0;d->capacity=0;d->effect=z->stealth>5?6:z->weather>=40?9:4;
        d->potency=z->tier+2;d->price=45+d->armor*17+d->weight*9+(z->tier==4?2400:0);
        snprintf(armorDescriptions[n],sizeof(armorDescriptions[n]),"%s. %s; arm coverage %d%%. Armor %d; weight %d.",
                 z->name,z->heavy?"Heavy":z->weight<=5?"Light":"Medium",z->coverage,d->armor,d->weight);
        if(p==0 && z->owner>=0){
            int unique=REGALIA_ITEMS+z->owner*2+1;
            itemDefs[unique]=*d;
            itemDefs[unique].family=eliteDefs[z->owner].armorFamily;
            snprintf(armorDescriptions[n],sizeof(armorDescriptions[n]),"%s's %s. Armor %d; weight %d; arm coverage %d%%.",
                     eliteDefs[z->owner].name,z->name,d->armor,d->weight,z->coverage);
        }
    }
}
void armor_equip(Actor *a,int set) {
    static const int slots[]={1,2,4,3};
    if(set<0||set>=ARMOR_SETS||a->kind==BEAST)return;
    for(int p=0;p<4;p++){a->gear[slots[p]]=armor_set_item(set,p);a->gearStolen[slots[p]]=0;}
}
int armor_uniform_set(int faction,int rank) {
    if(rank<0)rank=0;if(rank>2)rank=2;
    static const int sets[3][3]={{4,5,9},{20,22,23},{13,14,25}};
    int f=war_faction_index(faction);return f<0?-1:sets[f][rank];
}
void armor_wanderer(Actor *a,unsigned seed,int tier) {
    if(a->kind==BEAST||a->squad||tier<0)return;
    if(tier>3)tier=3;
    int options[ARMOR_SETS],count=0;
    for(int s=0;s<26;s++){
        const ArmorSetDef *z=&armorSets[s];
        if(z->tier>tier||z->tier<tier-1)continue;
        if(z->faction!=a->faction && z->faction!=FREE)continue;
        options[count++]=s;
    }
    if(count)armor_equip(a,options[seed%(unsigned)count]);
}
void armor_stock(int town) {
    memset(g.armor.stock[town],0,sizeof(g.armor.stock[town]));
    memset(g.armor.venue[town],0,sizeof(g.armor.venue[town]));
    const TownDesign *plan=town_design(town);
    int venue=plan->smith?TRADE_SMITH:-1;
    for(int b=0;b<g.buildingCount;b++)
        if(g.frontier.variant[b]==BUILD_ARMORER && building_town(b)==town)venue=TRADE_ARMORER;
    if(venue<0)return;
    for(int s=0;s<26;s++){
        const ArmorSetDef *z=&armorSets[s];
        if(z->faction!=g.townOwner[town] && z->faction!=FREE && z->faction!=SCAVENGERS)continue;
        if(z->tier>plan->marketTier+1)continue;
        int p=(g.day+town+s)%4,id=armor_set_item(s,p);
        if((s+town+g.day)%3 && z->tier>0)continue;
        /* Rotate clothing into the existing smith stock budget. Keep weapon
           counts and small-town service sizes intact as the catalog expands. */
        if(venue==TRADE_SMITH){
            int replace=-1;
            for(int it=1;it<V29_ITEMS;it++)
                if(itemDefs[it].slot>=1&&itemDefs[it].slot<=4&&*market_stock(town,it)>0&&*market_venue(town,it)==venue){
                    replace=it;if(itemDefs[it].slot==itemDefs[id].slot)break;
                }
            if(replace<0)continue;
            *market_stock(town,replace)=0;
        }
        *market_stock(town,id)=1;
        *market_venue(town,id)=venue;
    }
}
void armor_world_init(void) {
    if(g.armor.version)return;
    g.armor.version=1;
    for(int n=0;n<g.actorCount;n++){
        Actor *a=&g.actors[n];
        if(!a->active||a->squad||a->dead||a->unconscious>0||!a->gear[1]||a->kind==BEAST||a->prisonTime>0||a->gear[1]==SLAVE_GARB)continue;
        int elite=civil_elite_index(n);
        if(elite>=0){
            for(int s=26;s<30;s++)if(armorSets[s].owner==elite)armor_equip(a,s);
            continue;
        }
        if(depth_named(n)>=0||regional_named(n))continue;
        /* Service staff keep their authored work clothes and movement. */
        if(a->kind!=GUARD && npc_role(a)!=ROLE_CITIZEN)continue;
        if(armor_item_set(a->gear[1])>=0)continue;
        if(a->kind==GUARD&&war_faction_index(a->faction)>=0){
            int lv=level(a,MELEE),rank=lv>=40?2:lv>=25?1:0;
            armor_equip(a,armor_uniform_set(a->faction,rank));
        }else if(!(a->kind==RAIDER&&level(a,MELEE)<12)){
            int lv=level(a,MELEE),tier=lv>=40?3:lv>=24?2:lv>=12?1:0;
            armor_wanderer(a,g.seed+(unsigned)n*7919u,tier);
        }
    }
    /* Existing runs may have unloaded army reserves. Upgrade only living,
       still-equipped members once; stripped bodies and the player's squad stay intact. */
    for(int k=0;k<POP_GROUPS+EVENTS;k++){
        WorldEvent *e=k<POP_GROUPS?&g.population.groups[k]:&g.living.events[k-POP_GROUPS];
        int army=k<POP_GROUPS?(e->type==POP_ARMY||e->type==POP_PATROL):e->type==6;
        if(!army||!e->stored||war_faction_index(e->faction)<0)continue;
        for(int i=0;i<e->count;i++){
            Actor *a=&e->reserve[i];
            if(!a->dead&&a->unconscious<=0&&!a->squad&&a->gear[1])
                armor_equip(a,armor_uniform_set(e->faction,i==0?2:i%4==0?1:0));
        }
    }
    for(int t=0;t<TOWNS;t++)armor_stock(t);
}
