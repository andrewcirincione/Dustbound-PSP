#define main older_fixture_main
#include "make_v1000_fixture.c"
#undef main
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
int main(int argc,char **argv){
    root=argc>1?argv[1]:"build/v1000/fixtures";mkdir(root,0777);
    for(int s=0;s<ARSENAL_WEAPONS;s++){
        game_new(73819,0);g.selected=0;g.squadCount=1;g.dayTime=12;g.weather=0;g.weatherTimer=g.eventTimer=10000;
        Actor *a=&g.actors[0];a->body=a->race=a->hair=a->beard=a->scar=0;a->skin=2;a->hairColor=0;
        a->autoattack=0;a->target=a->carrying=a->carriedBy=-1;a->gear[0]=WEAPON_ITEMS+s;a->gear[5]=NONE;
        a->xp[STRENGTH]=12*49*49;armor_equip(a,s%ARMOR_SETS);memset(a->pack,0,sizeof(a->pack));
        inventory_add(a,WEAPON_ITEMS+s,1);
        for(int n=1;n<g.actorCount;n++)g.actors[n].active=0;
        for(int k=0;k<POP_GROUPS;k++){
            g.population.groups[k].x=g.population.groups[k].y=1400;
            g.population.state[k].siteX=g.population.state[k].siteY=1400;
        }
        for(int k=0;k<EVENTS;k++)g.living.events[k].x=g.living.events[k].y=1400;
        ui.screen=SCREEN_WORLD;ui.camX=a->x;ui.camY=a->y;
        char path[512];snprintf(path,sizeof(path),"%s/weapon-%03d.sav",root,s);assert(save_game(path));
    }
    static const int tests[]={0,4,14,24,29,32,36,37};
    for(int k=0;k<8;k++){
        int index=-1;
        for(int i=0;i<ARSENAL_WEAPONS;i++)if(arsenal_type(WEAPON_ITEMS+i)==tests[k])index=i;
        assert(index>=0);arena(1);Actor*a=&g.actors[0];a->gear[0]=a->attackWeapon=WEAPON_ITEMS+index;
        armor_equip(a,29);g.actors[a->target].gear[0]=NONE;
        for(int l=0;l<LIMBS;l++)g.actors[a->target].health[l]=1000;
        g.actors[a->target].blood=100;g.actors[a->target].bleed=0;
        if(weapon_ranged(a->gear[0])){inventory_add(a,tests[k]>=29?CARTRIDGE:BOLTS,100);g.actors[a->target].x=a->x+5;g.actors[a->target].y=a->y;}
        char name[80];snprintf(name,sizeof(name),"weapon-combat-%02d",k);save(name);
    }
    puts("Prepared one hundred real weapon loadouts for PSP controller review.");return 0;
}
