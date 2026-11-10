#include "game.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
extern int conscious(const Actor*);
extern int add_actor(const char*,int,int,float,float,int);
extern int hostile(const Actor*,const Actor*);
#define S (g.society)
static float bound(float v,float a,float b){return fminf(b,fmaxf(a,v));}
static int actor_no(const Actor*a){return (uintptr_t)a>=(uintptr_t)g.actors&&(uintptr_t)a<(uintptr_t)(g.actors+g.actorCount)?(int)(a-g.actors):-1;}
const char *weaponClassNames[WEAPON_CLASSES] = {
    "Slashing Blades", "Light Blades", "Heavy Weapons", "Polearms", "Bows & Crossbows", "Firearms"
};
static const unsigned char styleSkill[WEAPON_STYLES] = {
    [WS_KATANA] = WC_SLASHING, [WS_SABRE] = WC_SLASHING,
    [WS_HACKER] = WC_HEAVY, [WS_HEAVY] = WC_HEAVY, [WS_BLUNT] = WC_HEAVY,
    [WS_POLEARM] = WC_POLEARMS, [WS_RAPIER] = WC_LIGHT, [WS_KNIFE] = WC_LIGHT,
    [WS_BOW] = WC_ARCHERY, [WS_CROSSBOW] = WC_ARCHERY, [WS_FIREARM] = WC_FIREARMS
};
_Static_assert(WEAPON_STYLES == WEAPON_XP_SLOTS, "Legacy weapon XP order");
int weapon_style(int it) {
    if (!it) return -1;
    if (weapon_rapier(it)) return WS_RAPIER;
    if (it == RUSTSWORD || it == SABRE || (it >= SCIMITAR_ITEMS && it < SCIMITAR_ITEMS + 24))
        return WS_SABRE;
    if (it == CLEAVER) return WS_HACKER;
    if (it == STAFF) return WS_POLEARM;
    if (it == CROSSBOW) return WS_CROSSBOW;
    int family = gear_family(it);
    switch (family) {
    case 0: case 1: case 2: return WS_KATANA;
    case 3: case 9: return WS_KNIFE;
    case 4: case 5: case 6: case 16: case 86: return WS_POLEARM;
    case 11: case 12: case 13: case 87: return WS_HACKER;
    case 20: case 84: return WS_HEAVY;
    case 14: case 15: case 23: case 85: return WS_BLUNT;
    case 24: case 28: return WS_BOW;
    case 25: case 26: case 27: return WS_CROSSBOW;
    case 29: case 30: case 31: return WS_FIREARM;
    default: return family >= 0 && family < 24 ? WS_SABRE : -1;
    }
}
int weapon_class(int it) {
    int style = weapon_style(it);
    return style < 0 ? -1 : styleSkill[style];
}
static void merge_weapon_xp(float xp[WEAPON_XP_SLOTS]) {
    float merged[WEAPON_CLASSES] = {0};
    for (int i = 0; i < WEAPON_XP_SLOTS; i++)
        merged[styleSkill[i]] += xp[i];
    memset(xp, 0, sizeof(float) * WEAPON_XP_SLOTS);
    for (int i = 0; i < WEAPON_CLASSES; i++)
        xp[i] = fminf(SKILL_XP_CAP, merged[i]);
}
void weapon_xp_migrate(Game *save) {
    for (int actor = 0; actor < MAX_ACTORS; actor++)
        merge_weapon_xp(save->society.weaponXP[actor]);
    if (save->population.version) {
        for (int group = 0; group < POP_GROUPS; group++)
            for (int member = 0; member < save->population.groups[group].count; member++)
                merge_weapon_xp(save->population.social[group][member].weaponXP);
    }
}
int weapon_class_level(const Actor*a,int c){int n=actor_no(a);if(c<0||c>=WEAPON_CLASSES||n<0)return 1;if(!a->squad)return level(a,c>=WC_ARCHERY?CROSSBOWS:MELEE);return (int)bound(1+sqrtf(S.weaponXP[n][c]/12),1,SKILL_CAP);}
void weapon_class_gain(Actor*a,int w,float amount){int n=actor_no(a),c=weapon_class(w);if(n<0||c<0||!a->squad)return;S.weaponXP[n][c]=bound(S.weaponXP[n][c]+amount,0,SKILL_XP_CAP);}
float weapon_cut_ratio(int it) {
    if (!it) return 1;
    if (it == STAFF || gear_family(it) == 16) return .05f;
    switch (weapon_style(it)) {
    case WS_BLUNT: return .05f;
    case WS_HEAVY: return .45f;
    case WS_HACKER: return .55f;
    case WS_KATANA: return .9f;
    case WS_SABRE: return .8f;
    case WS_RAPIER: case WS_KNIFE: return .95f;
    default: return .75f;
    }
}
float combat_learning(float learner,float opponent){float d=opponent-learner;return bound(1+d*(d>=0?.1f:.04f),.1f,6);}
int combat_evasion(const Actor*a){return !a->gear[0]||weapon_ranged(a->gear[0])||a->stun>0;}
float dodge_mobility(const Actor*a){float capacity=actor_capacity(a);return bound(1-fmaxf(0,encumbrance(a)-capacity)/fmaxf(1,capacity),.05f,1);}
static const int exports[TOWNS]={GRAIN,COPPER,WATER,HIDE,MEAT,ORE,IRONBAR,CLOTH,PARTS,WOOD,COAL,IRONBAR,MEDKIT,GRAIN,MEAT,WOOD,PARTS,IRONBAR,MEDKIT};
int town_export(int town) { return exports[town]; }
static const int imports[TOWNS]={IRONBAR,WOOD,GRAIN,MEDKIT,CLOTH,WATER,COAL,MEAT,MEDKIT,IRONBAR,WOOD,PARTS,WATER,ORE,MEDKIT,PARTS,COAL,GRAIN,WATER};
static int commodity(int it){return it==RATION||it==MEDKIT||it==ORE||it==COPPER||it==WOOD||it==CLOTH||it==WATER||it==GRAIN||it==MEAT||it==IRONBAR||it==HIDE||it==COAL||it==PARTS||it==TONIC||it==RELIC;}
float market_factor(int town,int item){
 if(!commodity(item))return 1;
 float v=item==exports[town]?.59f:item==imports[town]?1.72f:1.f;
 if(item==RATION&&exports[town]==GRAIN)v=.68f;
 if(item==RATION&&imports[town]==GRAIN)v=1.55f;
 int stock=g.stock[town][item];v+=stock<4?.25f:stock<10?.1f:stock>45?-.12f:0;
 if(S.crisis[town]==1&&(item==RATION||item==GRAIN||item==WATER))v+=.4f;
 if(S.crisis[town]==2&&(item==MEDKIT||item==IRONBAR||item==PARTS))v+=.35f;
 return bound(v,.48f,2.25f);
}
int market_percent(int t,int it,int selling){float normal=itemDefs[it].price*(selling?.75f:1);return normal>0?(int)roundf((item_price(t,it,selling)/normal-1)*100):0;}
const char *trade_tip(int town){
 static char line[240];int bestItem=0,bestTown=-1,bestProfit=0;
 if(town<0)return "Ask a town merchant for current prices and transport work.";
 for(int it=1;it<BASE_ITEMS;it++)if(commodity(it)&&g.stock[town][it]>0)for(int t=0;t<TOWNS;t++)if(t!=town){int profit=item_price(t,it,1)-item_price(town,it,0);if(profit>bestProfit){bestProfit=profit;bestItem=it;bestTown=t;}}
 if(bestTown>=0)snprintf(line,sizeof(line),"%s: buy here for %d, sell at %s for %d. About %d silver per unit before travel costs. Supply and unrest change prices.",itemDefs[bestItem].name,item_price(town,bestItem,0),towns[bestTown].name,item_price(bestTown,bestItem,1),bestProfit);
 else snprintf(line,sizeof(line),"No profitable export quote today. We produce %s and need %s. Ask again after trade or shortages change stock.",itemDefs[exports[town]].name,itemDefs[imports[town]].name);
 return line;
}
void society_stock(int t){
 g.stock[t][exports[t]]=40;g.stock[t][imports[t]]=6;
 for(int k=0;k<3;k++){int v=(t*3+g.day+k*7)%24;g.stock[t][SCIMITAR_ITEMS+v]=1;g.stock[t][RAPIER_ITEMS+(v+5)%24]=1;}
 if(t==8){g.stock[t][gear_id(37,0,1)]=2;g.stock[t][gear_id(55,0,1)]=2;}
 if(t==12){g.stock[t][gear_id(39,0,1)]=2;g.stock[t][gear_id(57,0,1)]=2;}
 if(t==6){g.stock[t][gear_id(34,1,1)]=2;g.stock[t][gear_id(50,1,1)]=2;}
}
void faction_goodwill(int faction, int amount) {
    if (faction <= FREE || faction >= WILDLIFE)
        return;
    g.rep[faction] += amount;
    log_progress("Standing with %s improved (+%d).", factionNames[faction], amount);
}
void trade_goodwill(int faction, int price) {
    if (faction <= FREE || faction >= WILDLIFE || g.rep[faction] >= 20)
        return;
    int total = g.field.tradeCredit[faction] + (int)fminf(price, 360);
    g.field.tradeCredit[faction] = total % 120;
    if (total >= 120)
        faction_goodwill(faction, total / 120);
}
int wanted_amount(const Actor*a,int f){int n=actor_no(a);if(n<0||f<0||f>=FACTIONS)return 0;int sum=0;for(int j=0;j<FACTIONS;j++)sum+=S.wanted[n][j];return sum?S.wanted[n][f]:f==UNION?a->bounty:0;}
static void wanted_sum(Actor*a){int n=actor_no(a);a->bounty=0;for(int f=0;f<FACTIONS;f++)a->bounty+=S.wanted[n][f];}
void crime_charge(Actor *a, int f, int amount, unsigned reason) {
    int n = actor_no(a);
    if (n < 0 || f <= FREE || f >= WILDLIFE || amount <= 0)
        return;
    int listed = 0;
    for (int j = 0; j < FACTIONS; j++) listed += S.wanted[n][j];
    if (!listed && a->bounty > 0) S.wanted[n][UNION] = a->bounty;
    if (S.wanted[n][f] && !g.charges[n][f]) g.charges[n][f] = CRIME_PRIOR;
    S.wanted[n][f] = (int)bound(S.wanted[n][f] + amount, 0, 100000);
    g.charges[n][f] |= reason;
    S.recognized[n][f] = 600;
    wanted_sum(a);
}
void crime_add(Actor *a, int f, int amount) { crime_charge(a, f, amount, CRIME_PRIOR); }
void crime_clear(Actor *a, int f) {
    int n = actor_no(a);
    if (n < 0 || f <= FREE || f >= FACTIONS)
        return;
    if (wanted_amount(a, f)) {
        if (g.rep[f] < -20) g.rep[f] = -20;
        g.depth.actors[n].trespass = 0;
        for (int i = 0; i < g.actorCount; i++) {
            Actor *b = &g.actors[i];
            if (b->squad || b->faction != f)
                continue;
            if (watch_town(b) >= 0 || b->target == n || (b->attackPhase && b->attackTarget == n))
                g.living.anger[i] = 0;
            if (b->target == n) b->target = -1;
            if (b->attackTarget == n) b->attackPhase = 0;
            if (a->target == i) a->target = -1;
            if (a->attackTarget == i) a->attackPhase = 0;
            if (g.living.command[n] == i) g.living.command[n] = -1;
        }
    }
    S.wanted[n][f] = 0;
    S.recognized[n][f] = 0;
    g.charges[n][f] = 0;
    wanted_sum(a);
}
int crime_pay(Actor *a, int f) {
    int amount = wanted_amount(a, f);
    if (amount <= 0)
        return 0;
    if (g.money < amount) {
        log_msg("You need %d silver to pay this bounty.", amount);
        return 0;
    }
    g.money -= amount;
    crime_clear(a, f);
    log_msg("Paid %d silver. %s cleared your charges.", amount, factionNames[f]);
    return 1;
}
void crime_description(const Actor *a, int f, char *out, int size) {
    static const char *names[] = {"Theft", "Pickpocketing", "Assault", "Trespass",
        "Selling stolen goods", "Jail escape", "Helping a prisoner escape", "False disguise", "Prior charge"};
    unsigned mask = g.charges[a - g.actors][f];
    if (!mask) mask = CRIME_PRIOR;
    out[0] = 0;
    int used = 0;
    for (unsigned i = 0; i < sizeof(names) / sizeof(*names) && used < size; i++)
        if (mask & (1u << i))
            used += snprintf(out + used, size - used, "%s%s", used ? ", " : "", names[i]);
}
int watch_town(const Actor *guard) {
    int t = guard->homeTown;
    return !guard->squad && guard->kind == GUARD && guard->voice != ROLE_MERCENARY &&
           t >= 0 && t < TOWNS &&
           guard->faction == g.townOwner[t] && g.field.jail[t] ? t : -1;
}
int disguise_faction(const Actor*a){
 int n=actor_no(a),bodyItem=a->gear[1];if(!bodyItem&&n>=0)bodyItem=g.depth.actors[n].layer;
 int body=gear_family(bodyItem),head=gear_family(a->gear[2]);
 if(body==39&&head==57)return MONKS;
 if((body==34||body==47)&&(head==50||head==58||head==59))return SHOGUN;
 if(body==37&&head==55)return HOSTILE;
 if(body==36&&head==51)return OUTLAWS;
 if(body==42&&head==53)return UNION;
 if(body==32&&head==49)return SCAVENGERS;
 return FREE;
}
int disguise_score(const Actor*a,int f){if(disguise_faction(a)!=f)return 0;int v=55+itemDefs[a->gear[1]].quality*5+level(a,STEALTH)/2-(a->sneaking?18:0)-(a->autoattack?12:0);if(f==MONKS&&a->prostheticMask)v=0;return (int)bound(v,0,95);}
const char *town_custom(int t){
 const char *capital=war_capital_custom(t);if(capital)return capital;
 if(t==7)return "ASH MONASTERY: only robed initiates enter. Robe and cowl required; exposed prosthetics are heresy. Lingering near inquisitors risks recognition.";
 if(t==11)return "SHOGUN'S REST: closed garrison. Lamellar or captain armor plus a shogunate helm opens the gate. Officers inspect strangers at close range.";
 if(t==14)return "VULTURE NEST: a flesh cult. Bone harness and oni mask blend in. Outsiders are beaten, carried to the cooking ground, bound and eaten. Rescue is possible.";
 if(t==8)return "BLACK MESA: smugglers sell forbidden uniforms and machine parts. Ordinary stolen goods find buyers here, but local theft still has consequences.";
 if(t==12)return "PALE PASS: pilgrim sanctuary; medicine is produced here, water is scarce. Pilgrim clothing is sold openly.";
 return "Local production, shortages and faction control change trade. Ask merchants for a route and marshals for work.";
}
static int restricted(int t){return (t==7||t==11||t==14||t==OLD_TOWNS+1)&&g.townOwner[t]==towns[t].faction;}
static int guard_exists(int t){for(int n=0;n<g.actorCount;n++){Actor*b=&g.actors[n];if(!b->squad&&b->kind==GUARD&&b->homeTown==t&&!b->dead&&b->unconscious<=0)return 1;}return 0;}
int society_gate(Actor*a,float x,float y){
 int n=actor_no(a);if(n<0||!a->squad)return 1;
 for(int t=0;t<TOWNS;t++)if(restricted(t)&&distance(x,y,towns[t].x,towns[t].y)<18&&distance(a->x,a->y,towns[t].x,towns[t].y)>=18&&guard_exists(t)){
  int f=g.townOwner[t];if(disguise_score(a,f)>=40&&S.recognized[n][f]<=0)return 1;
  if(S.gateMessage[n]<=0){log_msg("%s: matching body armor and headwear required.",towns[t].name);S.gateMessage[n]=5;}return 0;
 }return 1;
}
/* -1 means defer to the normal faction relationship. */
int society_hostile(const Actor*a,const Actor*b){
 int ai=actor_no(a),bi=actor_no(b),pa=a->squad||(ai>=0&&S.freed[ai]),pb=b->squad||(bi>=0&&S.freed[bi]);
 const Actor*p=pa?a:pb?b:NULL,*w=pa?b:a;if(!p||w->squad||(pa&&pb))return -1;
 int n=actor_no(p),wi=actor_no(w),f=w->faction,t=w->homeTown;if(n<0||f==FREE||f==WILDLIFE)return -1;
 if(disguise_score(p,f)>=40&&S.recognized[n][f]<=0&&(actor_no(w)<0||!g.living.anger[actor_no(w)]))return 0;
 if (slave_in_camp(p) && S.captorFaction[n] == f) return 0;
 int room = depth_room(p);
 int bounty = w->kind == GUARD ? wanted_amount(p, f) : 0;
 if (bounty >= SERIOUS_BOUNTY && watch_town(w) >= 0) return 1;
 if (bounty > 0 && watch_town(w) >= 0 && (wi < 0 || !g.living.anger[wi])) return 0;
 if (room && g.buildings[room - 1].owner < 0 && g.buildings[room - 1].faction == f &&
     g.rep[f] < -25 && g.depth.actors[n].trespass < 7 && (wi < 0 || !g.living.anger[wi])) return 0;
 if (room && g.buildings[room - 1].owner < 0 && g.buildings[room - 1].faction == f &&
     g.rep[f] < -25 && g.depth.actors[n].trespass >= 7) return 1;
 if (w->kind == GUARD && (g.field.escaped[n] & (1u << f))) return 1;
 if (!room && w->kind == GUARD && t < 0 && !strcmp(w->name, "Camp overseer") &&
     !slave_in_camp(p) && distance(p->x, p->y, w->homeX, w->homeY) < 10) return 1;
 if(t>=0&&restricted(t)&&distance(p->x,p->y,towns[t].x,towns[t].y)<18&&disguise_score(p,f)<40)return 1;
 return -1;
}
void society_actor_reset(int n){
 if(n<0||n>=MAX_ACTORS)return;
 g.war.prisonTown[n]=0;
 memset(g.charges[n],0,sizeof(g.charges[n]));
 memset(S.weaponXP[n],0,sizeof(S.weaponXP[n]));memset(S.wanted[n],0,sizeof(S.wanted[n]));memset(S.recognized[n],0,sizeof(S.recognized[n]));
 S.queueCount[n]=S.cannibal[n]=S.rescueHelper[n]=S.freed[n]=S.captorFaction[n]=0;
 S.eating[n]=S.suspicion[n]=S.gateMessage[n]=0;
 for(int i=0;i<MAX_ACTORS;i++)if(S.rescueHelper[i]==n+1){S.rescueHelper[i]=0;g.depth.actors[i].escape=0;}
}
int society_rescue_busy(int helper){for(int n=0;n<g.actorCount;n++)if(S.rescueHelper[n]==helper+1)return 1;return 0;}
void society_rescue_cancel(int helper){for(int n=0;n<g.actorCount;n++)if(S.rescueHelper[n]==helper+1){S.rescueHelper[n]=0;g.depth.actors[n].escape=0;}}
static int rescue_faction(int n){
 Actor*a=&g.actors[n];if(a->prisonTime>0&&a->prisonTown>=0)return g.townOwner[a->prisonTown];
 if(S.captorFaction[n])return S.captorFaction[n];return g.depth.actors[n].captive==3?HOSTILE:OUTLAWS;
}
static int rescue_guard(int n,float reach){
 Actor*a=&g.actors[n];int faction=rescue_faction(n);
 for(int j=0;j<g.actorCount;j++){Actor*b=&g.actors[j];if(j!=n&&!b->squad&&conscious(b)&&(b->kind==GUARD||b->kind==RAIDER)&&
  (b->faction==faction||(faction==OUTLAWS&&b->faction==HOSTILE))&&depth_distance(a,b)<reach)return j;}
 return -1;
}
int society_rescue_start(int n){
 if(n<0||n>=g.actorCount)return 0;Actor*a=&g.actors[g.selected],*b=&g.actors[n];DepthActor*z=&g.depth.actors[n];
 if(!conscious(a)||!b->active||b->dead||b->carriedBy>=0||depth_room(a)!=depth_room(b)||depth_distance(a,b)>TALK_REACH||(!z->captive&&b->prisonTime<=0))return 0;
 if(z->captive==2||S.cannibal[n]==1){log_msg("Stop the captor and set the prisoner down first.");return 0;}
 if(S.rescueHelper[n]&&S.rescueHelper[n]!=g.selected+1){log_msg("An ally is already working those restraints.");return 0;}
 society_rescue_cancel(g.selected);S.captorFaction[n]=rescue_faction(n);S.rescueHelper[n]=g.selected+1;z->escape=1;a->taskKind=0;
 log_msg("Picking %s's restraints. Stay close; guards can catch you.",b->name);depth_open(SCREEN_WORLD);return 1;
}
static void rescue_refuge(Actor*a,int former){
 int town=-1;float best=1e9f;
 for(int t=0;t<TOWNS;t++)if(g.townOwner[t]!=former&&g.townOwner[t]!=HOSTILE&&g.townOwner[t]!=OUTLAWS){float d=distance(a->x,a->y,towns[t].x,towns[t].y);if(d<best){town=t;best=d;}}
 if(town<0)town=0;a->homeTown=town;a->homeX=towns[town].x-9;a->homeY=towns[town].y+6;
 for(int r=0;r<12&&!can_walk(a->homeX,a->homeY);r++){a->homeX=towns[town].x-8+(r%4);a->homeY=towns[town].y+7+r/4;}
 a->tx=a->homeX;a->ty=a->homeY;a->routeLen=a->routePos=0;
}
static void rescue_finish(int n,int helper){
 Actor*a=&g.actors[n];DepthActor*z=&g.depth.actors[n];int faction=rescue_faction(n);
 if (g.field.laborCamp[n]) {
     g.field.laborCamp[n] = 0;
     g.field.escaped[n] |= 1u << faction;
 }
 g.field.cellOpen[n] = 0;
 z->escape=0;z->captive=0;z->recognized=helper>=0?1800:0;S.cannibal[n]=0;S.eating[n]=0;S.rescueHelper[n]=0;
 g.war.prisonTown[n]=0;
 if (a->taskKind == TASK_CELL_LOCK || a->taskKind == TASK_JAIL_CARE) a->taskKind = 0;
 a->prisonTime=0;a->prisonTown=-1;a->target=-1;a->attackPhase=0;g.living.command[n]=-1;
 S.captorFaction[n]=faction;
 if(helper>=0){crime_charge(a,faction,80,CRIME_ESCAPE);S.recognized[n][faction]=1800;g.depth.freed++;}
 if(z->confiscated&&!a->gear[0]){a->gear[0]=z->confiscated;a->gearStolen[0]=z->confiscatedOwner;z->confiscated=z->confiscatedOwner=0;}
 if(!a->squad){S.freed[n]=helper>=0&&(g.seed+(unsigned)n)%3==0?1:2;a->kind=SETTLER;a->faction=FREE;a->autoattack=0;rescue_refuge(a,faction);}
 if(helper>=0)log_warn("%s is free.%s",a->name,a->squad?"":S.freed[n]==1?" They choose to follow. Recruit or dismiss through nearby actions.":" They are running for refuge.");
}
void society_release_war_prisoners(int town){
 for(int n=0;n<g.actorCount;n++)if(g.war.prisonTown[n]==town+1&&S.captorFaction[n]!=g.townOwner[town]){
  if(!g.actors[n].dead)rescue_finish(n,-1);
  else g.war.prisonTown[n]=0;
 }
}
void society_rescue_order(int n,int order){
 if(n<0||n>=g.actorCount)return;Actor*a=&g.actors[n],*p=&g.actors[g.selected];
 if(!S.freed[n]||a->squad||a->dead||!conscious(p)||g.depth.actors[n].captive||a->prisonTime>0||depth_distance(a,p)>TALK_REACH)return;
 if(order==ACTION_RESCUED_RECRUIT){if(g.squadCount>=SQUAD){log_msg("Squad full. They remain free and can follow or seek refuge.");return;}
  a->squad=1;a->kind=PLAYER;a->faction=FREE;a->job=JOB_FOLLOW;a->autoattack=1;g.squadCount++;g.depth.actors[n].group=g.depth.actors[g.selected].group;S.freed[n]=0;log_msg("%s joins your expedition.",a->name);
 }else if(order==ACTION_RESCUED_LEAVE){S.freed[n]=2;rescue_refuge(a,S.captorFaction[n]);log_msg("%s: Fine. I will make my own way to %s.",a->name,towns[a->homeTown].name);}
 else{S.freed[n]=1;log_msg("%s: I will stay close. Lead the way.",a->name);}
}
static void tick_rescues(float dt){
 for(int n=0;n<g.actorCount;n++)if(S.rescueHelper[n]){
  int h=S.rescueHelper[n]-1;Actor*a=&g.actors[n],*helper=&g.actors[h];DepthActor*z=&g.depth.actors[n];int guard=rescue_guard(n,4);
  if(a->dead||!a->active||!conscious(helper)||a->carriedBy>=0||depth_room(helper)!=depth_room(a)||depth_distance(helper,a)>TALK_REACH||(!z->captive&&a->prisonTime<=0)||guard>=0){
   S.rescueHelper[n]=0;z->escape=0;
   if(guard>=0){crime_charge(helper,g.actors[guard].faction,80,CRIME_AID_ESCAPE);g.actors[guard].target=h;log_warn("Caught freeing a prisoner! %s alerts the guards.",g.actors[guard].name);}
   else log_msg("Rescue interrupted. Return to the restraints.");continue;
  }
  z->escape+=dt*(inventory_count(helper,TOOLS)?2:1)*(1+level(helper,LOCKPICK)*.02f);gain(helper,LOCKPICK,dt*2);
  if(z->escape>=18)rescue_finish(n,h);
 }
 for(int n=0;n<g.actorCount;n++){Actor*a=&g.actors[n];if(S.freed[n]&&conscious(a)&&!a->squad&&distance(a->x,a->y,g.actors[g.selected].x,g.actors[g.selected].y)>45)society_npc_job(n,dt);}
}
static void seed_prisoners(void){
 if(S.prisonSeeded)return;S.prisonSeeded=1;
 for(int j=0;j<2;j++){int t=j?6:0;float x=towns[t].x-9,y=towns[t].y+5;
  for(int k=0;k<12&&!can_walk(x,y);k++){x=towns[t].x-9+k%4;y=towns[t].y+6+k/4;}
  if(!can_walk(x,y))continue;int n=add_actor(j?"Kest":"Ilan",SETTLER,FREE,x,y,4+j*3);if(n<0)continue;
  Actor*a=&g.actors[n];a->prisonTown=t;a->prisonTime=7200;a->gear[1]=RAGS;a->gear[2]=0;g.depth.actors[n].confiscated=a->gear[0];a->gear[0]=0;
  S.captorFaction[n]=g.townOwner[t];crime_charge(a,g.townOwner[t],120,CRIME_TRESPASS);
 }
}

void society_init(void){
 if(S.version){seed_prisoners();return;}memset(&S,0,sizeof(S));S.version=1;S.lastEconomyDay=g.day;
 for(int n=0;n<g.actorCount;n++)if(g.actors[n].squad&&g.actors[n].bounty>0)S.wanted[n][UNION]=g.actors[n].bounty;
 for(int t=0;t<TOWNS;t++){society_stock(t);int armed=0;for(int n=0;n<g.actorCount;n++){
  Actor*a=&g.actors[n];if(a->squad||a->homeTown!=t)continue;
  if(a->kind==GUARD&&t>=5&&!armed++){a->gear[0]=UNIQUE_ITEMS+t%8;a->gear[1]=UNIQUE_ITEMS+8+t%6;}
  if(a->kind==GUARD&&t==10&&armed==1)a->gear[5]=UNIQUE_ITEMS+14;
  if(a->kind==GUARD&&t==12&&armed==1)a->gear[2]=UNIQUE_ITEMS+15;
  if(a->kind==GUARD&&(t==7||t==11||t==14)){a->gear[1]=gear_id(t==7?39:t==11?34:37,t==11?1:0,2);a->gear[2]=gear_id(t==7?57:t==11?50:55,t==11?1:0,2);snprintf(a->name,sizeof(a->name),"%s",t==7?"Ash inquisitor":t==11?"Gate inspector":"Feast butcher");}
 }}
 seed_prisoners();
}
int bounty_contract_active(int actor){for(int i=0;i<CONTRACTS;i++)if(g.contracts[i].active==1&&g.contracts[i].type==7&&S.jobActor[i]==actor+1)return 1;return 0;}
int escort_contact(int slot) {
    const Contract *c = &g.contracts[slot];
    int n = S.jobActor[slot] - 1;
    return c->active == 1 && c->type == CONTRACT_ESCORT && !c->progress &&
           n >= 0 && n < g.actorCount && g.actors[n].active && !g.actors[n].dead ? n : -1;
}
int escorting_actor(int actor) {
    for (int i = 0; i < CONTRACTS; i++)
        if (escort_contact(i) == actor) return 1;
    return 0;
}
static int escort_near(const Actor *caravan, float reach) {
    for (int n = 0; n < g.actorCount; n++) {
        const Actor *a = &g.actors[n];
        if (a->squad && conscious(a) && depth_distance(a, caravan) <= reach)
            return 1;
    }
    return 0;
}
static int service_actor(int town, int kind) {
    for (int n = 0; n < g.actorCount; n++) {
        Actor *a = &g.actors[n];
        if (!conscious(a) || a->squad || living_owned(n) || civil_elite_index(n) >= 0 ||
            a->prisonTime > 0 || g.depth.actors[n].captive)
            continue;
        int match = kind == CONTRACT_BOUNTY ? a->kind == RAIDER && a->bountyReward > 0 :
            a->homeTown == town && a->kind == (kind == CONTRACT_ESCORT ? CARAVAN : GUARD) &&
            a->faction == g.townOwner[town] &&
            (kind != CONTRACT_ESCORT || (!population_owned(n) && !depth_room(a) && a->target < 0 &&
             distance(a->x, a->y, towns[town].x, towns[town].y) < 18));
        if (!match)
            continue;
        int used = 0;
        for (int i = 0; i < CONTRACTS; i++)
            if (g.contracts[i].active == 1 && S.jobActor[i] == n + 1)
                used = 1;
        if (!used)
            return n;
    }
    return -1;
}
static int can_spawn_convoy(void) {
    int count = 0;
    for (int n = 0; n < g.actorCount; n++)
        if (g.actors[n].active && !g.actors[n].dead && g.actors[n].kind == CARAVAN &&
            !strcmp(g.actors[n].name, "Freight caravan") && !population_owned(n))
            count++;
    return count < TOWNS && g.actorCount < MAX_ACTORS;
}
int society_job_available(int town, int kind) {
    return service_actor(town, kind) >= 0 || (kind == CONTRACT_ESCORT && can_spawn_convoy());
}
int society_bounty_reward(int town) {
    int who = service_actor(town, CONTRACT_BOUNTY);
    return bounty_reward(who, 1);
}
int society_contract(int town,int kind){
 if(town<0||town>=TOWNS||kind<4||kind>7)return 0;
 int slot=-1,live=0;for(int i=0;i<CONTRACTS;i++){if(g.contracts[i].active==1){if(g.contracts[i].town==town&&g.contracts[i].type==kind){log_msg("That job is already active.");return 0;}if(g.contracts[i].type==5||g.contracts[i].type==6)live++;}else if(slot<0)slot=i;}
 if(slot<0){log_msg("Finish an active contract first.");return 0;}if((kind==5||kind==6)&&live>=4){log_msg("Four service jobs already active.");return 0;}
 int actor=-1;Actor*p=&g.actors[g.selected];
 if(kind >= CONTRACT_ESCORT) {
  actor = service_actor(town, kind);
  if(actor<0&&kind==CONTRACT_ESCORT){
   if(can_spawn_convoy()){float x=towns[town].x,y=towns[town].y+12;for(int j=0;j<12&&!can_walk(x,y);j++)x+=1;
    if(can_walk(x,y)){actor=add_actor("Freight caravan",CARAVAN,g.townOwner[town],x,y,14);if(actor>=0){g.actors[actor].homeTown=town;g.actors[actor].gear[0]=RAPIER_ITEMS+5;inventory_add(&g.actors[actor],exports[town],4);}}}
  }
  if(actor<0){log_msg(kind==7?"No unclaimed named bounty today.":"No free employer here; finish current work.");return 0;}
 }
 Contract c={0};c.active=1;c.type=kind;c.town=town;c.target=contract_destination(town, g.townOwner[town]);c.reward=contract_reward(town, kind);c.needed=kind==4?4:1;
 if(kind==4){c.item=exports[town];int bond=item_price(town,c.item,0)*c.needed;if(g.money<bond){log_msg("Freight deposit: %d silver, refunded on delivery.",bond);return 0;}
  if(!inventory_add(p,c.item,c.needed)){log_msg("Make room for four units of freight.");return 0;}g.money-=bond;S.jobBond[slot]=bond;
 }else S.jobBond[slot]=0;
 if(kind==7)c.reward=bounty_reward(actor,1);
 g.contracts[slot]=c;g.frontier.giver[slot]=ui.screen==SCREEN_DIALOG?ui.dialogActor+1:work_contact(town)+1;S.jobActor[slot]=actor+1;S.jobStage[slot]=0;S.jobWarned[slot]=0;S.jobFar[slot]=S.jobWarning[slot]=0;S.jobElapsed[slot]=S.jobNear[slot]=S.jobPaid[slot]=S.jobDistance[slot]=0;
 if(actor>=0){S.jobX[slot]=g.actors[actor].x;S.jobY[slot]=g.actors[actor].y;}
 if(kind==CONTRACT_ESCORT) {
  log_msg("Meet %s outside %s.", g.actors[actor].name, towns[town].name);
  return 1;
 }
 if(kind==CONTRACT_PATROL){log_msg("Report to %s.",g.actors[actor].name);return 1;}
 log_msg(kind==CONTRACT_FREIGHT ? "Freight loaded." : "Bounty accepted.");return 1;
}
void society_contract_text(int n, char *out, int size) {
    const Contract *c = &g.contracts[n];
    int who = S.jobActor[n] - 1, valid = who >= 0 && who < g.actorCount;
    const Actor *employer = valid ? &g.actors[who] : NULL;
    const char *name = valid ? employer->name : "the employer";
    if (c->active == 3)
        snprintf(out, size, "Job failed. Wages earned so far are yours.");
    else if (c->type == CONTRACT_FREIGHT)
        snprintf(out, size, "The merchants need %s in %s. Deliver %d (%d/%d in squad). Speak to the local contact for %d silver and your %d deposit.",
                 itemDefs[c->item].name, towns[c->target].name, c->needed,
                 contract_item_count(c->item, -1), c->needed, c->reward, S.jobBond[n]);
    else if (c->type == CONTRACT_BOUNTY)
        snprintf(out, size, "%s is wanted for road raids. Carry them to a paying jailkeeper. See their belongings for the list.",
                 name);
    else if (contract_ready(n))
        snprintf(out, size, "Job complete. Speak to your contact in %s.",
                 towns[contract_turn_in(n)].name);
    else if (c->type == CONTRACT_ESCORT && employer) {
        float gap = distance(g.actors[g.selected].x, g.actors[g.selected].y, employer->x, employer->y);
        if (!S.jobStage[n])
            snprintf(out, size, "Meet %s at %s. %.0fm away.", name, towns[c->town].name, gap);
        else
            snprintf(out, size, "%s to %s. %s %.0fm away.", name, towns[c->target].name,
                     escort_near(employer, 8) ? "" : "Waiting.", gap);
    }
    else
        snprintf(out, size, "The watch is short on hands. Patrol within 8m of %s (%d/180s, %d/35m).",
                 name, (int)fminf(180, S.jobNear[n]), (int)fminf(35, S.jobDistance[n]));
}
int society_complete(int n){if(n<0||n>=CONTRACTS||g.contracts[n].active!=1)return 0;Contract*c=&g.contracts[n];Actor*a=&g.actors[g.selected];int town=contract_turn_in(n);if(town_near(a->x,a->y,14)!=town){log_msg("Claim at %s.",towns[town].name);return 0;}
 int reward=c->reward+S.jobBond[n];
 if(c->type==4){if(!contract_use_items(c->item,c->needed,town)){log_msg("Bring the full shipment and its carriers.");return 0;}g.stock[town][c->item]+=c->needed;}
 else if(c->type==CONTRACT_BOUNTY){log_msg("Take the target to a jailkeeper.");return 0;}
 else if(!c->progress){log_msg("Complete the escort or patrol first.");return 0;}
 c->active=2;c->completed=1;remember_contract(c);g.money+=reward;g.contractsDone++;faction_goodwill(g.townOwner[town],5);S.jobBond[n]=0;log_level("%s paid %d silver for %s.", towns[town].name, reward, contract_title(c->type));return 1;
}
int society_npc_job(int n,float dt){
 Actor*freed=&g.actors[n];if(S.freed[n]&&!freed->squad){
  if(!conscious(freed)||freed->prisonTime>0||g.depth.actors[n].captive)return 1;
  if(S.freed[n]==1){Actor*leader=&g.actors[g.selected];if(depth_room(leader)==depth_room(freed)&&depth_distance(leader,freed)>2.4f)route_toward(freed,leader->x,leader->y,dt);}
  else if(S.freed[n]==2){route_toward(freed,freed->homeX,freed->homeY,dt);if(distance(freed->x,freed->y,freed->homeX,freed->homeY)<2){S.freed[n]=3;log_msg("%s reached refuge at %s.",freed->name,towns[freed->homeTown].name);}}
  return 1;
 }
 Actor*cook=&g.actors[n];if(cook->kind==GUARD&&cook->faction==HOSTILE&&cook->homeTown==14&&g.townOwner[14]==HOSTILE){for(int v=0;v<g.actorCount;v++)if(S.cannibal[v]==2&&!g.actors[v].dead&&distance(cook->x,cook->y,g.actors[v].x,g.actors[v].y)<24){if(distance(cook->x,cook->y,g.actors[v].x,g.actors[v].y)>2.5f)route_toward(cook,g.actors[v].x,g.actors[v].y,dt*.7f);return 1;}}

 for(int i=0;i<CONTRACTS;i++){Contract*c=&g.contracts[i];if(c->active!=1||(c->type!=5&&c->type!=6)||S.jobActor[i]!=n+1||c->progress)continue;
 Actor*a=&g.actors[n];float x=towns[c->target].x,y=towns[c->target].y;
 if(c->type==CONTRACT_ESCORT && (!S.jobStage[i] || !escort_near(a, 8))) return 1;
 if(c->type==CONTRACT_ESCORT)army_ford_waypoint(a->x,a->y,&x,&y);
 if(c->type==6){static const float dx[]={18,0,-18,0},dy[]={0,18,0,-18};x=towns[c->town].x+dx[S.jobStage[i]%4];y=towns[c->town].y+dy[S.jobStage[i]%4];if(distance(a->x,a->y,x,y)<3)S.jobStage[i]++;}
 if(distance(a->x,a->y,x,y)>2)route_toward(a,x,y,dt*.72f);return 1;
 }return 0;
}
static void tick_jobs(float dt) {
    Actor *p = &g.actors[g.selected];
    for (int i = 0; i < CONTRACTS; i++) {
        Contract *c = &g.contracts[i];
        int escort = c->type == CONTRACT_ESCORT;
        if (c->active != 1 || (!escort && c->type != CONTRACT_PATROL) || c->progress)
            continue;
        int n = S.jobActor[i] - 1;
        if (n < 0 || n >= g.actorCount || g.actors[n].dead || !g.actors[n].active) {
            c->active = 3;
            log_warn("Employer lost.");
            continue;
        }
        Actor *a = &g.actors[n];
        if (escort && !S.jobStage[i]) {
            if (!escort_near(a, 4)) {
                if ((S.jobFar[i] += dt) > 1800) {
                    c->active = 3;
                    log_warn("%s left without an escort.", a->name);
                }
                continue;
            }
            S.jobStage[i] = 1;
            S.jobFar[i] = 0;
            S.jobElapsed[i] = 0;
            S.jobX[i] = a->x;
            S.jobY[i] = a->y;
            log_msg("%s is leaving for %s.", a->name, towns[c->target].name);
        }
        if (!escort && distance(a->x,a->y,p->x,p->y) > 45)
            society_npc_job(n, dt);
        int near = escort ? escort_near(a, 8) : conscious(p) && depth_distance(p, a) <= 8;
        if (!near) {
            S.jobFar[i] += dt;
            if (S.jobFar[i] >= 2 && !S.jobWarned[i]) {
                log_warn(escort ? "%s is waiting." : "%s is out of range.", a->name);
                S.jobWarned[i] = 1;
            }
            if (escort && S.jobFar[i] > 1800) {
                c->active = 3;
                log_warn("Escort abandoned.");
                continue;
            }
        } else {
            S.jobFar[i] = 0;
            S.jobWarned[i] = 0;
        }
        S.jobElapsed[i] += dt;
        float moved = distance(a->x, a->y, S.jobX[i], S.jobY[i]);
        S.jobX[i] = a->x;
        S.jobY[i] = a->y;
        if (near && conscious(a) && moved > .001f) {
            S.jobNear[i] += dt;
            S.jobDistance[i] += moved;
            if (S.jobNear[i] - S.jobPaid[i] >= 30) {
                g.money += 20;
                S.jobPaid[i] += 30;
                log_msg("Wages +20 silver.");
            }
        }
        if ((escort && near && distance(a->x,a->y,towns[c->target].x,towns[c->target].y) < 4) ||
            (!escort && S.jobNear[i] >= 180 && S.jobDistance[i] >= 35)) {
            c->progress = 1;
            log_level("%s complete.", contract_title(c->type));
        }
        if (!c->progress && !escort && S.jobElapsed[i] > 3600) {
            c->active = 3;
            log_warn("Patrol expired.");
        }
    }
}
static int near_guard(const Actor*a,int town,float reach){for(int n=0;n<g.actorCount;n++){Actor*b=&g.actors[n];if(!b->squad&&b->kind==GUARD&&b->faction==g.townOwner[town]&&b->homeTown==town&&conscious(b)&&depth_distance(a,b)<reach)return n;}return -1;}
void town_theft(int town) {
    if (town < 0 || town >= TOWNS || g.townOwner[town] == HOSTILE)
        return;
    TownLife *life = &g.townLife;
    float now = settlement_time();
    if (now >= life->theftUntil[town])
        life->theftCount[town] = 0;
    life->theftUntil[town] = now + DAY_SECONDS * 2;
    if (life->theftCount[town] < 3 && ++life->theftCount[town] == 3) {
        life->searchAfter[town] = now + 10;
        log_warn("%s is checking pockets after repeated thefts.", towns[town].name);
    }
}
void pocket_inspection(int guard, int allow) {
    Actor *a = &g.actors[g.selected];
    if (guard < 0 || guard >= g.actorCount)
        return;
    int f = g.actors[guard].faction, found = 0;
    if (!allow) {
        log_msg("You refused the search. The watch keeps an eye on you.");
        return;
    }
    for (int i = 0; i < PACK; i++)
        if (a->pack[i].item && a->stolen[i] == f) {
            found += a->pack[i].qty;
            inventory_remove_slot(a, i, a->pack[i].qty);
        }
    for (int i = 0; i < GEARS; i++)
        if (a->gear[i] && a->gearStolen[i] == f) {
            found++;
            a->gear[i] = a->gearStolen[i] = 0;
        }
    if (found) {
        crime_charge(a, f, 40, CRIME_THEFT);
        log_warn("%s confiscated %d stolen goods. Bounty +40.", g.actors[guard].name, found);
    } else
        log_msg("%s searched your pockets and let you go.", g.actors[guard].name);
}
static void tick_pocket_inspection(void) {
    Actor *a = &g.actors[g.selected];
    if (ui.screen != SCREEN_WORLD || ui.paused || !conscious(a) || actor_inside(a) ||
        a->attackPhase || a->target >= 0 || a->prisonTime > 0 || a->carrying >= 0 || a->sneaking)
        return;
    int town = town_near(a->x, a->y, 15);
    float now = settlement_time();
    if (town < 0 || g.townLife.theftCount[town] < 3 || now >= g.townLife.theftUntil[town] ||
        now < g.townLife.searchAfter[town])
        return;
    int guard = near_guard(a, town, 2.7f);
    if (guard < 0 || g.actors[guard].target >= 0 || hostile(a, &g.actors[guard]))
        return;
    g.townLife.searchAfter[town] = now + DAY_SECONDS * .45f;
    dialogue_open(guard);
    ui.dialogPage = DIALOG_INSPECTION;
}
static void watch_tick(float dt) {
    ui.watchApproach = 0;
    Actor *a = &g.actors[g.selected];
    if (ui.screen != SCREEN_WORLD || ui.paused || !conscious(a) || playing_dead(a) ||
        a->attackPhase || a->prisonTime > 0 || a->carrying >= 0 || g.depth.actors[g.selected].captive)
        return;
    int t = town_near(a->x, a->y, 20);
    if (t < 0)
        return;
    int f = g.townOwner[t], due = wanted_amount(a, f);
    if (due <= 0 || due >= SERIOUS_BOUNTY)
        return;
    int guard = -1;
    float near = 10;
    for (int i = 0; i < g.actorCount; i++) {
        Actor *b = &g.actors[i];
        if (!conscious(b) || watch_town(b) != t || b->target >= 0 || b->carrying >= 0)
            continue;
        float d = depth_distance(a, b);
        if (d < near && npc_sees(b, a, 10)) {
            near = d;
            guard = i;
        }
    }
    if (guard < 0)
        return;
    if (near < 2.0f) {
        dialogue_open(guard);
        ui.dialogPage = DIALOG_SUMMONS;
        return;
    }
    Actor *b = &g.actors[guard];
    ui.watchApproach = guard + 1;
    if (actor_inside(b))
        depth_move_room(b, depth_x(a) - depth_x(b), depth_y(a) - depth_y(b), dt);
    else
        route_toward(b, a->x, a->y, dt * .9f);
}
static void tick_cannibals(float dt){
 for(int n=0;n<g.actorCount;n++){Actor*a=&g.actors[n];if(a->dead)continue;
 if(S.cannibal[n]==1){int captor=a->carriedBy;if(captor<0||!conscious(&g.actors[captor])){S.cannibal[n]=0;g.depth.actors[n].captive=0;continue;}
 Actor*b=&g.actors[captor];route_toward(b,towns[14].x-10,towns[14].y+6,dt*.7f);a->x=b->x;a->y=b->y;
 if(distance(b->x,b->y,towns[14].x-10,towns[14].y+6)<2){b->carrying=-1;a->carriedBy=-1;S.cannibal[n]=2;S.eating[n]=0;log_warn("%s is bound at the cooking ground. Rescue them!",a->name);}
 }else if(S.cannibal[n]==2){
 DepthActor*z=&g.depth.actors[n];a->target=-1;a->attackPhase=0;a->taskKind=0;

 if (near_guard(a, 14, 5) >= 0) {
     S.eating[n] += dt;
     if (S.eating[n] >= 45) {
         S.eating[n] -= 10;
         a->health[CHEST] -= 12;
         depth_wound(a, CHEST, 12);
         a->blood -= 15;
         a->unconscious = fmaxf(a->unconscious, 12);
         if (a->squad) log_warn("Cannibals are eating %s!", a->name);
         if (fatal_injury(a)) {
             z->captive = 0;
             S.cannibal[n] = 0;
         }
     }
 }
 }else if(a->squad&&a->race!=2&&!actor_inside(a)&&a->unconscious>0&&a->carriedBy<0&&!g.depth.actors[n].captive&&g.townOwner[14]==HOSTILE&&distance(a->x,a->y,towns[14].x,towns[14].y)<24){int c=near_guard(a,14,TALK_REACH);if(c>=0&&g.actors[c].carrying<0){g.actors[c].carrying=n;a->carriedBy=c;S.cannibal[n]=1;g.depth.actors[n].captive=3;a->bleed=0;log_warn("Cannibals seized %s and are carrying them to the fire.",a->name);}}
 }
}
void society_tick(float dt){
 tick_rescues(dt);
 tick_jobs(dt);tick_cannibals(dt);
 watch_tick(dt);
 tick_pocket_inspection();
 for(int n=0;n<g.actorCount;n++)if(g.actors[n].squad){Actor*a=&g.actors[n];S.gateMessage[n]=fmaxf(0,S.gateMessage[n]-dt);for(int f=0;f<FACTIONS;f++)S.recognized[n][f]=fmaxf(0,S.recognized[n][f]-dt);
  int t=town_near(a->x,a->y,18),f=t>=0?g.townOwner[t]:FREE;
  int due = f > FREE ? wanted_amount(a,f) : 0;
  int avoiding_watch = f > FREE && (restricted(t) || g.rep[f] < -25 || due >= SERIOUS_BOUNTY);
  if(avoiding_watch && a->prisonTime <= 0 && conscious(a) && disguise_faction(a)==f && near_guard(a,t,3)>=0){
      int score=disguise_score(a,f);
      S.suspicion[n]+=dt*((100-score)/30.f+(due>=300?8:0));
      if(S.recognized[n][f]<=0&&(S.suspicion[n]>=100||!score)){
          S.suspicion[n]=0;
          crime_charge(a,f,80,CRIME_DISGUISE);
          log_warn("Your %s disguise was exposed.",factionNames[f]);
      }
  }
  else S.suspicion[n]=fmaxf(0,S.suspicion[n]-dt*2);
 }
 S.economyClock+=dt;if(S.economyClock>=60){S.economyClock-=60;
 for(int t=0;t<TOWNS;t++){int it=exports[t]==MEDKIT?CLOTH:exports[t];g.stock[t][it]=(int)bound(g.stock[t][it]+1,0,3 + town_design(t)->marketTier);int food=g.stock[t][RATION]+g.stock[t][GRAIN];if(g.stock[t][RATION]>0)g.stock[t][RATION]--;else if(g.stock[t][GRAIN]>0)g.stock[t][GRAIN]--;if(g.stock[t][WATER]>0)g.stock[t][WATER]--;g.prosperity[t]=(int)bound(g.prosperity[t]+(food<4?-1:1),10,95);
  if(g.day!=S.lastEconomyDay){S.crisis[t]=(g.day+t*3+g.seed)%9<2?1+(g.day+t)%2:0;if(S.crisis[t])g.stock[t][imports[t]]=g.stock[t][imports[t]]/2;}
 }S.lastEconomyDay=g.day;}
}
static NearbyAction actions[ACTION_LIMIT];static int actionCount;
static void add_action(int type,int kind,int target){if(actionCount<ACTION_LIMIT)actions[actionCount++]=(NearbyAction){type,kind,target,depth_room(&g.actors[g.selected])};}
int living_local_count(void){int n=0;Actor*a=&g.actors[g.selected];for(int e=0;e<EVENTS;e++)if(!g.living.events[e].status&&distance(a->x,a->y,g.living.events[e].x,g.living.events[e].y)<8)n++;return n;}
int living_local_slot(int row){Actor*a=&g.actors[g.selected];for(int e=0;e<EVENTS;e++)if(!g.living.events[e].status&&distance(a->x,a->y,g.living.events[e].x,g.living.events[e].y)<8&&row--==0)return e;return -1;}
int nearby_actions(void){
 actionCount=0;Actor*a=&g.actors[g.selected];int room=depth_room(a);
 if (a->carrying >= 0) {
     for (int n = 0; n < g.actorCount; n++)
         if (bounty_guard(n, a->carrying))
             add_action(ACTION_BOUNTY_TURN_IN, 0, n);
     if (lay_carried_in_bed(a, 0)) add_action(ACTION_LAY_IN_BED, 0, a->carrying);
     add_action(ACTION_PUT_DOWN, 0, a->carrying);
     add_action(ACTION_INSPECT_HEALTH, 0, a->carrying);
 }
 if (wound_needs_dressing(a))
     add_action(ACTION_SELF_AID, 0, g.selected);
 if (depth_needs_repair(a))
     add_action(ACTION_SELF_REPAIR, 0, g.selected);
 if (a->bleed > .01f && a->race != 2)
     add_action(ACTION_EMERGENCY, 0, g.selected);
 if(g.depth.actors[g.selected].captive)add_action(ACTION_RESCUE,0,g.selected);
 for(int n=0;n<g.actorCount;n++){Actor*b=&g.actors[n];if(n==g.selected||!b->active||population_hidden(n)||depth_room(b)!=room||depth_distance(a,b)>TALK_REACH||b->carriedBy>=0)continue;
  if(!b->dead&&(g.depth.actors[n].captive||b->prisonTime>0))add_action(ACTION_RESCUE,0,n);
  if (!b->squad && wound_needs_dressing(b)) add_action(ACTION_TREAT_OTHER, 0, n);
  if(S.freed[n]&&!b->squad&&!b->dead){add_action(S.freed[n]==1?ACTION_RESCUED_LEAVE:ACTION_RESCUED_FOLLOW,0,n);add_action(ACTION_RESCUED_RECRUIT,0,n);}
  if(b->dead||b->unconscious>0){add_action(ACTION_CONTEXT,0,n);if(a->carrying<0&&!g.depth.actors[n].captive)add_action(ACTION_CARRY,0,n);if(can_execute(n))add_action(ACTION_EXECUTE,0,n);}
  else if(!b->squad){if(!hostile(a,b)||b->target<0){add_action(ACTION_CONTEXT,0,n);if(a->sneaking)add_action(ACTION_STEAL,0,n);}if(a->autoattack)add_action(ACTION_ATTACK,0,n);}
  else add_action(ACTION_CONTEXT,0,n);
  add_action(ACTION_INSPECT_HEALTH,0,n);
 }
 for (int n = 0; n < g.nodeCount; n++)
     if (g.nodes[n].remaining && node_distance(a, n) <= WORK_REACH) {
         if (g.nodes[n].locked) {
             add_action(ACTION_UNLOCK, 1, n);
             add_action(ACTION_OPEN, 1, n);
         } else {
             add_action(ACTION_CONTEXT, 1, n);
         }
     }
 if(room){
  int context=interior_context();
  if(context!=-2)add_action(ACTION_CONTEXT,5,room-1);
  if(context!=-3&&home_state(room-1)==HOME_OWNED)add_action(ACTION_FURNISH,5,room-1);
  return actionCount;
 }

 for (int b = 0; b < g.buildingCount; b++) {
     Building *v = &g.buildings[b];
     if (portable_bedroll(b) < 0)
         continue;
     if (building_has_interior(v->type) && building_door_distance(b, a->x, a->y) <= DOOR_REACH) {
         add_action(ACTION_CONTEXT, 3, b);
         if (v->type == JAIL && bounty_board(b) >= 0) add_action(ACTION_WARRANTS, 3, b);
     }
     if (distance(a->x, a->y, v->x, v->y) > 1.7f)
         continue;
     if (v->type >= WELL && v->type <= STORE && v->type != WALL && v->type != WINDMILL && v->type != FORGE)
         add_action(ACTION_CONTEXT, 2, b);
     if (portable_bedroll(b) > 0)
         add_action(ACTION_PACK_BEDROLL, 2, b);
     if (v->type == SLAVE_YARD && !slave_in_camp(a))
         add_action(ACTION_PROPERTY, 2, b);
 }
 for(int i=0;i<living_local_count();i++)add_action(ACTION_EVENT,6,living_local_slot(i));
 for(int k=0;k<POP_GROUPS;k++)if(g.population.version&&distance(a->x,a->y,g.population.groups[k].x,g.population.groups[k].y)<55){
  if(distance(a->x,a->y,g.population.groups[k].x,g.population.groups[k].y)<8)add_action(ACTION_POPULATION,0,k);
  int count=population_object_count(k);
  for(int j=0;j<count;j++){float x,y;int type=population_object(k,j,&x,&y);if(distance(a->x,a->y,x,y)>1.7f)continue;
   if(type==1){add_action(ACTION_POPULATION,1,k);if(g.population.groups[k].type==POP_MERCHANTS||g.population.groups[k].type==POP_SMUGGLERS)for(int v=0;v<3;v++)add_action(ACTION_POPULATION,30+v,k);}
   else if(type==2)add_action(ACTION_POPULATION,10+j,k);
   else if(type==3)add_action(ACTION_POPULATION,20,k);
  }
 }
 int town=town_near(a->x,a->y,24);if(town>=0&&g.life.version&&g.life.towns[town].stage)add_action(ACTION_POLITICS,0,town);
 return actionCount;
}
static int action_tab_actor(const NearbyAction *action) {
    if (action->kind != 0 || action->type == ACTION_POPULATION || action->type == ACTION_POLITICS)
        return -1;
    return action->target;
}
void nearby_action_tabs(NearbyTabs *tabs) {
    tabs->tabCount = tabs->activeTab = tabs->rowCount = tabs->activeRow = 0;
    if (!actionCount)
        return;
    int selection = ui.selection >= 0 && ui.selection < actionCount ? ui.selection : 0;
    int selectedActor = action_tab_actor(&actions[selection]);
    for (int row = 0; row < actionCount; row++) {
        int actor = action_tab_actor(&actions[row]), tab = 0;
        while (tab < tabs->tabCount && tabs->actors[tab] != actor)
            tab++;
        if (tab == tabs->tabCount) {
            tabs->actors[tab] = actor;
            tabs->first[tab] = row;
            tabs->tabCount++;
        }
        if (actor == selectedActor) {
            tabs->activeTab = tab;
            if (row == selection)
                tabs->activeRow = tabs->rowCount;
            tabs->rows[tabs->rowCount++] = row;
        }
    }
}
const NearbyAction *nearby_action(int row){return row>=0&&row<actionCount?&actions[row]:NULL;}
void nearby_action_label(int row,char*out,int size){const NearbyAction*x=nearby_action(row);if(!x){snprintf(out,size,"Nothing within reach");return;}
 if (x->type == ACTION_INSPECT_HEALTH || x->type == ACTION_EXECUTE) {
     snprintf(out, size, "%s", x->type == ACTION_INSPECT_HEALTH ? "Check health" : "Execute");
     return;
 }
 if (x->type == ACTION_BOUNTY_TURN_IN) {
     snprintf(out, size, "Turn in %s to the jailkeeper", g.actors[g.selected].carrying >= 0 ?
              g.actors[g.actors[g.selected].carrying].name : "bounty");
     return;
 }
 if (x->type == ACTION_WARRANTS || x->type == ACTION_FURNISH) {
     snprintf(out,size,"%s",x->type == ACTION_WARRANTS ? "Read wanted posters" : "Furnish house");
     return;
 }
 if (x->type == ACTION_CONTEXT && x->kind == 3 && home_state(x->target) == HOME_FOR_SALE) {
     snprintf(out,size,"View house deed (%d silver)",home_price(x->target));
     return;
 }
 if (x->type == ACTION_LAY_IN_BED || x->type == ACTION_PUT_DOWN) {
     snprintf(out, size, x->type == ACTION_PUT_DOWN ? "Put down %s" : x->room ? "Lay %s in bed" :
              "Lay %s on bedroll", g.actors[x->target].name);
     return;
 }
 if (x->type == ACTION_SELF_AID || x->type == ACTION_SELF_REPAIR) {
     snprintf(out, size, "%s", x->type == ACTION_SELF_AID
         ? "Bandage yourself" : "Repair yourself (1 spare part)");
     return;
 }
 if (x->type == ACTION_EMERGENCY) {
     snprintf(out, size, "%s", inventory_count(&g.actors[g.selected], CLOTH) < 2 &&
              g.actors[g.selected].gear[1] == RAGS ? "Tear worn rags to stop bleeding" : "Stop bleeding (2 cloth)");
     return;
 }
 if (x->type == ACTION_PACK_BEDROLL || x->type == ACTION_PROPERTY) {
     snprintf(out, size, "%s", x->type == ACTION_PACK_BEDROLL ? "Pack up bedroll" :
         "Open confiscated belongings");
     return;
 }
 if (x->type == ACTION_TREAT_OTHER) {
     snprintf(out, size, "Bandage %s", g.actors[x->target].name);
     return;
 }
 if(x->type==ACTION_POLITICS){snprintf(out,size,"Council at %s",towns[x->target].name);return;}
 if(x->type==ACTION_POPULATION){population_action_label(x->target,x->kind,out,size);return;}
 if(x->type==ACTION_EVENT){char name[70];living_title(x->target,name,sizeof(name));snprintf(out,size,"Investigate %s",name);return;}
 if(x->type>=ACTION_RESCUED_FOLLOW){snprintf(out,size,"%s %s",x->type==ACTION_RESCUED_FOLLOW?"Ask to follow:":x->type==ACTION_RESCUED_LEAVE?"Leave me alone:":"Recruit:",g.actors[x->target].name);return;}
 if(x->kind==5){snprintf(out,size,"%s",interior_prompt());return;}
 if(x->type==ACTION_ATTACK||x->type==ACTION_CARRY||x->type==ACTION_RESCUE||x->type==ACTION_STEAL){snprintf(out,size,"%s %s",x->type==ACTION_ATTACK?"Attack":x->type==ACTION_CARRY?"Carry":x->type==ACTION_RESCUE?(g.actors[x->target].prisonTime>0?"Free prisoner:":"Free from shackles:"):"Pickpocket",g.actors[x->target].name);return;}
 int kind=ui.contextKind,index=ui.contextIndex;ui.contextKind=x->kind;ui.contextIndex=x->target;
 const char *verb = context_action();
 if (x->type == ACTION_OPEN)
     verb = g.nodes[x->target].owner ? "SEARCH OWNED" : "SEARCH";
 if (x->type == ACTION_UNLOCK)
     snprintf(out, size, "Pick lock (%d%%) / %s", lockpick_chance(&g.actors[g.selected], LOCK_SALVAGE), context_name());
 else if (x->kind == 2 && g.buildings[x->target].type == BENCH && depth_build_ready(x->target))
     snprintf(out, size, "Use research bench");
 else
     snprintf(out, size, "%s %s", verb, context_name());
 ui.contextKind=kind;ui.contextIndex=index;
}
static int action_valid(const NearbyAction*x){Actor*a=&g.actors[g.selected];if(!conscious(a)||depth_room(a)!=x->room)return 0;int n=x->target;
 if (x->type == ACTION_SELF_AID || x->type == ACTION_SELF_REPAIR || x->type == ACTION_EMERGENCY || x->type == ACTION_PLAY_DEAD)
     return n == g.selected;
 if (x->type == ACTION_BOUNTY_TURN_IN)
     return bounty_guard(n, a->carrying);
 if (x->type == ACTION_WARRANTS)
     return !x->room && bounty_board(n) >= 0 && building_door_distance(n,a->x,a->y) <= DOOR_REACH;
 if (x->type == ACTION_FURNISH)
     return x->room == n + 1 && home_state(n) == HOME_OWNED;
 if (x->type == ACTION_PUT_DOWN || x->type == ACTION_LAY_IN_BED)
     return n >= 0 && a->carrying == n && (x->type == ACTION_PUT_DOWN || lay_carried_in_bed(a, 0));
 if (x->type == ACTION_INSPECT_HEALTH && n >= 0 && n < g.actorCount && a->carrying == n)
     return g.actors[n].active && g.actors[n].carriedBy == g.selected;
 if(x->type==ACTION_POLITICS)return n>=0&&n<TOWNS&&distance(a->x,a->y,towns[n].x,towns[n].y)<=24;
 if(x->type==ACTION_POPULATION)return population_action_valid(n,x->kind);
 if(x->type==ACTION_EVENT)return n>=0&&n<EVENTS&&!g.living.events[n].status&&distance(a->x,a->y,g.living.events[n].x,g.living.events[n].y)<=8;
 if(x->kind==0)return n>=0&&n<g.actorCount&&g.actors[n].active&&depth_room(&g.actors[n])==x->room&&depth_distance(a,&g.actors[n])<=TALK_REACH;
 if(x->kind==1)return n>=0&&n<g.nodeCount&&g.nodes[n].remaining&&distance(a->x,a->y,g.nodes[n].x,g.nodes[n].y)<=WORK_REACH;
 if(x->kind==4)return 0;
 if(x->kind==5)return g.interiorBuilding==n+1;
  if (n < 0 || n >= g.buildingCount) return 0;
 if (x->kind == 3) return building_door_distance(n, a->x, a->y) <= DOOR_REACH;
 return distance(a->x, a->y, g.buildings[n].x, g.buildings[n].y) <= 1.7f;
}
static const char *action_reason(const NearbyAction *x, int queueing) {
    Actor *a = &g.actors[g.selected];
    int n = x->target;
    if (!conscious(a))
        return "Select a conscious member";
    if (!action_valid(x))
        return "Out of reach";
    if (x->type == ACTION_PLAY_DEAD)
        return "Action unavailable";
    if (x->type == ACTION_ATTACK && !a->autoattack)
        return "Switch to Fight stance";
    if (x->type == ACTION_EXECUTE)
        return can_execute(n) ? NULL : "Target must be down";
    if (x->type == ACTION_PACK_BEDROLL)
        return portable_bedroll(n) > 0 ? NULL : "Already packed";
    if (x->type == ACTION_PROPERTY)
        return slave_camp_at(n) >= 0 && !slave_in_camp(a) ? NULL : "Restrained";
    if (x->type == ACTION_TREAT_OTHER)
        return !wound_needs_dressing(&g.actors[n]) ? "No wound needs dressing" :
               !inventory_count(a, MEDKIT) ? "Need a field dressing" : NULL;
    if (x->type == ACTION_SELF_AID)
        return !wound_needs_dressing(a) ? "No wound needs dressing" :
               !inventory_count(a, MEDKIT) ? "Need a field dressing" : NULL;
    if (x->type == ACTION_SELF_REPAIR)
        return !depth_needs_repair(a) ? "No mechanical damage" :
               !inventory_count(a, PARTS) ? "Need a spare part" : NULL;
    if (x->type == ACTION_EMERGENCY)
        return a->bleed <= .01f ? "Bleeding already stopped" :
               inventory_count(a, CLOTH) < 2 && a->gear[1] != RAGS ? "Need 2 cloth or worn rags" : NULL;
    if (x->type == ACTION_POPULATION)
        return population_action_reason(n, x->kind);
    if (x->kind == 1) {
        Node *node = &g.nodes[n];
        if (!node->remaining)
            return "Nothing left";
        if (x->type == ACTION_OPEN && node->locked) {
            if (queueing)
                for (int q = 0; q < S.queueCount[g.selected]; q++)
                    if (S.queue[g.selected][q].type == ACTION_UNLOCK && S.queue[g.selected][q].target == n)
                        return NULL;
            return "Pick the lock first";
        }
        if (x->type == ACTION_UNLOCK && !node->locked)
            return "Already unlocked";
    }
    if (x->kind == 2 && depth_build_ready(n)) {
        Building *b = &g.buildings[n];
        if (b->type == FARM && b->progress < 100 && !inventory_count(a, WATER))
            return "Need water";
        if (b->type == BED && b->owner < 0 && !trail_bed(n)) {
            int town = town_near(a->x, a->y, 12);
            if (town < 0 || g.money < regional_bed_cost(town))
                return "Cannot pay for the bed";
        }
    }
    if (x->kind == 5 && g.buildings[n].owner < 0 && g.money < 25) {
        InteriorProp props[ROOM_PROPS];
        interior_floor_props(n, actor_floor(a), props);
        int c = interior_context();
        if (c >= 0 && props[c].action == ROOM_REST)
            return "Need 25 silver";
    }
    return NULL;
}
const char *nearby_action_reason(int row) {
    const NearbyAction *x = nearby_action(row);
    return x ? action_reason(x, ui.paused) : "Out of reach";
}
static void perform_action(const NearbyAction *x) {
 Actor *a = &g.actors[g.selected];
 int n = x->target;
 const char *reason = action_reason(x, 0);
 if (reason) { log_msg("%s.", reason); return; }
 if (x->type != ACTION_SELF_AID && x->type != ACTION_SELF_REPAIR && x->type != ACTION_EMERGENCY)
     g.field.playingDead[g.selected] = 0;
 if (x->type == ACTION_BOUNTY_TURN_IN) { bounty_turn_in(n); return; }
 if (x->type == ACTION_WARRANTS) {
     ui.warrantTown = bounty_board(n);
     ui.warrantClosed = 0;
     depth_open(SCREEN_WARRANTS);
     return;
 }
 if (x->type == ACTION_FURNISH) { home_open(n); return; }
 if (x->type == ACTION_LAY_IN_BED) { lay_carried_in_bed(a, 1); return; }
 if (x->type == ACTION_PUT_DOWN) { put_down(a); return; }
 if (x->type == ACTION_SELF_AID) { heal_actor(n, n); return; }
 if (x->type == ACTION_TREAT_OTHER) { heal_actor(g.selected, n); return; }
 if (x->type == ACTION_PACK_BEDROLL) { pack_bedroll(n); return; }
 if (x->type == ACTION_PROPERTY) {
     ui.chestKind = CHEST_PROPERTY;
     ui.storageIndex = slave_camp_at(n);
     depth_open(SCREEN_CHEST);
     return;
 }
 if (x->type == ACTION_SELF_REPAIR) { depth_repair(n, n); return; }
 if (x->type == ACTION_EMERGENCY) { begin_treatment(n, n, TREAT_PRESSURE, -1); return; }
 if(x->type==ACTION_POLITICS){politics_open(n);return;}
 if(x->type==ACTION_POPULATION){population_action(n,x->kind);return;}
 if(x->type==ACTION_EVENT){WorldEvent*e=&g.living.events[n];if(e->type==2||e->type==3||e->type==4)living_action(n);else{char b[160];living_detail(n,b,sizeof(b));log_msg("%s",b);}return;}
 if(x->type==ACTION_ATTACK){command_attack(n);return;}
 if(x->type==ACTION_RESCUE){society_rescue_start(n);return;}
 if(x->type>=ACTION_RESCUED_FOLLOW){society_rescue_order(n,x->type);return;}
 if(x->type==ACTION_CARRY){Actor*b=&g.actors[n];if(a->carrying<0&&b->carriedBy<0&&!g.depth.actors[n].captive&&(b->dead||b->unconscious>0)){a->carrying=n;b->carriedBy=g.selected;b->x=a->x;b->y=a->y;log_msg("Carrying %s.",b->name);}return;}
 if(x->type==ACTION_STEAL){if(a->sneaking&&conscious(&g.actors[n])){ui.lootActor=n;ui.selection=0;ui.screen=SCREEN_THEFT;}return;}
 if(x->kind==5){interior_interact();return;}
 interact_context(x->kind,n);
}
void action_menu_open(void){nearby_actions();if(!actionCount){log_msg("No actions within reach.");return;}ui.screen=SCREEN_ACTIONS;ui.selection=0;ui.menuReturn=0;}
void nearby_action_choose(int row){const NearbyAction*p=nearby_action(row);if(!p)return;
 const char *reason = nearby_action_reason(row);
 if (reason) { log_msg("%s.", reason); return; }
 if (p->type == ACTION_INSPECT_HEALTH || p->type == ACTION_EXECUTE) {
     ui.nearbyTarget = p->target;
     ui.screen = p->type == ACTION_INSPECT_HEALTH ? SCREEN_INSPECT_HEALTH : SCREEN_EXECUTE;
     return;
 }
 NearbyAction a=*p;int n=g.selected;ui.screen=ui.paused?SCREEN_ACTIONS:SCREEN_WORLD;
 if(ui.paused){int q=S.queueCount[n];if(q>=ACTION_QUEUE){log_msg("Four actions queued. Clear or resume first.");return;}S.queue[n][q]=a;S.queueCount[n]++;log_msg("Action queued. R resumes; movement cancels queued actions.");}
 else perform_action(&a);
}
void society_queue_tick(void){int n=g.selected;Actor*a=&g.actors[n];if(ui.paused||ui.screen!=SCREEN_WORLD||!S.queueCount[n]||a->taskKind||a->attackPhase||society_rescue_busy(n))return;NearbyAction x=S.queue[n][0];for(int i=1;i<S.queueCount[n];i++)S.queue[n][i-1]=S.queue[n][i];S.queueCount[n]--;perform_action(&x);}
int society_validate(const Game*v){const Society*s=&v->society;if(!s->version)return 1;if(s->version!=1||!isfinite(s->economyClock)||s->economyClock<0||s->prisonSeeded<0||s->prisonSeeded>1)return 0;
 for(int t=0;t<TOWNS;t++)if(s->crisis[t]<0||s->crisis[t]>2)return 0;
 for(int n=0;n<MAX_ACTORS;n++){if(s->rescueHelper[n]<0||s->rescueHelper[n]>v->actorCount||s->freed[n]<0||s->freed[n]>3||s->captorFaction[n]<0||s->captorFaction[n]>=FACTIONS)return 0;for(int c=0;c<WEAPON_XP_SLOTS;c++)if(!isfinite(s->weaponXP[n][c])||s->weaponXP[n][c]<0||s->weaponXP[n][c]>SKILL_XP_CAP)return 0;if(s->queueCount[n]<0||s->queueCount[n]>ACTION_QUEUE||s->cannibal[n]<0||s->cannibal[n]>2||!isfinite(s->eating[n])||!isfinite(s->suspicion[n])||!isfinite(s->gateMessage[n]))return 0;
  for(int f=0;f<FACTIONS;f++)if(s->wanted[n][f]<0||s->wanted[n][f]>100000||!isfinite(s->recognized[n][f]))return 0;
  for(int q=0;q<s->queueCount[n];q++){const NearbyAction*a=&s->queue[n][q];if(a->type<0||a->type>ACTION_FURNISH||a->kind<0||a->kind>32||a->room<0||a->room>v->buildingCount||a->target<0||a->target>=MAX_NODES)return 0;
   if(a->type==ACTION_WARRANTS||a->type==ACTION_FURNISH){if(a->target>=v->buildingCount||a->kind!=(a->type==ACTION_WARRANTS?3:5))return 0;continue;}
   if(a->type==ACTION_BOUNTY_TURN_IN||a->type==ACTION_PUT_DOWN||a->type==ACTION_LAY_IN_BED){if(a->kind||a->target>=v->actorCount)return 0;continue;}
   if(a->type==ACTION_PACK_BEDROLL||a->type==ACTION_PROPERTY){if(a->kind!=2||a->target>=v->buildingCount)return 0;continue;}
   if(a->type==ACTION_TREAT_OTHER){if(a->kind!=0||a->target>=v->actorCount)return 0;continue;}
   if(a->type>=ACTION_EMERGENCY){if(a->target!=n||a->target>=v->actorCount||a->kind!=0)return 0;continue;}
   if(a->type==ACTION_POLITICS){if(a->target>=TOWNS||a->kind!=0)return 0;continue;}
   if(a->type==ACTION_POPULATION){if(a->target>=POP_GROUPS||!(a->kind==0||a->kind==1||(a->kind>=11&&a->kind<=13)||a->kind==20||(a->kind>=30&&a->kind<=32)))return 0;continue;}
   if(a->kind>6)return 0;
   if((a->type==ACTION_OPEN||a->type==ACTION_UNLOCK)&&a->kind!=1)return 0;
   if((a->type==ACTION_ATTACK||a->type==ACTION_CARRY||a->type==ACTION_STEAL||a->type==ACTION_RESCUE||a->type>=ACTION_RESCUED_FOLLOW)&&a->kind!=0)return 0;
   if(a->type==ACTION_EVENT&&a->kind!=6)return 0;}
 }
 for(int j=0;j<CONTRACTS;j++)if(s->jobStage[j]<0||s->jobStage[j]>10000000||s->jobActor[j]<0||s->jobActor[j]>v->actorCount||s->jobBond[j]<0||s->jobWarned[j]<0||s->jobWarned[j]>1||!isfinite(s->jobFar[j])||!isfinite(s->jobWarning[j])||!isfinite(s->jobElapsed[j])||!isfinite(s->jobNear[j])||!isfinite(s->jobPaid[j])||!isfinite(s->jobDistance[j])||!isfinite(s->jobX[j])||!isfinite(s->jobY[j]))return 0;
 return 1;
}
#undef S
