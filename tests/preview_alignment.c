/* Prepared visual-review scenes, using the unmodified gameplay renderer.
   This captures previews only; it does not run the regression suites. */
#include "../src/render.c"
#include <assert.h>
#include <sys/stat.h>

static void preview_frame(const char *root,const char *name) {
    static uint32_t pixels[480*272];
    render_game(pixels,480);
    char path[512];snprintf(path,sizeof(path),"%s/%s.ppm",root,name);
    FILE *f=fopen(path,"wb");assert(f);fprintf(f,"P6\n480 272\n255\n");
    for(int i=0;i<480*272;i++){
        unsigned char rgb[]={pixels[i]&255,pixels[i]>>8&255,pixels[i]>>16&255};
        fwrite(rgb,1,3,f);
    }
    fclose(f);
}
static void safe_road(Actor *a) {
    int found=0;
    for(int y=160;y<1200&&!found;y+=17)for(int x=160;x<1200;x+=17)
        if(town_near(x,y,30)<0&&can_walk(x,y)&&can_walk(x+12,y)&&can_walk(x-2,y)){
            a->x=x;a->y=y;found=1;break;
        }
    assert(found);a->homeX=a->x;a->homeY=a->y;
    for(int n=1;n<g.actorCount;n++)if(!g.actors[n].squad&&!actor_inside(&g.actors[n])){
        g.actors[n].x=g.actors[n].y=1450;g.actors[n].target=-1;
    }
}
int main(int argc,char **argv) {
    const char *root=argc>1?argv[1]:"build/alignment-preview/native";
    mkdir(root,0777);mkdir("build/v1000/fixtures",0777);
    int sets[]={0,3,9,26},weapons[]={100,109,119,32};
    for(int n=0;n<4;n++)for(int face=0;face<2;face++){
        game_new(73819,0);g.selected=0;g.dayTime=12;
        Actor *a=&g.actors[0];safe_road(a);armor_equip(a,sets[n]);
        a->gear[0]=WEAPON_ITEMS+weapons[n];a->gear[2]=a->gear[5]=0;
        a->facing=face;a->body=a->race=a->hair=a->beard=0;a->skin=2;
        a->autoattack=a->attackPhase=a->movedTimer=a->sneaking=a->taskKind=0;
        a->target=-1;a->anim=0;
        memset(a->pack,0,sizeof(a->pack));memset(a->stolen,0,sizeof(a->stolen));
        inventory_add(a,MEDKIT,1);
        ui.screen=SCREEN_WORLD;ui.noticeTime=ui.feedCount=0;
        char name[96],path[512];
        snprintf(name,sizeof(name),"alignment-set-%02d-weapon-%03d-face-%d",sets[n],weapons[n],face);
        snprintf(path,sizeof(path),"build/v1000/fixtures/%s.sav",name);assert(save_game(path));
        char output[112];snprintf(output,sizeof(output),"%s-world",name);preview_frame(root,output);
        ui.screen=SCREEN_PACK;ui.selection=0;snprintf(output,sizeof(output),"%s-pack",name);preview_frame(root,output);
        ActorSprite sprite={0};paint_cached_actor(&sprite,a,1,0,0);
        snprintf(path,sizeof(path),"%s/%s.rgba",root,name);FILE *f=fopen(path,"wb");assert(f);
        fwrite(sprite.pixels,sizeof(sprite.pixels),1,f);fclose(f);
        snprintf(path,sizeof(path),"%s/%s-hi.rgba",root,name);f=fopen(path,"wb");assert(f);
        fwrite(actorScratch,sizeof(actorScratch),1,f);fclose(f);
        a->movedTimer=.12f;
        for(int pose=0;pose<8;pose++){
            a->anim=pose*6.2831853f/8;
            paint_cached_actor(&sprite,a,1,0,0);
            snprintf(path,sizeof(path),"%s/%s-walk-%d.rgba",root,name,pose);f=fopen(path,"wb");assert(f);
            fwrite(sprite.pixels,sizeof(sprite.pixels),1,f);fclose(f);
        }
    }
    puts("Prepared four outfits / four greatswords in both facings, and rendered idle/walking visual previews. No regression tests run.");
    return 0;
}
