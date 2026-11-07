/* Persistent regional stories; included by world.c. Save v7 appends this state. */
const char *regional_names[3]={"SALTWATCH: THE DRY WELL","BLACK MESA: ROAD TOLL","PALE PASS: WINTER REFUGE"};
int regional_town(int r) {static const int t[3]={2,8,12};return r>=0&&r<3?t[r]:0;}
int regional_at(float x,float y) {
    for(int r=0;r<3;r++) if(distance(x,y,towns[regional_town(r)].x,towns[regional_town(r)].y)<14) return r;
    return -1;
}
int regional_named(int n) {
    if(!g.regions.version || n<0) return 0;
    for(int i=0;i<3;i++) if(g.regions.toll[i]==n) return 1;
    return 0;
}
static void regional_clear(float *x,float *y) {
    float ox=*x,oy=*y;
    for(int r=0;r<10;r++) for(int j=0;j<16;j++) {
        float a=j*.392699f,xx=ox+cosf(a)*r,yy=oy+sinf(a)*r;
        if(can_walk(xx,yy)){*x=xx;*y=yy;return;}
    }
}
void regional_init(void) {
    if(g.regions.version) return;
    memset(&g.regions,0,sizeof(g.regions));g.regions.version=1;
    int near=0;float best=1e9f;
    for(int i=0;i<8;i++){float d=distance(towns[8].x,towns[8].y,towns[i].x,towns[i].y);if(d<best){best=d;near=i;}}
    g.regions.campX=towns[8].x+(towns[near].x-towns[8].x)*32/fmaxf(1,best);
    g.regions.campY=towns[8].y+(towns[near].y-towns[8].y)*32/fmaxf(1,best);
    regional_clear(&g.regions.campX,&g.regions.campY);
    for(int i=0;i<3;i++) {
        g.regions.toll[i]=-1;
        if(g.actorCount>=MAX_ACTORS) continue;
        float x=g.regions.campX+i*1.6f,y=g.regions.campY+i*.6f;
        regional_clear(&x,&y);
        int n=add_actor(i==0?"MESA TOLL BOSS":"MESA COLLECTOR",GUARD,OUTLAWS,x,y,12+i*3);
        if(n<0) continue;
        g.regions.toll[i]=n;
        Actor *a=&g.actors[n];a->gear[0]=gear_id(5+i,0,1);a->gear[1]=gear_id(34,0,1);
        a->gear[2]=gear_id(56,0,0);a->voice=ROLE_MARSHAL;a->homeTown=8;
        inventory_add(a,RATION,2);inventory_add(a,MEDKIT,1);
        g.depth.actors[n].upkeep=100;
    }
    for(int i=0;i<3;i++) regional_restock(regional_town(i));
}
int regional_validate(const Game *s) {
    const RegionalState *r=&s->regions;
    if(!r->version) return 1;
    if(r->version!=1 || !isfinite(r->campX) || !isfinite(r->campY) || r->campX<0 || r->campY<0 || r->campX>WORLD || r->campY>WORLD) return 0;
    for(int i=0;i<3;i++) if(r->outcome[i]<0 || r->outcome[i]>2 || r->toll[i]<-1 || r->toll[i]>=s->actorCount) return 0;
    return 1;
}
int regional_hostile(const Actor *a,const Actor *b) {
    if(g.regions.outcome[1]) return 0;
    if((uintptr_t)a<(uintptr_t)g.actors || (uintptr_t)a>=(uintptr_t)(g.actors+g.actorCount) ||
       (uintptr_t)b<(uintptr_t)g.actors || (uintptr_t)b>=(uintptr_t)(g.actors+g.actorCount)) return 0;
    int ai=(int)(a-g.actors),bi=(int)(b-g.actors);
    return (regional_named(ai) && (b->squad || merc_slot(b)>=0)) ||
           (regional_named(bi) && (a->squad || merc_slot(a)>=0));
}
int regional_toll_left(void) {
    int alive=0;
    for(int i=0;i<3;i++) {
        int n=g.regions.toll[i];
        if(n>=0 && g.actors[n].active && !g.actors[n].dead && g.actors[n].unconscious<=0) alive++;
    }
    return alive;
}
float regional_price(int t,int it) {
    int supplies=it==WATER||it==RATION||it==MEDKIT;
    if(t==2 && (it==WATER||it==RATION)) return !g.regions.outcome[0]?.25f:g.regions.outcome[0]==1?-.15f:.2f;
    if(t==3 && it==WATER && g.regions.outcome[0]==2) return .2f;
    if((t==8||t==10) && g.regions.outcome[1]) return g.regions.outcome[1]==1?-.12f:t==8?-.1f:.2f;
    if(t==12 && supplies) return !g.regions.outcome[2]?.25f:g.regions.outcome[2]==1?-.15f:.1f;
    return 0;
}
void regional_restock(int t) {
    if(!g.regions.version) return;
    if(t==2) {
        int o=g.regions.outcome[0];g.stock[t][WATER]=!o?2:o==1?32:20;g.stock[t][RATION]=!o?6:o==1?24:16;
    }
    if(t==3 && g.regions.outcome[0]==2) g.stock[t][WATER]=6;
    if(t==8||t==10) {
        int o=g.regions.outcome[1];
        if(o==1){g.stock[t][RATION]=28;g.stock[t][MEDKIT]=20;}
        if(o==2){g.stock[t][RATION]=t==8?24:6;g.stock[t][MEDKIT]=t==8?20:4;}
    }
    if(t==12) {
        int o=g.regions.outcome[2];g.stock[t][RATION]=!o?3:o==1?30:16;g.stock[t][MEDKIT]=!o?2:o==1?20:8;
        if(o==2){g.stock[t][CARTRIDGE]=100;g.stock[t][gear_id(2,4,2)]=3;g.stock[t][gear_id(34,4,2)]=2;}
    }
}
int regional_bed_cost(int t) {return t==12&&g.regions.outcome[2]==1?0:25;}
int regional_well(Actor *a,Building *b) {
    if(regional_at(b->x,b->y)!=0) return 0;
    int o=g.regions.outcome[0];
    if(!o){log_msg("Saltwatch's pump is broken. Journal > Conflicts has the dispute.");return 1;}
    if(o==2) {
        if(g.money<5){log_msg("Union water rights: 5 silver for two water.");return 1;}
        if(inventory_add(a,WATER,2)){g.money-=5;log_msg("Two water: 5 silver to the Union concession.");}return 1;
    }
    return 0;
}
const char *regional_story(int r) {
    if(r==0) return "Saltwatch's pump failed. Repair a public well, or grant the Union an exclusive water concession.";
    if(r==1) return "Three collectors block Black Mesa's northern road. Break their camp, or finance the toll syndicate.";
    return "Pale Pass cannot feed its refugees through winter. Supply a sanctuary, or accept Shogunate rule and a garrison.";
}
const char *regional_choice(int r,int c) {
    static const char *names[3][2]={{"Repair the public pump","Grant Union water rights"},{"Dismantle the toll camp","Back the toll syndicate"},{"Supply the sanctuary","Accept a Shogun garrison"}};
    return names[r][c];
}
void regional_requirements(int r,int c,char *out,int size) {
    if(r==0) snprintf(out,size,c?"COST: 200 silver, 2 machine parts":"COST: 6 iron bars, 4 machine parts");
    else if(r==1) {
        if(c) snprintf(out,size,"COST: 500 silver; collectors grant you passage");
        else snprintf(out,size,"DEFEAT COLLECTORS: %d remain; then return here",regional_toll_left());
    } else snprintf(out,size,c?"COST: 6 iron bars, 4 rations":"COST: 6 rations, 4 dressings, 6 timber");
}
const char *regional_result(int r,int c) {
    static const char *s[3][2]={
        {"Free well; more, cheaper food/water. Scavengers +12; Union -8.","Water costs 5 silver; water/food prices rise. Union +20; Scavengers -15."},
        {"Toll camp gone; Mesa/Port supplies cheaper. Union +15; Outlaws -30; +250 silver.","Your crew passes; Port supplies shrink and cost more. Outlaws +18; Union -20."},
        {"Free beds; more, cheaper food/medicine. Monks +20; Shogun -12.","Shogun rules; guards and arsenal improve. Shogun +20; Monks -20; supplies cost more."}
    };return s[r][c];
}
static void regional_rule(int r,int faction) {
    int t=regional_town(r);g.townOwner[t]=faction;
    for(int i=0;i<g.buildingCount;i++) {Building*b=&g.buildings[i];if(b->owner<0&&distance(b->x,b->y,towns[t].x,towns[t].y)<14)b->faction=faction;}
    for(int i=0;i<g.actorCount;i++) {
        Actor *a=&g.actors[i];if(a->squad||a->dead||a->kind==BEAST||a->kind==RAIDER||regional_named(i)||merc_slot(a)>=0)continue;
        if(distance(a->homeX,a->homeY,towns[t].x,towns[t].y)<14) {
            a->faction=faction;
            if(r==2&&g.regions.outcome[2]==2&&a->kind==GUARD) {
                a->gear[0]=gear_id(2,4,2);a->gear[1]=gear_id(34,4,2);
                if(level(a,MELEE)<26)a->xp[MELEE]=25*25*12;
                if(level(a,DEFENCE)<24)a->xp[DEFENCE]=23*23*12;
            }
        }
    }
}
int regional_resolve(int r,int c) {
    if(r<0||r>=3||c<0||c>1||g.regions.outcome[r]) return 0;
    Actor*a=&g.actors[g.selected];
    if(!conscious(a) || regional_at(a->x,a->y)!=r) {log_msg("Reach %s to settle this dispute.",towns[regional_town(r)].name);return 0;}
    int items[3]={0},qty[3]={0},silver=0;
    if(r==0){items[0]=PARTS;qty[0]=c?2:4;if(c)silver=200;else{items[1]=IRONBAR;qty[1]=6;}}
    if(r==1){if(c)silver=500;else if(regional_toll_left()){log_msg("The northern toll camp still has %d standing collectors.",regional_toll_left());return 0;}}
    if(r==2){items[0]=RATION;qty[0]=c?4:6;items[1]=c?IRONBAR:MEDKIT;qty[1]=c?6:4;if(!c){items[2]=WOOD;qty[2]=6;}}
    if(g.money<silver){log_msg("Need %d silver to commit this agreement.",silver);return 0;}
    for(int i=0;i<3;i++) if(items[i] && inventory_count(a,items[i])<qty[i]) {log_msg("Bring %d %s in the selected survivor's pack.",qty[i],itemDefs[items[i]].name);return 0;}
    for(int i=0;i<3;i++) if(items[i]) inventory_remove(a,items[i],qty[i]);
    g.money-=silver;g.regions.outcome[r]=c+1;
    if(r==0){g.rep[UNION]+=c?20:-8;g.rep[SCAVENGERS]+=c?-15:12;regional_rule(r,c?UNION:SCAVENGERS);}
    if(r==1){g.rep[OUTLAWS]+=c?18:-30;g.rep[UNION]+=c?-20:15;
        if(!c){g.money+=250;for(int i=0;i<3;i++){int n=g.regions.toll[i];if(n>=0&&!g.actors[n].dead){Actor *b=&g.actors[n];if(b->carriedBy>=0)g.actors[b->carriedBy].carrying=-1;b->carriedBy=-1;b->active=0;b->target=-1;}}}
        else for(int i=0;i<3;i++){int n=g.regions.toll[i];if(n>=0)g.actors[n].target=-1;}
    }
    if(r==2){g.rep[MONKS]+=c?-20:20;g.rep[SHOGUN]+=c?20:-12;regional_rule(r,c?SHOGUN:MONKS);}
    /* Consequences apply now and on every daily restock, never on each load. */
    for(int t=0;t<TOWNS;t++)regional_restock(t);
    snprintf(g.worldNews,sizeof(g.worldNews),"%s: %s.",towns[regional_town(r)].name,regional_choice(r,c));
    log_msg("Agreement permanent: %s.",regional_choice(r,c));return 1;
}
void regional_tick(void) {
    if(g.regions.outcome[0]) g.townOwner[2]=g.regions.outcome[0]==1?SCAVENGERS:UNION;
    if(g.regions.outcome[2]) g.townOwner[12]=g.regions.outcome[2]==1?MONKS:SHOGUN;
}
