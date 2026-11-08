#include "game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void settle(TownPolitics *p,int side) {
    p->age=419;
    p->support[side]=85;
    p->support[1-side]=20;
    politics_tick(1);
    assert(p->stage==2);
}
static void capital(int side) {
    game_new(73819,0);
    int t=OLD_TOWNS+side, faction=g.townOwner[t];
    int old=g.war.resident[side][0]-1, local=g.war.resident[side][1]-1;
    TownPolitics *p=&g.life.towns[t];
    assert(old>=0&&local>=0);
    g.actors[old].unconscious=20;
    politics_tick(1);
    assert(!p->stage);
    g.actors[old].dead=1;
    g.actors[local].health[LARM]=62;
    int weapon=g.actors[local].gear[0];
    politics_tick(1);
    assert(p->stage==1&&p->original==faction);
    assert(!strcmp(politics_claimant(t,faction),g.actors[local].name));
    settle(p,0);
    assert(p->leader==local+1&&g.townOwner[t]==faction);
    assert(politics_leader_title(local)&&g.actors[old].dead);
    assert(g.actors[local].gear[0]==weapon&&g.actors[local].health[LARM]==62);
    int recent=(g.war.reportNext+WAR_REPORTS-1)%WAR_REPORTS;
    assert(g.war.report[recent].kind==WAR_SUCCESSION);
    assert(strstr(war_ambient(32+recent,FREE),g.actors[local].name));
    if(side==0){
        g.dayTime=12;
        g.actors[0].x=towns[t].x+2;
        g.actors[0].y=towns[t].y+4;
        g.actors[0].autoattack=0;
        g.discovered[t]=1;
        assert(save_game("build/v171/seated.sav"));
    }

    p->relief=1;p->pledge=0;
    g.actors[local].dead=1;
    politics_tick(1);
    assert(p->stage==1&&p->pledge==-1&&!p->relief&&politics_protected(local));
    assert(save_game("build/v171/vacancy.sav")&&load_game("build/v171/vacancy.sav"));
    assert(p->stage==1&&g.actors[old].dead&&g.actors[local].dead);
    char candidate[16];
    snprintf(candidate,sizeof(candidate),"%s",politics_claimant(t,faction));
    settle(p,0);
    int successor=p->leader-1;
    assert(successor>=0&&successor!=local&&successor!=old);
    assert(!strcmp(g.actors[successor].name,candidate)&&politics_leader_title(successor));
    assert(regalia_owner(g.actors[successor].gear[0])<0&&regalia_owner(g.actors[successor].gear[1])<0);
    assert(g.actors[old].dead&&g.actors[local].dead&&g.actors[local].gear[0]==weapon);
    int count=g.actorCount, rounds=g.life.succession;
    assert(save_game("build/v171/succession.sav")&&load_game("build/v171/succession.sav"));
    politics_tick(1);
    assert(p->leader==successor+1&&g.actorCount==count&&g.life.succession==rounds);
}
static void rival(void) {
    game_new(73819,0);
    int e=9, t=eliteDefs[e].town, old=g.civil.eliteActor[e]-1;
    assert(old>=0&&g.actors[old].gear[0]==REGALIA_ITEMS+2*e);
    g.actors[old].dead=1;
    politics_tick(1);
    TownPolitics *p=&g.life.towns[t];
    assert(p->stage==1&&p->elite==e);
    int contender=p->rival;
    char name[16];
    snprintf(name,sizeof(name),"%s",politics_claimant(t,contender));
    settle(p,1);
    assert(g.townOwner[t]==contender&&p->leader);
    assert(!strcmp(g.actors[p->leader-1].name,name));
    assert(g.actors[old].dead&&g.actors[old].gear[0]==REGALIA_ITEMS+2*e);
    int copies=0;
    for(int n=0;n<g.actorCount;n++)copies+=g.actors[n].gear[0]==REGALIA_ITEMS+2*e;
    assert(copies==1);
}
static void full_roster(void) {
    game_new(73819,0);
    int t=OLD_TOWNS;
    g.actors[g.war.resident[0][0]-1].dead=1;
    g.actors[g.war.resident[0][1]-1].dead=1;
    g.actorCount=MAX_ACTORS;
    for(int n=0;n<MAX_ACTORS;n++)if(g.living.owned[n]<0)g.living.owned[n]=0;
    politics_tick(1);
    TownPolitics *p=&g.life.towns[t];
    settle(p,0);
    assert(!p->leader);
    g.living.owned[MAX_ACTORS-1]=-1;
    politics_tick(1);
    assert(p->leader==MAX_ACTORS&&politics_leader_title(MAX_ACTORS-1));
    politics_tick(1);
    assert(p->leader==MAX_ACTORS);
}
int main(int argc,char **argv) {
    for(int i=0;i<WAR_FACTIONS;i++)capital(i);
    rival();
    full_roster();
    if(argc>1){
        assert(load_game(argv[1]));
        assert(g.war.version&&g.townOwner[OLD_TOWNS]==UNION);
    }
    puts("PASS: successive appointments in all three capitals, rival wins at older towns, regalia stays unique, full rosters retry, saves preserve vacancies and incumbents.");
}
