#include "game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(int argc,char**argv){
 for(int i=1;i<argc;i++){
  assert(load_game(argv[i]));int money=g.money,selected=g.selected;Actor saved=g.actors[selected];
  assert(save_game("build/v1000/compat-roundtrip.sav")&&load_game("build/v1000/compat-roundtrip.sav"));
  const Actor*a=&g.actors[g.selected];
  assert(g.money==money&&g.selected==selected&&a->x==saved.x&&a->y==saved.y);
  assert(!memcmp(a->pack,saved.pack,sizeof(a->pack))&&!memcmp(a->gear,saved.gear,sizeof(a->gear)));
  assert(!memcmp(a->xp,saved.xp,sizeof(a->xp))&&!memcmp(a->stolen,saved.stolen,sizeof(a->stolen)));
  printf("PASS archived save %s, inventory/gear/XP/money/position roundtrip.\n",argv[i]);
 }
 return 0;
}
