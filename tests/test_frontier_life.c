#include "game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int action(int type, int kind, int target) {
    int count = nearby_actions();
    for (int row = 0; row < count; row++) {
        const NearbyAction *a = nearby_action(row);
        if (a->type == type && a->kind == kind && a->target == target)
            return row;
    }
    return -1;
}
static void choose(int type, int kind, int target) {
    int row = action(type,kind,target);
    assert(row >= 0);
    nearby_action_choose(row);
}
static void button(int key) {
    game_tick(1.f/60,(Input){.pressed = key, .held = key});
}
static int pack_slot(const Actor *a,int item,int owner) {
    for (int i = 0; i < PACK; i++)
        if (a->pack[i].item == item && a->stolen[i] == owner) return i;
    return -1;
}
static void room_spot(float x,float y) {
    g.interiorX=x;
    g.interiorY=y;
}
static void purchase(int site) {
    Actor *a=&g.actors[g.selected];
    building_door(site,&a->x,&a->y);
    choose(ACTION_CONTEXT,3,site);
    assert(ui.screen==SCREEN_HOME);
    int money=g.money;
    button(KEY_CROSS);
    assert(ui.screen==SCREEN_WORLD&&home_state(site)==HOME_OWNED&&g.money==money-home_price(site));
    choose(ACTION_CONTEXT,3,site);
    assert(g.interiorBuilding==site+1&&home_occupant(site)<0);
}
static void test_homes(void) {
    game_new(73819,0);
    Actor *a=&g.actors[g.selected];
    int sites[2],count=0;
    int prices[TOWNS] = {0};
    for (int b=0;b<g.buildingCount;b++)
        if (home_state(b)==HOME_FOR_SALE) {
            if (count<2) sites[count]=b;
            count++;
            int town = building_town(b);
            assert(town >= 0 && town < TOWNS);
            prices[town] = home_price(b);
        }
    assert(count>=8);
    assert(prices[1] >= 5200 && prices[3] > prices[1]);
    assert(prices[OLD_TOWNS] > prices[3] && prices[OLD_TOWNS] >= 22000);
    int first=sites[0],second=sites[1];
    assert(building_town(first)!=building_town(second));
    assert(!home_buy(second));
    assert(home_price(first) >= 5200 && home_price(second) >= 5200);
    building_door(first, &a->x, &a->y);
    g.money = home_price(first) - 1;
    assert(!home_buy(first) && home_state(first) == HOME_FOR_SALE);
    g.money = home_price(first) + home_price(second) + 1200;
    purchase(first);
    choose(ACTION_FURNISH,5,first);
    assert(ui.screen==SCREEN_HOME);
    button(KEY_CROSS);
    assert(g.homes.furniture[first][0]==FURN_BED);
    button(KEY_RIGHT);
    button(KEY_DOWN);
    button(KEY_CROSS);
    assert(g.homes.furniture[first][1]==FURN_CHEST);
    button(KEY_CIRCLE);
    room_spot(2.55f,1.55f);
    int money=g.money;
    interior_interact();
    assert(actor_in_bed(a)&&g.money==money);
    assert(!home_furnish(first,0,FURN_EMPTY));
    a->taskKind=0;
    room_spot(4.50f,1.55f);
    choose(ACTION_CONTEXT,5,first);
    assert(ui.screen==SCREEN_STORAGE&&storage_slots(first)==HOME_STASH);
    int item=portion_item(MEDKIT,37);
    assert(inventory_add_owned(a,item,2,UNION));
    ui.selection=pack_slot(a,item,UNION);
    button(KEY_SQUARE);
    assert(pack_slot(a,item,UNION)<0&&g.homes.stash[0].qty==2&&storage_owner(first,0)==UNION);
    button(KEY_CIRCLE);
    interior_leave();
    purchase(second);
    assert(!storage_transfer(second,1,0));
    assert(home_furnish(second,2,FURN_CHEST));
    room_spot(2.45f,3.55f);
    choose(ACTION_CONTEXT,5,second);
    assert(ui.screen==SCREEN_STORAGE);
    button(KEY_RIGHT);
    button(KEY_CROSS);
    assert(g.homes.stash[0].qty==1&&pack_slot(a,item,UNION)>=0);
    assert(save_game("build/home-network.sav")&&load_game("build/home-network.sav"));
    assert(home_state(first)==HOME_OWNED&&home_state(second)==HOME_OWNED);
    assert(g.interiorBuilding==second+1&&g.homes.furniture[first][0]==FURN_BED&&
           g.homes.furniture[second][2]==FURN_CHEST);
    assert(storage_transfer(second,1,0)&&!g.homes.stash[0].item&&inventory_count(a,item)==2);
    assert(inventory_remove_slot(a,pack_slot(a,item,UNION),2));
    puts("PASS: buy homes in two towns, furnish both, use a free bed, and withdraw the same marked partial items after reload.");
}
static int watch_for(unsigned mask,int accepted) {
    for (int n=0;n<g.actorCount;n++) {
        int t=bounty_station(n);
        if (t>=0&&((mask&(1u<<g.townOwner[t]))!=0)==accepted)
            return n;
    }
    return -1;
}
static void next_to_watch(int n) {
    Actor *p=&g.actors[g.selected],*guard=&g.actors[n];
    int room=depth_room(guard);
    if (g.interiorBuilding)
        interior_leave();
    p->x=guard->x;
    p->y=guard->y;
    if (room) {
        actor_enter_room(g.selected,room-1);
        room_spot(depth_x(guard)+.4f,depth_y(guard));
    }
}
static void carry_target(int n,int dead) {
    Actor *a=&g.actors[n],*p=&g.actors[g.selected];
    p->carrying=n;
    a->carriedBy=g.selected;
    a->dead=dead;
    a->unconscious=dead?0:150;
}
static void test_warrants(void) {
    game_new(73819,0);
    int names[]={13,16,17};
    for (int i=0;i<ELITE_WARRANTS;i++) {
        WarrantView w;
        int n=g.civil.eliteActor[names[i]]-1;
        assert(n>=0&&warrant_view(i,&w)&&w.elite&&w.reward>=2000);
        assert(level(&g.actors[n],MELEE)>=60);
        assert(!strcmp(w.name,eliteDefs[names[i]].name));
    }
    int victim=g.civil.eliteActor[names[0]]-1;
    int jailkeepers=0;
    for (int n=0;n<g.actorCount;n++)
        if (g.actors[n].active&&!strcmp(g.actors[n].name,"Jailkeeper")&&g.actors[n].homeTown==0) {
            assert(g.actors[n].faction==g.townOwner[0]&&bounty_station(n)==0);
            jailkeepers++;
        }
    assert(jailkeepers>0);
    unsigned payers=bounty_payers(victim);
    int yes=watch_for(payers,1),no=watch_for(payers,0);
    assert(yes>=0&&no>=0);
    int town=bounty_station(yes), jail=g.field.jail[town]-1;
    Actor *p=&g.actors[g.selected];
    building_door(jail,&p->x,&p->y);
    choose(ACTION_WARRANTS,3,jail);
    assert(ui.screen==SCREEN_WARRANTS&&ui.warrantTown==town);
    int ids[WARRANT_ROWS],count=warrant_list(town,ids);
    assert(count>=1&&ids[0]==0);
    button(KEY_CROSS);
    assert(g.warrants.elite[0].tracked);
    button(KEY_SQUARE);
    assert(ui.screen==SCREEN_MAP&&ui.waypoint);
    button(KEY_CIRCLE);

    next_to_watch(no);
    carry_target(victim,1);
    int money=g.money,reward=bounty_reward(victim,0);
    assert(!bounty_turn_in(no)&&g.money==money);
    next_to_watch(yes);
    choose(ACTION_BOUNTY_TURN_IN,0,yes);
    assert(g.money==money+reward&&!bounty_turn_in(yes));
    assert(g.actors[victim].dead&&!g.actors[victim].active);
    assert(warrant_receipt_count()==1&&warrant_receipt(0)->silver==reward);
    assert(save_game("build/warrant-paid.sav")&&load_game("build/warrant-paid.sav"));
    civil_tick(2);
    assert(g.civil.fallen[names[0]]&&g.actors[victim].dead&&warrant_receipt(0)->dead);
    assert(!warrant_view(0,&(WarrantView){0})&&!bounty_payers(victim));

    int group=-1;
    for (int k=0;k<POP_GROUPS;k++)
        if (warrant_view(k+ELITE_WARRANTS,&(WarrantView){0})) { group=k; break; }
    assert(group>=0&&population_materialize(group));
    int n=g.population.groups[group].members[0];
    yes=watch_for(bounty_payers(n),1);
    assert(yes>=0);
    next_to_watch(yes);
    carry_target(n,0);
    g.actors[n].unconscious=0;
    money=g.money;
    reward=bounty_reward(n,1);
    assert(bounty_turn_in(yes)&&g.money==money+reward);
    assert(!population_owned(n)&&g.actors[n].prisonTime>0&&depth_room(&g.actors[n])>0);
    assert(g.population.state[group].detached&1);
    assert(save_game("build/warrant-capture.sav")&&load_game("build/warrant-capture.sav"));
    assert(!bounty_payers(n)&&!population_owned(n)&&warrant_receipt_count()==2);
    puts("PASS: three named elites, posted lead, wrong-jail rejection, permanent death, one-time payment, and live custody after reload.");
}
int main(void) {
    test_homes();
    test_warrants();
    return 0;
}
