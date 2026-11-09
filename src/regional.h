/* Regional economy and older save state, included by world.c. */
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
int regional_bed_cost(int t) {return (t==12&&g.regions.outcome[2]==1)||(t==OLD_TOWNS+2&&g.townOwner[t]==MONKS)?0:25;}
int regional_well(Actor *a,Building *b) {
    if(regional_at(b->x,b->y)!=0) return 0;
    int o=g.regions.outcome[0];
    if (!o) return 0;
    if(o==2) {
        if(g.money<5){log_msg("Union water rights: 5 silver for two water.");return 1;}
        if(inventory_add(a,WATER,2)){g.money-=5;log_msg("Two water: 5 silver to the Union concession.");}return 1;
    }
    return 0;
}
