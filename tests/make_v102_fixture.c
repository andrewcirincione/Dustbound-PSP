#define main older_fixture_main
#include "make_v1000_fixture.c"
#undef main
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
int main(int argc,char **argv){
    root=argc>1?argv[1]:"build/v1000/fixtures";mkdir(root,0777);
    for(int s=0;s<ARMOR_SETS;s++){
        game_new(73819,0);g.selected=0;g.squadCount=1;g.dayTime=12;g.weather=0;g.weatherTimer=g.eventTimer=10000;
        Actor *a=&g.actors[0];a->body=a->race=a->hair=a->beard=a->scar=0;a->skin=2;a->hairColor=0;
        a->autoattack=0;a->target=a->carrying=a->carriedBy=-1;a->gear[0]=SABRE;a->gear[5]=NONE;
        a->xp[STRENGTH]=12*49*49;armor_equip(a,s);memset(a->pack,0,sizeof(a->pack));
        for(int p=0;p<4;p++)inventory_add(a,armor_set_item(s,p),1);
        for(int n=1;n<g.actorCount;n++)g.actors[n].active=0;
        for(int k=0;k<POP_GROUPS;k++){
            g.population.groups[k].x=g.population.groups[k].y=1400;
            g.population.state[k].siteX=g.population.state[k].siteY=1400;
        }
        for(int k=0;k<EVENTS;k++)g.living.events[k].x=g.living.events[k].y=1400;
        ui.screen=SCREEN_WORLD;ui.camX=a->x;ui.camY=a->y;
        char path[512];snprintf(path,sizeof(path),"%s/armor-%02d.sav",root,s);assert(save_game(path));
    }
    arena(1);armor_equip(&g.actors[0],29);save("armor-combat");
    puts("Prepared thirty real item loadouts for PSP controller review.");return 0;
}
