/* Persistent locations plus a bounded travel director. No additional actor pool. */
typedef struct { const char *name,*record; int a,b,c,loot; } WildSite;
static const WildSite wildSites[WILDERNESS_SITES]={
 {"Cinderwheel Foundry","The furnace ledger ends mid-shift. Spare metal was sealed in the stores.",FORGE,STORE,WINDMILL,IRONBAR},
 {"Last Lantern Watch","The watch withdrew toward the towns. The supply room was never emptied.",TOWER,HOUSE,WALL,MEDKIT},
 {"Shrine of the Empty Bell","Pilgrims sheltered here before the road fell silent. Offerings remain.",HOUSE,STORE,WELL,RELIC},
 {"Salt Road Caravanserai","A faded manifest lists food, cloth and a caravan that never returned.",BAR,SHOP,CAMPFIRE,RATION},
 {"Splitrock Quarry Works","The cutting crews fled the quarry. Tools and ore lie behind the works.",FORGE,STORE,WINDMILL,ORE},
 {"Redoubt Nine","A last order reads: hold until relieved. No relief ever came.",TOWER,STORE,WALL,BOLTS},
 {"White Reed Infirmary","A medic's note asks travelers to leave dressings for the next wounded soul.",HOUSE,HOUSE,WELL,MEDKIT},
 {"Derelict Freight Yard","Abandoned freight crates carry the marks of three rival settlements.",STORE,SHOP,CAMPFIRE,PARTS},
 {"Black Kiln Factory","Cold kilns and sealed stores outlived the workers. Salvagers came later.",FORGE,STORE,WINDMILL,COPPER},
 {"Broken Signal Station","The signal crew recorded lights moving beyond the dunes before vanishing.",TOWER,SHOP,WELL,PARTS},
 {"Sunken Road Sanctuary","The sanctuary register names refugees from villages no longer on the road.",HOUSE,BAR,WALL,CLOTH},
 {"Hollow Mule Waystation","The stable hands hid emergency rations when the caravans stopped coming.",SHOP,BAR,WELL,JERKY},
 {"Ironwake Pump Works","A maintenance slate warns of a seized pump. Replacement parts remain.",FORGE,STORE,WINDMILL,PARTS},
 {"Ashgate Border Fort","The gate changed hands many times. Its last garrison left weapons behind.",TOWER,HOUSE,WALL,IRONBAR},
 {"Pale Glass Hospice","The beds are empty. A locked-away supply chest survived the evacuation.",HOUSE,SHOP,WELL,TONIC},
 {"Long Haul Exchange","Freight accounts stop on the same day. Something interrupted the handover.",STORE,FORGE,CAMPFIRE,RELIC}
};
int wilderness_site(int b){return b>=0&&b<g.buildingCount&&g.frontier.variant[b]>=BUILD_WILD_FIRST&&g.frontier.variant[b]<=BUILD_WILD_LAST?g.frontier.variant[b]-BUILD_WILD_FIRST:-1;}
const char *wilderness_name(int i){return i>=0&&i<WILDERNESS_SITES?wildSites[i].name:"Abandoned site";}
const char *wilderness_record(int i){return i>=0&&i<WILDERNESS_SITES?wildSites[i].record:"The marks have weathered away.";}
static int wild_site_clear(float x,float y){
 if(x<20||y<20||x>WORLD-20||y>WORLD-20||town_near(x,y,38)>=0||observers(x,y)<34)return 0;
 if(g.baseExists&&distance(x,y,g.baseX,g.baseY)<35)return 0;
 for(int j=0;j<g.buildingCount;j++)if(distance(x,y,g.buildings[j].x,g.buildings[j].y)<19)return 0;
 for(int dx=-8;dx<=8;dx+=2)for(int dy=-6;dy<=6;dy+=2)if(!can_walk(x+dx,y+dy)||terrain_road(x+dx,y+dy))return 0;
 return 1;
}
void wilderness_init(void){
 if(g.wilderness.version)return;
 memset(&g.wilderness,0,sizeof(g.wilderness));g.wilderness.version=1;
 static const int start[]={0,1,3,5};
 for(int i=0;i<WILDERNESS_SITES;i++){
  if(g.buildingCount>MAX_BUILDINGS-20)break; /* reserve construction capacity */
  float ox,oy;if(i<8){int t=start[i/2];ox=towns[t].x;oy=towns[t].y;}else{ox=230+(i-8)%4*330;oy=700+(i-8)/4*500;}
  float x=0,y=0;int found=0;
  for(int r=0;r<18&&!found;r++)for(int a=0;a<24;a++){
   float angle=(a+i*7)*.2617994f,rad=(i<8?48:0)+r*5;
   x=ox+cosf(angle)*rad;y=oy+sinf(angle)*rad;
   if(wild_site_clear(x,y)){found=1;break;}
  }
  if(!found)continue;
  const WildSite *s=&wildSites[i];
  int types[]={s->a,s->b,s->c};
  for(int j=0;j<3;j++){
   float bx=x+(j==1?7:j==2?-5:0),by=y+(j==1?(i%2?3:-2):j==2?4:0);
   int b=add_build(bx,by,types[j],FREE,-1);if(b<0)break;
   g.frontier.variant[b]=BUILD_WILD_FIRST+i;
   if(!j)g.wilderness.building[i]=b+1;
   if(building_has_interior(types[j])){
    g.buildings[b].storage[0]=(Stack){s->loot,1+(i+j)%3};
    g.buildings[b].storage[1]=(Stack){j?CLOTH:PARTS,1+(i%2)};
    g.buildings[b].storage[2]=(Stack){i<8?OATCAKE:MEDKIT,1};
   }
  }
 }
}
int wilderness_validate(const Game *s){
 if(s->wilderness.version<0||s->wilderness.version>1||s->wilderness.sequence<0||!isfinite(s->wilderness.quiet)||!isfinite(s->wilderness.danger)||!isfinite(s->wilderness.cooldown)||s->wilderness.quiet<0||s->wilderness.danger<0||s->wilderness.cooldown<0)return 0;
 for(int i=0;i<WILDERNESS_SITES;i++){int b=s->wilderness.building[i];if(s->wilderness.guardSerial[i]<0||b<0||b>s->buildingCount||(b&&s->frontier.variant[b-1]!=BUILD_WILD_FIRST+i))return 0;}
 return 1;
}
static int wilderness_guard_group(int k){int i=k-48;return i>=0&&i<WILDERNESS_SITES&&g.wilderness.guardSerial[i]>0&&g.wilderness.guardSerial[i]==P.groups[k].serial;}
static void wilderness_guards(void){
 for(int i=0;i<WILDERNESS_SITES;i++){
  int b=g.wilderness.building[i]-1,k=48+i;
  if(b<0||i%4==1||g.wilderness.guardSerial[i]||P.groups[k].spawned||observers(P.groups[k].x,P.groups[k].y)<100||warrant_preserve(k))continue;
  EncounterSite site={g.buildings[b].x,g.buildings[b].y,HOSTILE,0,i<8?1:2};
  generate_population(k,POP_RAIDERS,&site);g.wilderness.guardSerial[i]=P.groups[k].serial;
  for(int m=0;m<P.groups[k].count;m++){
   DepthActor *z=&P.groups[k].reserveDepth[m];z->room=b+1;z->rx=2+m*2;z->ry=3.5f;
   P.groups[k].reserve[m].homeTown=-1;
  }
 }
}
/* Only retire off-screen, healthy passing traffic: never a visible camp,
   corpse, prisoner, recruit, bounty, quest party, or someone carrying a casualty. */
static int travel_slot(const float *near){
 for(int pass=0;pass<2;pass++)for(int k=pass?16:WAR_POP_GROUPS;k<(pass?48:POP_GROUPS);k++){
  WorldEvent *e=&P.groups[k];GroupLife *z=&g.life.groups[k];
  if(wilderness_guard_group(k)||e->type==POP_ARMY||e->type==POP_PRISONERS||e->type==POP_DUELISTS)continue;
  if(e->spawned||near[k]<100||warrant_preserve(k)||P.state[k].detached||P.state[k].phase==POP_FIGHT)continue;
  if(g.fieldNotes.group[k].serial==e->serial&&g.time-g.fieldNotes.group[k].time<180)continue;
  if((camp_type(k)&&e->age<300)||z->goal==LIFE_RESCUE||z->goal==LIFE_DELIVER||z->goal==LIFE_REINFORCE||z->goal==LIFE_RAID)continue;
  int healthy=1;for(int m=0;m<e->count;m++){Actor*a=member(k,m);if(!a||a->dead||a->unconscious>0||a->bleed>.01f||a->prisonTime>0||a->carrying>=0||a->carriedBy>=0||e->reserveDepth[m].captive||e->reserveDepth[m].room)healthy=0;}
  if(healthy||reusable(k,near[k]))return k;
 }
 for(int k=0;k<POP_GROUPS;k++)if(reusable(k,near[k]))return k;
 return -1;
}
static int travel_site_ok(EncounterSite site,int slot,const Actor *lead){
 float x=site.x,y=site.y;
 if(x<10||y<10||x>WORLD-10||y>WORLD-10||town_near(x,y,29)>=0||(g.baseExists&&distance(x,y,g.baseX,g.baseY)<32))return 0;
 for(int n=0;n<g.actorCount;n++)if(g.actors[n].squad&&g.actors[n].active&&!g.actors[n].dead){Actor*a=&g.actors[n];
  if(distance(x,y,a->x,a->y)<27||(fabsf(view_horizontal(x-a->x,y-a->y))<17&&fabsf(view_vertical(x-a->x,y-a->y))<21))return 0;
 }
 for(int k=0;k<POP_GROUPS;k++)if(k!=slot&&life_alive(k)&&distance(x,y,P.groups[k].x,P.groups[k].y)<17)return 0;
 for(int j=0;j<g.buildingCount;j++)if(distance(x,y,g.buildings[j].x,g.buildings[j].y)<9)return 0;
 if(!can_walk(x,y)||!can_walk(x+3,y)||!can_walk(x-3,y)||!can_walk(x,y+3)||!can_walk(x,y-3))return 0;
 for(int d=1;d<12;d++){float u=d/12.f;if(!can_walk(lead->x+(x-lead->x)*u,lead->y+(y-lead->y)*u))return 0;}
 return 1;
}
static void travel_director(const float *near,float dt){
 Actor *a=&g.actors[g.selected];
 wilderness_guards();
 for(int i=0;i<WILDERNESS_SITES;i++){int b=g.wilderness.building[i]-1;if(b>=0&&!(g.wilderness.discovered&(1u<<i))&&!actor_inside(a)&&distance(a->x,a->y,g.buildings[b].x,g.buildings[b].y)<22){g.wilderness.discovered|=1u<<i;log_msg("Discovered %s.",wildSites[i].name);}}
 if(!conscious(a)||actor_inside(a)||a->movedTimer<=0||town_near(a->x,a->y,23)>=0)return;
 g.wilderness.quiet=fminf(120,g.wilderness.quiet+dt);g.wilderness.danger=fminf(180,g.wilderness.danger+dt);g.wilderness.cooldown=fmaxf(0,g.wilderness.cooldown-dt);
 int hostileClose=0,forward=0;float sx,sy,dx,dy;walk_heading(a,&sx,&sy);view_to_world(sx,sy,&dx,&dy);float len=fmaxf(.001f,hypotf(dx,dy));dx/=len;dy/=len;
 for(int n=0;n<g.actorCount;n++){Actor*b=&g.actors[n];if(!b->active||b->squad||!conscious(b)||depth_room(b))continue;
  float x=b->x-a->x,y=b->y-a->y;
  if(fabsf(view_horizontal(x,y))<11&&fabsf(view_vertical(x,y))<13){g.wilderness.quiet=0;if(hostile(a,b))g.wilderness.danger=0;}
  if(hostile(a,b)&&hypotf(x,y)<14)hostileClose++;
 }
 for(int k=0;k<POP_GROUPS;k++)if(life_alive(k)){
  float x=P.groups[k].x-a->x,y=P.groups[k].y-a->y,d=hypotf(x,y);
  if(d<16&&population_object_count(k))g.wilderness.quiet=0;
  if(!wilderness_guard_group(k)&&P.groups[k].count<=3&&d<42&&x*dx+y*dy>0&&fabsf(x*dy-y*dx)<18)forward++;
 }
 if(g.wilderness.cooldown>0||hostileClose||forward>=2||(g.wilderness.quiet<14&&g.wilderness.danger<50))return;
 int slot=travel_slot(near);if(slot<0)return;
 for(int trial=0;trial<32;trial++){
  float angle=(trial%2?1:-1)*(trial/8)*.35f,ca=cosf(angle),sa=sinf(angle),ux=dx*ca-dy*sa,uy=dx*sa+dy*ca;
  float ahead=29+random_step(&P.rng)%11,side=(int)(random_step(&P.rng)%11)-5;
  EncounterSite site={a->x+ux*ahead-uy*side,a->y+uy*ahead+ux*side,0,0,0};
  if(!travel_site_ok(site,slot,a))continue;
  int seq=g.wilderness.sequence++%6,safe=starter_country(site.x,site.y),type;
  if(g.wilderness.danger>=50||seq==0||seq==3)type=POP_RAIDERS;
  else if(seq==2)type=POP_BEASTS;
  else if(seq==1){static const int camps[]={POP_SCAVENGERS,POP_WOUNDED,POP_SALVAGE,POP_HUNTERS,POP_SMUGGLERS};type=camps[(g.wilderness.sequence/6)%5];site.beds=2;}
  else if(seq==4)type=POP_MERCHANTS;
  else {static const int others[]={POP_REFUGEES,POP_HUNTERS,POP_PATROL,POP_AMBUSH};type=others[(g.wilderness.sequence/6)%4];site.beds=type==POP_AMBUSH?2:0;}
  site.size=type==POP_RAIDERS||type==POP_BEASTS?(safe?1+random_step(&P.rng)%2:2+random_step(&P.rng)%2):2;
  generate_population(slot,type,&site);
  /* Traffic crosses the route instead of fleeing toward a remote town. */
  if(!site.beds){P.state[slot].targetX=a->x-ux*18+uy*6;P.state[slot].targetY=a->y-uy*18-ux*6;clear_position(&P.state[slot].targetX,&P.state[slot].targetY);}
  g.wilderness.cooldown=20;g.wilderness.quiet=0;
  if(type==POP_RAIDERS||type==POP_BEASTS||type==POP_AMBUSH)g.wilderness.danger=0;
  return;
 }
}
