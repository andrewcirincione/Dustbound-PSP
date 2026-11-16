#include "frontier_fixture.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
extern int add_actor(const char*,int,int,float,float,int);
static void fresh(void){game_new(73819,0);ui.screen=SCREEN_WORLD;ui.paused=0;}
static int station(int type){for(int b=0;b<g.buildingCount;b++)if(g.buildings[b].type==type&&g.buildings[b].owner<0&&!g.frontier.variant[b])return b;return -1;}
static void tick(int count){for(int j=0;j<count;j++)game_tick(.05f,(Input){0});}
int main(void){
    mkdir("build/v121",0777);
    fresh();Actor*a=&g.actors[0];
    memset(a->pack,0,sizeof(a->pack));memset(a->gear,0,sizeof(a->gear));g.depth.actors[0].layer=0;
    float empty=encumbrance(a);int weapon=gear_id(0,1,1),body=armor_set_item(19,0);
    a->gear[0]=weapon;a->gear[1]=body;
    assert(fabsf(encumbrance(a)-empty-itemDefs[weapon].weight-itemDefs[body].weight)<.01f);
    assert(item_slot_id(a,PACK)==weapon&&item_slot_qty(a,PACK)==1&&item_slot_equipped(PACK));
    ui.screen=SCREEN_PACK;assert(item_view_count()==2);
    int cap=actor_capacity(a);a->health[RARM]*=.6f;int hurt=actor_capacity(a);assert(hurt<cap);
    a->health[RARM]*=.3f;assert(actor_capacity(a)<hurt);a->lostMask|=1<<LARM;assert(actor_capacity(a)<hurt);
    fresh();a=&g.actors[0];g.money=100000;
    int item=a->gear[1],money=g.money;assert(item);
    assert(sell_item(0,PACK+1));assert(!a->gear[1]&&g.money>money);
    for(int t=0;t<TOWNS;t++)for(int i=1;i<ITEMS;i++){
        int buy=item_price(t,i,0),sell=item_price(t,i,1);
        assert(sell>0&&sell<=buy/2+1);
    }
    fresh();a=&g.actors[0];int smith=station(FORGE);assert(smith>=0);
    fixture_door(a,smith);assert(interior_enter(smith));fixture_service(ROOM_FORGE);
    memset(a->pack,0,sizeof(a->pack));inventory_add(a,IRONBAR,90);inventory_add(a,WOOD,40);inventory_add(a,CLOTH,40);inventory_add(a,HIDE,40);inventory_add(a,COAL,20);g.tech=3;a->xp[SMITHING]=12*99*99;
    ui.forgeSlot=0;ui.forgeFamily=13;ui.forgeMaterial=2;ui.forgeQuality=2;
    int forged=forge_selection_item();CraftRequirements req;assert(forge_requirements(forged,&req));
    assert(forge_equipment(forged)&&inventory_count(a,forged));
    for(int r=8;r<recipeCount;r++){assert(recipe_requirements(r,&req));assert(req.output>0&&req.qty>0);}
    int bench=station(BENCH);assert(bench>=0);actor_leave_room(0);a->x=g.buildings[bench].x+.65f;a->y=g.buildings[bench].y;
    assert(craft_item(8)&&inventory_count(a,LEATHER));assert(craft_item(9)&&inventory_count(a,BOOTS));
    fresh();a=&g.actors[0];int ordinary=harvest_yield(a);a->xp[FARMING]=12*59*59;assert(harvest_yield(a)==ordinary);
    int cooking=cooking_yield(a,2);a->xp[COOKING]=12*59*59;assert(cooking_yield(a,2)==cooking);
    int research=research_cost();a->xp[SCIENCE]=12*59*59;assert(research_cost()==research);
    int wood=build_wood_cost(3);a->xp[ENGINEERING]=12*99*99;assert(build_wood_cost(3)==wood);
    assert(buildCount==2&&!build_structure(3));
    fresh();a=&g.actors[0];int occupied=0,abandoned=0;
    for(int b=0;b<g.buildingCount;b++)if(g.buildings[b].type==STORE){
        if(g.frontier.variant[b]){assert(!depot_public(b));assert(!storage_slots(b));abandoned++;}
        else if(g.buildings[b].owner<0){assert(depot_public(b));occupied++;}
    }
    assert(occupied&&abandoned);
    int low=1000,high=0;for(int t=0;t<TOWNS;t++){int price=regional_bed_cost(t);if(price){if(price<low)low=price;if(price>high)high=price;}}assert(high>low);
    // Nothing is accepted by reading an offer; accept and abandon are separate inputs.
    char note[600];int before=journal_count();contract_offer_note(0,CONTRACT_FREIGHT,note,sizeof(note));assert(strstr(note,"Deposit")&&journal_count()==before);
    assert(accept_contract(0,CONTRACT_DISPATCH));int slot=-1;for(int i=0;i<CONTRACTS;i++)if(g.contracts[i].active==1){slot=i;break;}assert(slot>=0);
    assert(inventory_count(a,DISPATCH_ITEM)==1);assert(abandon_contract(slot));assert(!g.contracts[slot].active&&!inventory_count(a,DISPATCH_ITEM));
    assert(!g.plans.dispatchPending[slot]&&!g.society.jobActor[slot]);
    ui.journalTab=1;const Contract*c=journal_contract(journal_slot(0));assert(c&&c->active==2&&!c->completed);
    // Camp fixtures still work when no NPC slots are available.
    fresh();int group=-1;float x=0,y=0;
    for(int k=0;k<POP_GROUPS;k++)if(!g.population.groups[k].spawned)for(int o=0;o<population_object_count(k);o++)if(population_object(k,o,&x,&y)==3){group=k;goto camp;}
    camp:assert(group>=0);a=&g.actors[0];a->x=x;a->y=y;
    int supplies=g.population.state[group].cacheQty;assert(supplies>0);
    g.actorCount=MAX_ACTORS;for(int n=0;n<MAX_ACTORS;n++)g.living.owned[n]=0;
    assert(!population_materialize(group));
    assert(population_action(group,20)&&ui.screen==SCREEN_CHEST&&ui.chestKind==CHEST_CAMP);
    assert(g.population.state[group].cacheQty==supplies);interior_chest_take(0);assert(g.population.state[group].cacheQty==supplies-1);
    // A downed beast dies on vital harvest; persistent loot percentages survive saving.
    fresh();a=&g.actors[0];int beast=add_actor("Harvest test",BEAST,WILDLIFE,a->x+.5f,a->y,1);assert(beast>=0);
    Actor*b=&g.actors[beast];memset(b->pack,0,sizeof(b->pack));memset(b->gear,0,sizeof(b->gear));b->unconscious=60;b->pack[0]=(Stack){MEAT,2};b->pack[1]=(Stack){HIDE,2};ui.lootActor=beast;
    assert(loot_remaining(beast)==100);assert(loot_take(0,0));assert(b->dead&&loot_remaining(beast)==75);
    assert(save_game("build/v121/loot.sav")&&load_game("build/v121/loot.sav"));assert(g.actors[beast].dead&&loot_remaining(beast)==75);
    // Autosaving a defeated squad cannot replace an existing recovery point.
    fresh();set_save_root("build/v121");assert(save_slot(0));FILE*f=fopen("build/v121/DUST_0.sav","rb");assert(f);unsigned char header[20];assert(fread(header,20,1,f)==1);fclose(f);
    for(int n=0;n<g.actorCount;n++)if(g.actors[n].squad){g.actors[n].unconscious=60;g.actors[n].woundFlags|=WOUND_COMA;}
    assert(!squad_can_autosave()&&!save_slot(0));f=fopen("build/v121/DUST_0.sav","rb");unsigned char same[20];assert(fread(same,20,1,f)==1);fclose(f);assert(!memcmp(header,same,20));
    a=&g.actors[0];float wake=consciousness_progress(a);a->unconscious*=.5f;assert(consciousness_progress(a)>wake);
    a->unconscious=0;assert(squad_can_autosave());float recovered=recovery_progress(a);a->fatigue=80;assert(recovery_progress(a)<recovered);
    // Fight stance ignores opponents in another room and approaches visible opponents beyond 6m.
    fresh();a=&g.actors[0];float px=100,py=100;while(!can_walk(px,py)||!can_walk(px+12,py)){px+=20;py+=20;}
    a->x=px;a->y=py;a->autoattack=1;a->gear[0]=weapon;
    for(int n=1;n<g.actorCount;n++){g.actors[n].active=0;g.actors[n].squad=0;}
    int bad=add_actor("Indoor decoy",RAIDER,HOSTILE,px+.5f,py,1);g.depth.actors[bad].room=smith+1;g.depth.actors[bad].rx=g.depth.actors[bad].ry=3;
    int good=add_actor("Distant attacker",RAIDER,HOSTILE,px+11,py,1);g.actors[good].xp[ATHLETICS]=0;g.actors[good].job=JOB_HOLD;
    tick(2);assert(a->target==good);assert(a->target!=bad);
    float gap=depth_distance(a,&g.actors[good]);tick(120);assert(depth_distance(a,&g.actors[good])<gap||g.actors[good].unconscious>0);
    assert(load_game("build/dust-v34-expedition.sav"));assert(!strncmp(VERSION,"1.2.",4));assert(g.expedition.version&&g.actorCount>0);
    puts("PASS: 1.2.1 equipped slots/sales/weight, injury capacity, crafting, skill effects, depots, bed fees, offer/abandon, camp chests, vital harvest, persistent loot, autosave recovery, targeting and genuine v34 upgrade.");
    return 0;
}
