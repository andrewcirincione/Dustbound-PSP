#include "game.h"
#include <stdio.h>
#include <string.h>
static uint32_t pixels[480*272];
static void frame(const char *name) {
    char path[240];snprintf(path,sizeof(path),"evidence/v070/%s.ppm",name);
    render_game(pixels,480);FILE*f=fopen(path,"wb");fprintf(f,"P6\n480 272\n255\n");
    for(int i=0;i<480*272;i++){unsigned char c[3]={pixels[i]&255,(pixels[i]>>8)&255,(pixels[i]>>16)&255};fwrite(c,1,3,f);}fclose(f);
}
int main(void){
    game_new(73819,0);ui.noticeTime=0;ui.screen=SCREEN_WORLD;find_context();frame("v070-world");
    Actor*a=&g.actors[g.selected];
    ui.screen=SCREEN_PACK;inventory_add(a,gear_id(8,3,2),1);for(int i=0;i<PACK;i++)if(a->pack[i].item==gear_id(8,3,2))ui.selection=i;frame("v070-inventory");
    ui.screen=SCREEN_HEALTH;ui.selection=0;frame("v070-health");
    ui.screen=SCREEN_SQUAD;ui.selection=0;frame("v070-squad");
    ui.screen=SCREEN_SKILLS;ui.selection=STRENGTH;frame("v070-skills");
    ui.screen=SCREEN_MENU;ui.menuTab=1;ui.selection=0;frame("v070-start-squad");ui.menuTab=2;frame("v070-start-world");
    for(int i=0;i<g.actorCount;i++)if(!g.actors[i].squad&&g.actors[i].kind==TRADER){ui.lootActor=i;break;}
    Actor*b=&g.actors[ui.lootActor];inventory_add(b,RELIC,1);inventory_add(b,RATION,4);inventory_add(b,PARTS,2);
    a->x=b->x-.7f;a->y=b->y+.3f;a->sneaking=1;b->facing=1;ui.screen=SCREEN_THEFT;ui.selection=0;frame("v070-theft");
    ui.screen=SCREEN_LOOT;frame("v070-loot");
    memset(g.seen,255,sizeof(g.seen));memset(g.discovered,1,sizeof(g.discovered));
    ui.screen=SCREEN_MAP;ui.mapZoom=1;ui.mapTown=0;ui.mapX=towns[0].x+90;ui.mapY=towns[0].y+50;frame("v070-map");
    ui.screen=SCREEN_JOURNAL;ui.journalTab=1;ui.selection=0;ui.regionChoice=0;frame("v070-saltwatch");ui.selection=1;frame("v070-mesa");ui.selection=2;frame("v070-pale-pass");
    printf("Rendered 13 layouts. Actors %d/%d; toll ids %d,%d,%d; Game bytes %zu.\n",g.actorCount,MAX_ACTORS,g.regions.toll[0],g.regions.toll[1],g.regions.toll[2],sizeof(Game));
}
