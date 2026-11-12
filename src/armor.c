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
        d->potency=z->tier+2;
        d->price=z->tier==4?12000+d->armor*70+d->weight*20:
                 45+d->armor*(15+z->tier*3)+d->weight*9+z->tier*z->tier*90;
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
void armor_unit_outfit(Actor *a,int rank) {
    int set=armor_uniform_set(a->faction,rank);
    armor_equip(a,set);
    /* Ranged crews keep the rank's protective body but wear the same
       faction scout headwear and boots as the other members of their unit. */
    if(set>=0 && weapon_ranged(a->gear[0])) {
        int scout=a->faction==UNION?7:a->faction==SHOGUN?24:10;
        a->gear[2]=armor_set_item(scout,1);a->gear[3]=armor_set_item(scout,3);
    }
}
int armor_cache_item(unsigned seed,int tier) {
    /* Unique cuirasses remain on their named owners. Remote finds are
       independent accessories, never a free complete legendary set. */
    if(tier>=3 && seed%32u==0)return armor_set_item(26+(seed>>8)%4,1+(seed>>12)%3);
    int options[26],count=0;
    for(int s=0;s<26;s++)if(armorSets[s].tier<=tier && armorSets[s].tier>=tier-1)options[count++]=s;
    return count?armor_set_item(options[(seed>>5)%(unsigned)count],(seed>>13)%4):PARTS;
}
void armor_wanderer(Actor *a,unsigned seed,int tier) {
    if(a->kind==BEAST||a->squad||tier<0)return;
    if(tier>3)tier=3;
    if(a->kind==RAIDER) {
        static const int slots[]={1,2,4,3};
        if(tier<=1) {
            /* Ragged bandits sometimes scavenge one or two low-grade pieces. */
            int count=seed%3,first=(seed>>4)%4;
            for(int p=0;p<4;p++)if(armor_item_set(a->gear[slots[p]])>=0)a->gear[slots[p]]=NONE;
            for(int i=0;i<count;i++) {
                int part=(first+i*3)%4,set=((seed>>(8+i))&1)?16:19;
                a->gear[slots[part]]=armor_set_item(set,part);a->gearStolen[slots[part]]=0;
            }
        }else {
            int primary=a->faction==OUTLAWS?((seed&1)?6:18):17;
            armor_equip(a,primary);
            for(int p=1;p<4;p++)if((seed>>(p*3))&1)a->gear[slots[p]]=armor_set_item((seed&2)?16:19,p);
        }
        return;
    }
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
    if(plan->importance==TOWN_CAPITAL && plan->marketTier>=2 &&
       ((g.seed ^ (unsigned)town*7919u)+(unsigned)g.day*37u)%13u==0) {
        int set=g.townOwner[town]==MONKS?27:g.townOwner[town]==SHOGUN?26+3*((g.day+town)%2):28;
        int id=armor_set_item(set,1+(g.day+town)%3);
        *market_stock(town,id)=1;*market_venue(town,id)=venue;
    }
}
static void armor_hidden_caches(void) {
    int found=0;
    for(int b=0;b<g.buildingCount && found<2;b++) {
        Building *chest=&g.buildings[b];
        if(!ruined_building(b)||chest->type!=SHOP||distance(chest->x,chest->y,towns[0].x,towns[0].y)<300)continue;
        unsigned roll=g.seed^(unsigned)b*2654435761u;
        /* Ruined shops do not restock: taking this does not regenerate it. */
        if(!chest->storage[15].qty)chest->storage[15]=(Stack){armor_set_item(26+(roll>>8)%4,1+(roll>>16)%3),1};
        found++;
    }
}
void armor_world_init(void) {
    if(g.armor.version>=2)return;
    int previous=g.armor.version;g.armor.version=2;
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
        if(previous && armor_item_set(a->gear[1])>=0)continue;
        if(a->kind==GUARD&&war_faction_index(a->faction)>=0){
            int lv=level(a,MELEE),rank=lv>=40?2:lv>=25?1:0;
            armor_unit_outfit(a,rank);
        }else {
            int lv=level(a,MELEE),tier=lv>=40?3:lv>=24?2:lv>=12?1:0;
            armor_wanderer(a,g.seed+(unsigned)n*7919u,tier);
            if(a->kind==RAIDER)g.depth.actors[n].upkeep=35+(g.seed+(unsigned)n*13u)%36;
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
                armor_unit_outfit(a,i==0?2:i%4==0?1:0);
        }
    }
    armor_hidden_caches();
    for(int t=0;t<TOWNS;t++)armor_stock(t);
}
