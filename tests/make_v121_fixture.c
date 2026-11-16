#define main previous_fixture_main
#include "make_v1000_fixture.c"
#undef main
#include "frontier_fixture.h"
static void road(Actor*a){
    int found=0;for(int y=160;y<1200&&!found;y+=17)for(int x=160;x<1200;x+=17)if(town_near(x,y,30)<0&&can_walk(x,y)&&can_walk(x+12,y)&&can_walk(x-2,y)){a->x=x;a->y=y;found=1;break;}assert(found);
    a->homeX=a->x;a->homeY=a->y;
    for(int n=1;n<g.actorCount;n++)if(!g.actors[n].squad&&!actor_inside(&g.actors[n])){g.actors[n].x=g.actors[n].y=1450;g.actors[n].target=-1;}
}
static void near_person(Actor*a,int who){
    Actor*b=&g.actors[who];int room=depth_room(b);
    if(room)in_room(room-1,depth_x(b)+.8f,depth_y(b));else{a->x=b->x+.6f;a->y=b->y;}
}
static int station(int type){for(int b=0;b<g.buildingCount;b++)if(g.buildings[b].type==type&&g.buildings[b].owner<0&&!g.frontier.variant[b])return b;return -1;}
int main(int argc,char**argv){
    root=argc>1?argv[1]:"build/v1000/fixtures";mkdir(root,0777);char path[512],name[80];
    for(int face=0;face<2;face++){
        Actor*a=fresh();road(a);armor_equip(a,19);a->facing=face;a->gear[0]=0;a->skin=5;a->hair=3;a->beard=5;snprintf(name,sizeof(name),"v121-iron-%d",face);save(name);
        a->gear[0]=gear_id(0,1,1);snprintf(name,sizeof(name),"v121-katana-%d",face);save(name);
        a->gear[0]=WEAPON_ITEMS+109;a->autoattack=0;snprintf(name,sizeof(name),"v121-greatsword-%d",face);save(name);
    }
    for(int bag=0;bag<8;bag++){
        Actor*a=fresh();road(a);a->gear[1]=LEATHER;a->gear[5]=gear_id(76+bag,0,0);a->facing=bag%2;snprintf(name,sizeof(name),"v121-pack-%d",bag);save(name);
    }
    Actor*a=fresh();int smith=station(FORGE);assert(smith>=0);in_room(smith,3.5f,5.8f);fixture_service(ROOM_FORGE);
    armor_equip(a,19);a->gear[0]=gear_id(0,1,1);a->hair=3;a->beard=5;g.tech=3;a->xp[SMITHING]=12*99*99;
    inventory_add(a,IRONBAR,40);inventory_add(a,CLOTH,20);inventory_add(a,WOOD,20);inventory_add(a,COAL,20);save("v121-forge");
    fixture_service(ROOM_SMITH_TRADE);save("v121-smith-counter");
    a=fresh();int employer=-1,town=-1,workRow=-1;
    for(int t=0;t<TOWNS;t++)if(contract_offer(t,0)>=0&&work_contact(t)>=0){town=t;employer=work_contact(t);for(int r=0;r<dialogue_count(employer);r++)if(dialogue_action(employer,r)==1)workRow=r;if(workRow>=0)break;}
    assert(employer>=0&&workRow>=0);near_person(a,employer);save("v121-quest");
    char employerName[16];snprintf(employerName,sizeof(employerName),"%s",g.actors[employer].name);
    a=fresh();int barber=-1,appearance=-1;
    for(int n=1;n<g.actorCount;n++)if(npc_role(&g.actors[n])==ROLE_BARBER&&conscious(&g.actors[n])){barber=n;break;}assert(barber>=0);
    for(int r=0;r<dialogue_count(barber);r++)if(dialogue_action(barber,r)==5)appearance=r;assert(appearance>=0);near_person(a,barber);a->skin=4;a->hair=3;a->beard=5;save("v121-appearance");
    char barberName[16];snprintf(barberName,sizeof(barberName),"%s",g.actors[barber].name);
    a=fresh();int camp=-1;float x=0,y=0;
    for(int k=0;k<POP_GROUPS;k++)if(g.population.groups[k].faction!=HOSTILE&&g.population.state[k].cacheQty)for(int o=0;o<population_object_count(k);o++)if(population_object(k,o,&x,&y)==3){camp=k;goto found;}
    found:assert(camp>=0);a->x=x+.5f;a->y=y;save("v121-camp-chest");
    a=fresh();int inn=-1;for(int b=0;b<g.buildingCount;b++)if(g.buildings[b].type==BAR&&bed_cost(b)>0){inn=b;break;}assert(inn>=0);in_room(inn,3.5f,5.8f);if(inn_has_upstairs(inn))inn_change_floor(a,1);fixture_service(ROOM_REST);a->health[LARM]=35;a->fatigue=45;dress_wounds(a);save("v121-bed");
    a=fresh();road(a);a->unconscious=45;a->woundFlags=WOUND_COMA;a->health[HEAD]=-8;a->blood=65;dress_wounds(a);g.feedback.wakeTotal[0]=60;save("v121-coma");
    a=fresh();road(a);int beast=add_actor("Wounded prowler",BEAST,WILDLIFE,a->x+.5f,a->y,2);assert(beast>=0);Actor*b=&g.actors[beast];b->unconscious=100;memset(b->pack,0,sizeof(b->pack));memset(b->gear,0,sizeof(b->gear));b->pack[0]=(Stack){MEAT,4};b->pack[1]=(Stack){HIDE,4};save("v121-loot");
    a=fresh();road(a);int decoy=add_actor("Indoor opponent",RAIDER,HOSTILE,a->x+.5f,a->y,2);g.depth.actors[decoy].room=smith+1;g.depth.actors[decoy].rx=g.depth.actors[decoy].ry=3;
    int opponent=add_actor("Road opponent",RAIDER,HOSTILE,a->x+11,a->y,2);a->autoattack=1;a->gear[0]=SABRE;a->healthScale=1000;for(int l=0;l<LIMBS;l++)a->health[l]=1000;save("v121-fight");
    a=fresh();road(a);g.weather=2;g.weatherTimer=120;save("v121-rain");
    snprintf(path,sizeof(path),"%s/feedback.json",root);FILE*f=fopen(path,"w");assert(f);
    fprintf(f,"{\"work_row\":%d,\"employer\":\"%s\",\"barber\":\"%s\",\"appearance_row\":%d,\"opponent\":%d,\"note_screen\":%d,\"journal_screen\":%d,\"forge_screen\":%d,\"chest_screen\":%d,\"creator_screen\":%d}",workRow,employerName,barberName,appearance,opponent,SCREEN_CONTRACT_NOTE,SCREEN_JOURNAL,SCREEN_FORGE,SCREEN_CHEST,SCREEN_CREATE);fclose(f);
    puts("Prepared 1.2.1 actual-control scenes for character fits, 8 backpacks, forge/sales, quest offers and abandonment, barber, camp chest, lodging, coma, loot, fighting and rain.");return 0;
}
