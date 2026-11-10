/* Included by interior.c: home placement shares the authored furniture footprints. */
void placement_init(void) {
    if (g.plans.version) return;
    g.plans.version = 1;
    for(int i=0;i<CONTRACTS;i++)if(g.contracts[i].active==1&&g.contracts[i].type==CONTRACT_DISPATCH&&g.contracts[i].progress<g.contracts[i].needed)
        g.plans.dispatchPending[i]=1;
    for (int b = 0; b < g.buildingCount; b++) {
        for (int i = 0; i < HOME_BAYS; i++) {
            g.plans.furniture[b][i] = g.homes.furniture[b][i];
            g.plans.x[b][i] = homeBays[i][0];
            g.plans.y[b][i] = homeBays[i][1];
        }
        g.plans.campLastNear[b] = g.time;
    }
    depot_init();
}
static InteriorProp home_prop(int type, float x, float y) {
    const HomeFurniture *f = &homeFurniture[type];
    float w = f->w, d = f->d;
    if (f->action == ROOM_REST) { w *= .86f; d *= .86f; }
    int modelIndex=furniture_art_index(f->art);
    if(modelIndex<0||modelIndex>=FURNITURE_ART_COUNT)return (InteriorProp){x,y,0,0,0,ROOM_NONE};
    const FurnitureModel *model = &furnitureModels[modelIndex];
    float span = fminf(w + d, model->span / (2 * ROOM_MAX_SCALE));
    float share = furniture_is_bed(f->art) && d > w ? 1 - model->xShare : model->xShare;
    return (InteriorProp){x, y, span * share, span * (1-share), f->art, f->action};
}
int home_layout(int b, InteriorProp *props) {
    placement_init();
    int count = 0;
    for (int i = 0; i < HOME_PLACES; i++) {
        int type = g.plans.furniture[b][i];
        if (type) props[count++] = home_prop(type, g.plans.x[b][i], g.plans.y[b][i]);
    }
    return count;
}
static int home_site_valid(int type, int b, int skip, float x, float y) {
    if (type <= FURN_EMPTY || type >= FURN_TYPES || home_state(b) != HOME_OWNED ||
        depth_room(&g.actors[g.selected]) != b+1 || !isfinite(x) || !isfinite(y)) return 0;
    InteriorProp p = home_prop(type, x, y);
    if (x-p.w < .36f || x+p.w > 6.64f || y-p.d < .36f || y+p.d > 6.64f ||
        (fabsf(x-3.5f) < p.w+.60f && y+p.d > 5.92f)) return 0;
    for (int i = 0; i < HOME_PLACES; i++) {
        int t = g.plans.furniture[b][i];
        if (!t || i == skip) continue;
        InteriorProp other = home_prop(t, g.plans.x[b][i], g.plans.y[b][i]);
        if (fabsf(x-other.x) < p.w+other.w+.22f && fabsf(y-other.y) < p.d+other.d+.22f) return 0;
    }
    for (int i = 0; i < g.actorCount; i++) {
        const Actor *a = &g.actors[i];
        if (a->active && a->carriedBy < 0 && depth_room(a) == b+1 &&
            fabsf(x-depth_x(a)) < p.w+.32f && fabsf(y-depth_y(a)) < p.d+.32f) return 0;
    }
    return 1;
}
int placement_valid(int type, int b, float x, float y) {
    placement_init();
    if (b >= 0) return home_site_valid(type, b, ui.placeSlot, x, y);
    Actor *a = &g.actors[g.selected];
    if ((type != 0 && type != 1) || !conscious(a) || actor_inside(a) || a->prisonTime > 0 ||
        g.depth.actors[g.selected].captive || !isfinite(x) || !isfinite(y) ||
        distance(a->x,a->y,x,y) > 10 || town_near(x,y,10) >= 0 || !can_walk(x,y) ||
        !can_walk(x+.65f,y) || !can_walk(x-.65f,y) || !can_walk(x,y+.65f) || !can_walk(x,y-.65f)) return 0;
    for (int i = 0; i < g.buildingCount; i++) {
        Building *other = &g.buildings[i];
        if (portable_bedroll(i) >= 0 && distance(x,y,other->x,other->y) < 1.75f) return 0;
    }
    for(int i=0;i<g.nodeCount;i++)if(g.nodes[i].remaining && !g.drops[i].room && distance(x,y,g.nodes[i].x,g.nodes[i].y)<1.25f)return 0;
    for (int i = 0; i < g.actorCount; i++) {
        const Actor *other = &g.actors[i];
        if (other->active && !actor_inside(other) && other->carriedBy < 0 &&
            distance(x,y,other->x,other->y) < .85f) return 0;
    }
    return available_building_slot() >= 0;
}
int home_place(int b, int slot, int type, float x, float y) {
    placement_init();
    if (b < 0 || b >= g.buildingCount || home_state(b) != HOME_OWNED ||
        g.interiorBuilding != b+1 || !conscious(&g.actors[g.selected]) ||
        slot < 0 || slot >= HOME_PLACES || type < 0 || type >= FURN_TYPES) return 0;
    int previous = g.plans.furniture[b][slot];
    if (!type) {
        if (!previous) return 0;
        InteriorProp p = home_prop(previous, g.plans.x[b][slot],g.plans.y[b][slot]);
        for (int i=0;i<g.actorCount;i++) {
            Actor *a=&g.actors[i];
            if (a->active && actor_in_bed(a) && depth_room(a)==b+1 &&
                fabsf(depth_x(a)-p.x)<p.w+.35f && fabsf(depth_y(a)-p.y)<p.d+.35f) {
                log_msg("Someone is using it."); return 0;
            }
        }
    } else {
        if ((previous && previous != type) || !home_site_valid(type,b,slot,x,y)) {
            log_msg("That spot is occupied or obstructed."); return 0;
        }
        if (!previous && g.money < homeFurniture[type].price) {
            log_msg("You need %d silver.",homeFurniture[type].price); return 0;
        }
        if (!previous) g.money -= homeFurniture[type].price;
    }
    g.plans.furniture[b][slot] = type;
    g.plans.x[b][slot] = x; g.plans.y[b][slot] = y;
    if (slot < HOME_BAYS) g.homes.furniture[b][slot] = type;
    log_msg("%s %s.",homeFurniture[type ? type : previous].name, !type ? "removed" : previous ? "moved" : "placed");
    return 1;
}
int place_camp(int type, float x, float y) {
    Actor *a=&g.actors[g.selected];
    if (!placement_valid(type,-1,x,y)) { log_msg("Choose open ground outside town."); return 0; }
    int slot=-1, owner=0;
    if (type==1) {
        for(int i=0;i<PACK;i++)if(a->pack[i].item==BEDROLL_ITEM&&a->pack[i].qty){slot=i;break;}
        if(slot<0){log_msg("You need a bedroll.");return 0;}
        owner=a->stolen[slot];
    } else if (inventory_count(a,WOOD)<build_wood_cost(0)) {
        log_msg("You need %d timber.",build_wood_cost(0));return 0;
    }
    int b=add_build(x,y,type==1?BED:CAMPFIRE,FREE,g.selected);
    if(b<0)return 0;
    if(type==1){inventory_remove_slot(a,slot,1);g.field.rolls[b]=owner+1;}
    else inventory_remove(a,WOOD,build_wood_cost(0));
    g.plans.campLastNear[b]=g.time;
    g.depth.buildings[b]=(DepthBuilding){.condition=100,.fuel=12};
    if(type==0&&!g.baseExists){g.baseExists=1;g.baseX=x;g.baseY=y;g.raidTimer=420;}
    gain(a,ENGINEERING,3);
    log_msg(type==1?"Bedroll laid out.":"Campfire built.");
    return 1;
}
void placement_open(int type, int b) {
    placement_init();
    ui.placeReturn=ui.screen;
    ui.placeType=type;ui.placeBuilding=b;ui.placeSlot=-1;
    Actor *a=&g.actors[g.selected];
    ui.placeX=b>=0?3.5f:a->x+1.6f;
    ui.placeY=b>=0?3.5f:a->y;
    ui.screen=SCREEN_PLACE;
}
static int furniture_at_cursor(void) {
    int b=ui.placeBuilding,best=-1;float near=1.5f;
    for(int i=0;i<HOME_PLACES;i++)if(g.plans.furniture[b][i]){
        float d=distance(ui.placeX,ui.placeY,g.plans.x[b][i],g.plans.y[b][i]);
        if(d<near){near=d;best=i;}
    }
    return best;
}
int placement_input(float dt,int held,int pressed,float ax,float ay) {
    if(ui.screen!=SCREEN_PLACE)return 0;
    if(pressed&(KEY_CIRCLE|KEY_START)){ui.screen=pressed&KEY_START?SCREEN_WORLD:ui.placeReturn;return 1;}
    float sx=ax,sy=ay;
    if(held&KEY_LEFT)sx-=1;if(held&KEY_RIGHT)sx+=1;
    if(held&KEY_UP)sy-=1;if(held&KEY_DOWN)sy+=1;
    if(fabsf(sx)+fabsf(sy)>.15f){
        float step=dt*(ui.placeBuilding>=0?2.1f:3.1f);
        ui.placeX+=(sx+sy)*step;ui.placeY+=(sy-sx)*step;
        if(ui.placeBuilding>=0){ui.placeX=fmaxf(.5f,fminf(6.5f,ui.placeX));ui.placeY=fmaxf(.5f,fminf(6.5f,ui.placeY));}
        else {Actor*a=&g.actors[g.selected];float dx=ui.placeX-a->x,dy=ui.placeY-a->y,len=hypotf(dx,dy);if(len>9.5f){ui.placeX=a->x+dx*9.5f/len;ui.placeY=a->y+dy*9.5f/len;}}
    }
    if(ui.placeBuilding>=0&&ui.placeType==FURN_EMPTY){
        int slot=furniture_at_cursor();
        if((pressed&KEY_SQUARE)&&slot>=0)home_place(ui.placeBuilding,slot,0,ui.placeX,ui.placeY);
        if((pressed&KEY_CROSS)&&slot>=0){ui.placeSlot=slot;ui.placeType=g.plans.furniture[ui.placeBuilding][slot];ui.placeX=g.plans.x[ui.placeBuilding][slot];ui.placeY=g.plans.y[ui.placeBuilding][slot];}
        return 1;
    }
    if(pressed&KEY_CROSS){
        int result;
        if(ui.placeBuilding>=0){
            int slot=ui.placeSlot;
            if(slot<0)for(int i=0;i<HOME_PLACES;i++)if(!g.plans.furniture[ui.placeBuilding][i]){slot=i;break;}
            if(slot<0){log_msg("There is no room for more furniture.");return 1;}
            result=home_place(ui.placeBuilding,slot,ui.placeType,ui.placeX,ui.placeY);
        }else result=place_camp(ui.placeType,ui.placeX,ui.placeY);
        if(result)ui.screen=SCREEN_WORLD;
    }
    return 1;
}
void placement_tick(float dt) {
    placement_init();
    static float clock;clock+=dt;if(clock<1)return;clock=fmodf(clock,1);
    for(int i=0;i<CONTRACTS;i++)if(g.plans.dispatchPending[i]){
        Contract*c=&g.contracts[i];
        if(c->active!=1||c->progress>=c->needed){g.plans.dispatchPending[i]=0;continue;}
        for(int n=0;n<g.actorCount;n++)if(g.actors[n].active&&g.actors[n].squad&&!g.actors[n].dead&&inventory_add(&g.actors[n],DISPATCH_ITEM,1)){
            g.plans.dispatchPending[i]=0;break;
        }
    }
    int bases=0;
    for(int b=0;b<g.buildingCount;b++){
        Building *p=&g.buildings[b];
        if(p->owner<0||portable_bedroll(b)<0)continue;
        if(p->type!=CAMPFIRE&&p->type!=BED){bases++;continue;}
        int near=0,used=0;
        for(int i=0;i<g.actorCount;i++){
            Actor*a=&g.actors[i];
            if(a->active&&!a->dead&&a->squad&&distance(a->x,a->y,p->x,p->y)<35)near=1;
            if(a->active&&actor_in_bed(a)&&a->taskIndex==b)used=1;
        }
        if(near||used)g.plans.campLastNear[b]=g.time;
        if(g.time-g.plans.campLastNear[b]>=DAY_SECONDS){
            g.field.rolls[b]=-1;g.depth.buildings[b]=(DepthBuilding){0};
            for(int i=0;i<g.actorCount;i++)if(g.actors[i].taskKind==4&&g.actors[i].taskIndex==b)g.actors[i].taskKind=0;
        }else if(p->type==CAMPFIRE){if(!bases){g.baseX=p->x;g.baseY=p->y;}bases++;}
    }
    g.baseExists=bases>0;
    population_prune_corpses();
    if((int)g.time%30==0)depot_init();
}
