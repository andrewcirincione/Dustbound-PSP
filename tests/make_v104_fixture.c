#define main older_fixture_main
#include "make_v1000_fixture.c"
#undef main
int main(int argc,char**argv){root=argc>1?argv[1]:"build/v1000/fixtures";mkdir(root,0777);
 for(int i=100;i<120;i++){
  arena(1);Actor*a=&g.actors[0];int target=a->target;
  a->gear[0]=a->attackWeapon=WEAPON_ITEMS+i;a->gear[5]=NONE;armor_equip(a,(i-100)%ARMOR_SETS);
  a->xp[STRENGTH]=12*79*79;g.society.weaponXP[0][weapon_class(a->gear[0])]=12*39*39;
  memset(a->pack,0,sizeof(a->pack));inventory_add(a,WEAPON_ITEMS+i,1);a->autoattack=0;
  Actor*b=&g.actors[target];b->gear[0]=NONE;b->gear[2]=NONE;b->cooldown=1000;
  for(int l=0;l<LIMBS;l++)b->health[l]=1000;b->blood=100;b->bleed=0;
  b->x=a->x+(i%2?-.9f:.9f);b->y=a->y-.1f;a->facing=!(i%2);
  g.dayTime=12;g.weather=0;g.weatherTimer=g.eventTimer=10000;
  char name[64];snprintf(name,sizeof(name),"greatsword-%03d",i);save(name);
 }
 puts("Prepared 20 new greatswords for equipped and real combat PSP review.");return 0;
}
