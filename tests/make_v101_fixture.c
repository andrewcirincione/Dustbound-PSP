/* Ordinary save files for controller checks of the release executable. */
#define main previous_fixture_main
#include "make_v1000_fixture.c"
#undef main

int main(int argc,char **argv){
    previous_fixture_main(argc,argv);
    static const char *names[]={"naginata","unarmed","tetsubo"};
    const int weapons[]={gear_id(4,2,1),NONE,gear_id(15,2,1)};
    const int torsos[]={34,37,38};
    for(int i=0;i<3;i++){
        arena(0);
        Actor *a=&g.actors[0];
        a->gear[0]=a->attackWeapon=weapons[i];
        a->gear[1]=gear_id(torsos[i],2,1);
        if(weapons[i])g.society.weaponXP[0][weapon_class(weapons[i])]=12*9*9;
        else a->xp[MARTIAL]=12*9*9;
        save(names[i]);
    }
    puts("Saved additional naginata, unarmed and tetsubo release scenes.");
    return 0;
}
