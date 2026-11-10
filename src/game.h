#ifndef DUSTBOUND_GAME_H
#define DUSTBOUND_GAME_H
#include <stdint.h>
#define VERSION "0.18.5"
#define WORLD 1536
#define FOG_SIZE 384
#define WORK_REACH 1.15f
#define TALK_REACH 1.6f
#define MAX_ACTORS 192
#define MAX_BUILDINGS 384
#define MAX_NODES 640
#define PACK 24
#define SKILLS 20
#define LIMBS 6
#define BASE_ITEMS 36
#define FAMILIES 88
#define WEAPON_FAMILIES 36
#define MATERIALS 6
#define QUALITIES 4
#define CATALOG_ITEMS (BASE_ITEMS + FAMILIES * MATERIALS * QUALITIES)
#define V090_ITEMS (CATALOG_ITEMS + 88)
#define CIV_ARMOR_ITEMS V090_ITEMS
#define REGALIA_ITEMS (CIV_ARMOR_ITEMS+96)
#define V100_ITEMS (REGALIA_ITEMS+36)
#define LIMB_ITEMS V100_ITEMS
#define SEVERED_ARM (LIMB_ITEMS+8)
#define SEVERED_LEG (LIMB_ITEMS+9)
#define TOWN_STOCK_ITEMS (LIMB_ITEMS+10)
#define BEDROLL_ITEM TOWN_STOCK_ITEMS
#define SLAVE_GARB (BEDROLL_ITEM+1)
#define V23_ITEMS (SLAVE_GARB+1)
#define JERKY V23_ITEMS
#define OATCAKE (JERKY+1)
#define STEW (OATCAKE+1)
#define HERBAL_TEA (STEW+1)
#define PORTION_ITEMS (HERBAL_TEA+1)
#define PORTION_TYPES 6
#define ITEMS (PORTION_ITEMS+PORTION_TYPES*99)
#define LODGES 3
#define COMPLETED_CONTRACTS 32
#define HOME_BAYS 6
#define HOME_STASH 64
#define ELITE_WARRANTS 3
#define WARRANT_RECEIPTS 16
#define WARRANT_ROWS (ELITE_WARRANTS + 8)
#define SKILL_CAP 150
#define SKILL_XP_CAP (12 * (SKILL_CAP - 1) * (SKILL_CAP - 1))
#define SLAVE_CAMPS 3
#define SEIZED_STACKS (SQUAD * (PACK + GEARS + 1))
#define SPECIES 5
#define START_SCENARIOS 5
#define ELITES 18
#define EVENTS 8
#define GEARS 6
#define FACTIONS 8
#define CONTRACTS 24
#define DAY_SECONDS 1800
#define GUIDE_PAGES 25
#define SAVE_SLOTS 7
#define OLD_TOWNS 16
#define TOWNS 19
#define SQUAD 8
#define LOGS 24
enum {
    ATHLETICS,
    STRENGTH,
    DEXTERITY,
    MELEE,
    DEFENCE,
    TOUGHNESS,
    DODGE,
    MARTIAL,
    STEALTH,
    THEFT,
    LOCKPICK,
    MEDICINE,
    LABOUR,
    FARMING,
    COOKING,
    SMITHING,
    ENGINEERING,
    SCIENCE,
    CROSSBOWS,
    PERCEPTION
};
enum { HEAD, CHEST, LARM, RARM, LLEG, RLEG };
enum {
    WOUND_CUTS = (1u << LIMBS) - 1, WOUND_COMA = 1u << LIMBS, WOUND_AID_CHECKED = 1u << (LIMBS + 1),
    WOUND_REPAIR_SHIFT = LIMBS + 2, WOUND_REPAIRS = WOUND_CUTS << WOUND_REPAIR_SHIFT,
    WOUND_CLINIC = 1u << (WOUND_REPAIR_SHIFT + LIMBS), WOUND_TONIC = WOUND_CLINIC << 1,
    WOUND_CRITICAL_AWAKE = WOUND_TONIC << 1,
    WOUND_FLAGS = (WOUND_CRITICAL_AWAKE << 1) - 1
};
enum { TASK_CELL_LOCK = 5, TASK_TREAT, TASK_JAIL_CARE, TASK_TRAIN };
enum { REST_DURATION = 300 };
enum { TREAT_DRESS, TREAT_PRESSURE, TREAT_REPAIR, TREAT_CLINIC };
enum {
    NONE,
    RATION,
    MEDKIT,
    ORE,
    COPPER,
    WOOD,
    CLOTH,
    WATER,
    GRAIN,
    MEAT,
    IRONBAR,
    RUSTSWORD,
    SABRE,
    CLEAVER,
    STAFF,
    CROSSBOW,
    BOLTS,
    RAGS,
    LEATHER,
    PLATE,
    DUSTER,
    STRAW,
    HAT,
    HELM,
    BOOTS,
    SANDALS,
    ARM,
    LEG,
    TOOLS,
    RELIC,
    CARTRIDGE,
    HIDE,
    COAL,
    PARTS,
    TONIC,
    MAPSCROLL
};
enum { PLAYER, SETTLER, GUARD, TRADER, RECRUIT, RAIDER, BEAST, CARAVAN };
enum { FREE, UNION, SCAVENGERS, HOSTILE, SHOGUN, MONKS, OUTLAWS, WILDLIFE };
enum { HOUSE, SHOP, BAR, TOWER, WINDMILL, WELL, CAMPFIRE, BED, FARM, FORGE, BENCH, WALL, STORE, JAIL, SLAVE_YARD };
enum { NODE_ORE, NODE_COPPER, NODE_WOOD, NODE_LOOT, NODE_LOCKED, NODE_RUIN };
enum { JOB_FOLLOW, JOB_HOLD, JOB_MINE, JOB_FARM, JOB_MEDIC, JOB_SMELT, JOB_COOK, JOB_HAUL };
enum {
    SCREEN_WORLD,
    SCREEN_PACK,
    SCREEN_SKILLS,
    SCREEN_HEALTH,
    SCREEN_SQUAD,
    SCREEN_MAP,
    SCREEN_TRADE,
    SCREEN_RECRUIT,
    SCREEN_BUILD,
    SCREEN_CRAFT,
    SCREEN_RESEARCH,
    SCREEN_MENU,
    SCREEN_HELP,
    SCREEN_TITLE,
    SCREEN_CONFIRM,
    SCREEN_CREATE,
    SCREEN_NAME,
    SCREEN_DIALOG,
    SCREEN_FORGE,
    SCREEN_JOURNAL,
    SCREEN_FACTIONS,
    SCREEN_STORAGE,
    SCREEN_SAVES,
    SCREEN_EQUIPPED,
    SCREEN_LOOT,
    SCREEN_ASSIST,
    SCREEN_LOG, SCREEN_EXPEDITION, SCREEN_ORDERS, SCREEN_CAMP, SCREEN_CARE, SCREEN_ENCOUNTER, SCREEN_LAYERS,
    SCREEN_THEFT, SCREEN_ENGAGE, SCREEN_ACTIONS, SCREEN_LIMBS, SCREEN_LIMBFIT, SCREEN_SURGERY, SCREEN_LIMBCONFIRM, SCREEN_LIFE, SCREEN_POLITICS,
    SCREEN_PASS, SCREEN_SCENARIOS, SCREEN_CHEST, SCREEN_HOME, SCREEN_WARRANTS
};
enum {
    KEY_UP = 1,
    KEY_DOWN = 2,
    KEY_LEFT = 4,
    KEY_RIGHT = 8,
    KEY_CROSS = 16,
    KEY_CIRCLE = 32,
    KEY_SQUARE = 64,
    KEY_TRIANGLE = 128,
    KEY_L = 256,
    KEY_R = 512,
    KEY_START = 1024,
    KEY_SELECT = 2048
};
typedef struct {
    int item, qty;
} Stack;
typedef struct {
    const char *name, *desc;
    int price, weight, slot, power, armor;
    uint32_t color;
    int family, material, quality, effect, potency, penetration, accuracy, speed, stealth, capacity,
        protection;
    float range, cooldown;
    uint32_t trim;
} ItemDef;
typedef struct {
    char name[16];
    float x, y, homeX, homeY, tx, ty, health[LIMBS], blood, hunger, fatigue, bleed, cooldown, action, anim,
        unconscious;
    float xp[SKILLS];
    Stack pack[PACK];
    int gear[GEARS];
    int active, kind, faction, squad, dead, job, target, taskKind, taskIndex, sneaking, autoattack, carrying,
        carriedBy, lostMask, prostheticMask, bounty;
    unsigned woundFlags; /* occupies the unused arrested field in older saves */
    int healthScale; /* percent of species health, or zero for the normal maximum */
    int body, skin, hair, hairColor, beard, scar, origin, race, facing, homeTown;
    int voice, speech, prisonTown, stolen[PACK], bountyReward;
    float speechTime, speechCooldown, prisonTime, stun, exposure;
    int treatedMask; /* occupies the unused bleedTimer save field */
    int routeLen, routePos, routeX[24], routeY[24];
    float routeClock, routeTargetX, routeTargetY;
    int attackPhase, attackTarget, attackWeapon, impactLimb, impactDamage, impactKind, defeatCredited,
        gearStolen[GEARS];
    float attackClock, attackDuration, attackAngle, hitFlash, recoilX, recoilY, movedTimer;

} Actor;
typedef struct {
    float x, y;
    int type, faction, owner, level, stock;
    float progress;
    Stack storage[16];
} Building;
typedef struct {
    float x, y;
    int type, remaining, locked, owner, item, qty;
    float regen;
} Node;
typedef struct {
    const char *name;
    float x, y;
    int faction;
} Town;
typedef struct {
    int active, type, town, target, item, needed, progress, reward, completed, baseline;
} Contract;
enum { CONTRACT_DISPATCH, CONTRACT_ORE, CONTRACT_HUNT, CONTRACT_RELIC, CONTRACT_FREIGHT,
       CONTRACT_ESCORT, CONTRACT_PATROL, CONTRACT_BOUNTY, CONTRACT_TYPES };
enum { DIALOG_WORK = 1, DIALOG_INSPECTION = 6, DIALOG_DISMISS = 7, DIALOG_WORK_HINT = 8,
       DIALOG_BOUNTY = 9, DIALOG_SUMMONS = 10 };
typedef struct {
    int layout;
    int chestRefresh[MAX_BUILDINGS];
    int boardCycle[TOWNS];
    unsigned char boardTaken[TOWNS], theftCount[TOWNS];
    float theftUntil[TOWNS], searchAfter[TOWNS], boardAfter[TOWNS];
    float huntX[CONTRACTS], huntY[CONTRACTS];
    float squadHitAt[MAX_ACTORS];
} TownLife;
typedef struct {
    int exists, valid, version, day, squad, money;
    char name[16];
} SaveInfo;
#include "depth_types.h"
typedef struct { int version,outcome[3],toll[3]; float campX,campY; } RegionalState;
typedef struct {
    unsigned seed; int serial,type,modifier,faction,town,origin,tier,count,status,spawned,participated;
    float x,y,age,presence; int members[16],stored,reserveAnger[16];
    Actor reserve[16]; DepthActor reserveDepth[16];
} WorldEvent;
typedef struct {
    int version,serial,history[16],historyPos,owned[MAX_ACTORS],anger[MAX_ACTORS],command[MAX_ACTORS];
    float pulse; WorldEvent events[EVENTS];
} LivingWorld;
#include "society_types.h"
#include "population_types.h"
#include "life_types.h"
enum { HOME_NONE, HOME_FOR_SALE, HOME_OWNED };
enum { FURN_EMPTY, FURN_BED, FURN_CHEST, FURN_TABLE, FURN_SHELF, FURN_STOVE, FURN_TYPES };
typedef struct {
    const char *name;
    int price, art, action;
    float w, d;
} HomeFurniture;
typedef struct {
    int version;
    unsigned char property[MAX_BUILDINGS], furniture[MAX_BUILDINGS][HOME_BAYS];
    Stack stash[HOME_STASH];
    unsigned char stolen[HOME_STASH];
} Homes;
typedef struct {
    float x, y, seen, until;
    int serial;
    unsigned char tracked, down;
} WarrantLead;
typedef struct {
    char name[16];
    int silver, day;
    unsigned char faction, dead;
} WarrantReceipt;
typedef struct {
    int version;
    float pulse;
    WarrantLead elite[ELITE_WARRANTS], local[POP_GROUPS];
    WarrantReceipt eliteReceipts[ELITE_WARRANTS];
    WarrantReceipt receipts[WARRANT_RECEIPTS];
    unsigned char next, count;
} Warrants;
typedef struct {
    const char *name, *offense;
    int reward, elite, down, tracked, town;
    unsigned payers;
    float x, y, seen;
} WarrantView;
typedef struct {
    uint32_t seed, rng;
    float time, dayTime, raidTimer, autosave;
    int day, money, selected, squadCount, tech, rep[FACTIONS], kills, discovered[TOWNS], stock[TOWNS][TOWN_STOCK_ITEMS],
        lastRestock;
    int actorCount, buildingCount, nodeCount, baseExists;
    float baseX, baseY;
    uint8_t seen[FOG_SIZE * FOG_SIZE / 8];
    Actor actors[MAX_ACTORS];
    Building buildings[MAX_BUILDINGS];
    Node nodes[MAX_NODES];
    Contract contracts[CONTRACTS];
    int townOwner[TOWNS], prosperity[TOWNS], townState[TOWNS], weather, season, contractsDone;
    int factionWar[FACTIONS], storyStage, playOrigin, unlocks, difficulty;
    float eventTimer, weatherTimer, worldPulse, hitstop, shake;
    char worldNews[96];
    /* Version 3 extension. Version 2 ends immediately before these fields. */
    int interiorBuilding, interiorActor;
    float interiorX, interiorY;
    /* v5: autonomous hired guards; actor IDs use +1, zero means vacant. */
    int mercActor[4], mercMode[4], identityVersion;
    float mercUntil[4];
    /* v6 append-only extension. v5 prefix is exactly 410656 bytes. */
    FrontierDepth depth;
    /* v7 append-only regional consequences. */
    RegionalState regions;
    LivingWorld living;
    Society society;
    struct { int version,eliteActor[ELITES],routeTown[ELITES],fallen[ELITES]; float pulse; } civil;
    struct { int version, surgeonActor; } limbs;
    /* v12: persistent population; all earlier save bytes stay in place. */
    Population population;
    FrontierLife life;
    FieldNotes fieldNotes;
    struct { int version, seen, bed[6], omit; } trails;
    FrontierWar war;
    unsigned theftWarnings;
    struct { int actor, patient, bed, generated, day; float until; } roadsideAid;
    TownLife townLife; /* v21 extension */
    struct {
        int version;
        int stock[TOWNS][V23_ITEMS - TOWN_STOCK_ITEMS];
        unsigned short tradeCredit[FACTIONS];
        signed char rolls[MAX_BUILDINGS]; /* -1 packed, 0 fixed, otherwise 1 + stolen owner */
        int jail[TOWNS], camp[SLAVE_CAMPS]; /* building index plus one */
        int intake[MAX_ACTORS]; /* destination building plus one, while being carried */
        unsigned char laborCamp[MAX_ACTORS], escaped[MAX_ACTORS], playingDead[MAX_ACTORS];
        float cellOpen[MAX_ACTORS];
        Stack seized[SLAVE_CAMPS][SEIZED_STACKS];
        unsigned char seizedOwner[SLAVE_CAMPS][SEIZED_STACKS];
    } field; /* v22 extension */
    unsigned short charges[MAX_ACTORS][FACTIONS]; /* v23 extension */
    struct {
        int version, lodge[LODGES], clanMember;
        unsigned char variant[MAX_BUILDINGS];
        unsigned short giver[CONTRACTS]; /* actor index plus one */
        unsigned short completedNext, completedCount;
        Contract completed[COMPLETED_CONTRACTS];
        float taskTotal[MAX_ACTORS];
        unsigned char treatmentDose[MAX_ACTORS];
        signed char treatmentPhase[MAX_ACTORS]; /* 0 bleeding, 1..6 limbs, -1 other */
        int stock[TOWNS][ITEMS - V23_ITEMS];
        unsigned char venue[TOWNS][ITEMS];
        unsigned char knownSites[LANDMARKS], tollKnown, knownLodges[LODGES];
    } frontier; /* v24 extension */
    Homes homes; /* v26 extension */
    Warrants warrants;
} Game;
enum { CHEST_ROOM, CHEST_FIELD, CHEST_PROPERTY };
enum { ROOM_NONE, ROOM_TRADE, ROOM_REST, ROOM_FORGE, ROOM_STORAGE, ROOM_CHEST, ROOM_SMITH_TRADE, ROOM_MEMBERSHIP, ROOM_TRAIN, ROOM_HOME_STASH };
enum { ROOM_PROPS = 18 };
#define DOOR_REACH 1.65f
#define CELL_Y 3.65f
#define CELL_HINGE_X 2.65f
#define CELL_LATCH_X 4.35f
enum { BUILD_NORMAL, BUILD_ARMORER, BUILD_LODGE };
enum { TRADE_GENERAL, TRADE_SMITH, TRADE_INN, TRADE_ARMORER, TRADE_APOTHECARY };
typedef struct {
    const char *inn, *market;
    unsigned char beds, smith, custody, marketTier, plan, turn;
} TownDesign;
const TownDesign *town_design(int town);
int building_town(int id);
int starter_country(float x, float y);
unsigned bounty_payers(int target);
int bounty_guard(int guard, int target);
int bounty_turn_in(int guard);
void bounty_details(int target, char *out, int size);
int bounty_station(int guard);
int bounty_board(int building);
void warrants_init(void);
void warrants_tick(float dt);
int warrants_validate(const Game *save);
unsigned warrant_terms(int actor);
int warrant_authorizes(int actor);
int warrant_list(int town, int *ids);
int warrant_view(int id, WarrantView *out);
void warrant_track(int id);
void warrant_paid(int actor, int faction, int silver);
void warrant_register(int group);
int warrant_preserve(int group);
void population_detain(int actor);
const WarrantReceipt *warrant_receipt(int row);
int warrant_receipt_count(void);
const char *building_name(int id);
void civil_clear_position(float *x, float *y);
void building_footprint(int id, float *w, float *d, int *height);
typedef struct { float low, high; int alongY; } WallRun;
WallRun building_wall_run(int id);
void building_geometry_reset(void);
int structure_blocks(int id, float x, float y, float padding);
void building_door(int id, float *x, float *y);
float building_door_distance(int id, float x, float y);
typedef struct {
    float x, y, w, d;
    int art, action;
} InteriorProp;
int building_has_interior(int type);
int lodge_joined(int building);
void training_tick(Actor *a, float dt);
int interior_enter(int building);
void actor_enter_room(int actor, int building);
void interior_leave(void);
int actor_inside(const Actor *a);
int interior_props(int building, InteriorProp *props);
int interior_walkable(int room, float x, float y);
void interior_clear_position(Actor *a);
int jail_room_open(int room);
void room_route_toward(Actor *a, float x, float y, float dt);
void town_routines_reset(void);
int interior_context(void);
const char *interior_prompt(void);
void interior_interact(void);
void interior_move(float dx, float dy, float dt);
int interior_chest_count(void);
const Stack *interior_chest_item(int row);
void interior_chest_take(int row);
int chest_owner(void);
int deploy_bedroll(Actor *a, int slot);
int portable_bedroll(int building);
void pack_bedroll(int building);
void field_init(void);
int portion_base(int item);
int portion_percent(int item);
int portion_item(int base, int percent);
int food_capacity(int item);
void portion_content(void);
float treatment_progress(const Actor *a);
int treatment_patient(const Actor *a);
int lay_carried_in_bed(Actor *carrier, int place);
void frontier_world_init(void);
int field_validate(const Game *save);
int *market_stock(int town, int item);
int home_occupant(int building);
int rest_at(Actor *a, int bed, float x, float y, int room, int price);
int actor_in_bed(const Actor *a);
void homes_init(void);
int homes_validate(const Game *save);
int home_state(int building);
int home_price(int building);
int home_buy(int building);
int home_furnish(int building, int bay, int type);
void home_open(int building);
extern const HomeFurniture homeFurniture[FURN_TYPES];
extern const float homeBays[HOME_BAYS][2];
int storage_slots(int building);
const Stack *storage_stack(int building, int slot);
int storage_owner(int building, int slot);
typedef struct {
    int screen, selection, scroll, tab, paused, speed, contextKind, contextIndex, tradeTown, sell, slot,
        confirmAction, titleChoice, waypoint;
    float mapX, mapY, camX, camY, noticeTime, repeat, travelX, travelY;
    char logs[LOGS][80], notice[100];
    int logCount;
    char feed[12][100];
    unsigned char feedPriority[12];
    int feedCount, wantedTown, wantedValue;
    float feedClock, feedScroll, statusClock;
    int helpReturn, helpTopic, dialogActor, dialogPage, forgeSlot, forgeFamily, forgeMaterial, forgeQuality;
    int createRow, createOrigin, createBody, createSkin, createHair, createHairColor, createBeard,
        createRace;
    int nameCursor, nameCell, storageIndex, storageSide, shopFilter, saveMode, lootActor, mapZoom, mapTown;
    int chestKind, roadTrader, tradeSmith, watchApproach; /* actor indices plus one */
    char createName[16];
    SaveInfo saves[SAVE_SLOTS];
    int helpSelection, helpScroll, saveReturn, saveExit, contextChoice, contextCount, squadCyclePending;
    int depthSite, depthRow;
    int menuTab, menuRow, menuReturn, statusPanel;
    int journalTab,regionChoice,attackFocus;
    int homeSite, homeBay, warrantTown, warrantClosed;
    int mapCategory,mapChoice,skillPage;
    int limbTarget, limbSourceSlot, limbReturn, limbConfirm;
    float travelStall, travelLastX, travelLastY;
    float playerIdle;
    int manualMove;
    int passSlot, squadPage, scenarioReturn;
    uint32_t chatterSeen;
    float chatterNext;
    unsigned char jobGather[MAX_ACTORS], danger[MAX_ACTORS], priorityJob[MAX_ACTORS];
    char workNotice[80];
} UI;
typedef struct {
    int held, pressed;
    float ax, ay;
} Input;
extern Game g;
extern UI ui;
extern volatile uint32_t display_buffer;
extern volatile uint32_t rendered_frames;
extern ItemDef itemDefs[ITEMS];
extern Town towns[TOWNS];
void restock_town(int town);
void world_layout(int expanded);
int world_is_expanded(void);
void army_ford_waypoint(float x, float y, float *tx, float *ty);
enum {
    ROLE_CITIZEN,
    ROLE_MERCHANT,
    ROLE_RECRUIT,
    ROLE_MARSHAL,
    ROLE_INNKEEPER,
    ROLE_MEDIC,
    ROLE_SMITH,
    ROLE_BARBER,
    ROLE_SCOUT,
    ROLE_MERCENARY,
    ROLE_SEATED_LEADER
};
int npc_role(const Actor *a);
int weapon_family_index(int i);
int weapon_strength(int item);
float weapon_handling(const Actor *a, int item);
void frontier_identity(void);
void living_init(void);
void living_tick(float dt);
int living_validate(const Game *s);
void living_generate(int slot);
int living_owned(int n);
int living_hostile(const Actor *a,const Actor *b);
int living_walk(int n,float dt);
void living_loot(int n,int role,int tier);
void living_gear(Actor *actor,int role,int tier,int seed);
void living_title(int slot,char *out,int size);
void living_detail(int slot,char *out,int size);
void living_action(int slot);
int combat_candidate(int row);
int combat_candidates(void);
void command_attack(int n);
int actor_relation(int n);
void frontier_restock(int town);
int regional_town(int region);
int regional_at(float x,float y);
void regional_init(void);
int regional_validate(const Game *state);
int regional_named(int actor);
int regional_hostile(const Actor *a,const Actor *b);
float regional_price(int town,int item);
void regional_restock(int town);
int regional_bed_cost(int town);
int regional_well(Actor *actor,Building *well);
void frontier_tick(float dt);
int merc_slot(const Actor *a);
int merc_hire(int actor, int mode);
void merc_dismiss(int actor);
const char *faction_identity(int faction);
const char *npc_rumor(const Actor *a);
const char *npc_role_name(const Actor *a);
int dialogue_count(int actor);
int dialogue_action(int actor, int row);
const char *dialogue_option(int actor, int row);
int dialogue_dismissive(int actor);
int work_dialogue_count(int town);
int work_contact(int town);
const char *work_hint(int town, int speaker);
int contract_destination(int town, int faction);
int society_job_available(int town, int kind);
int society_bounty_reward(int town);
int town_export(int town);
void town_life_init(void);
float settlement_time(void);
void town_theft(int town);
void pocket_inspection(int guard, int allow);
int watch_town(const Actor *guard);
int watch_surrender(int guard);
float stolen_sale_chance(int town, int slot);
int stolen_sale(int town, int slot);
int town_npc_routine(int actor, float dt);
void actor_leave_room(int actor);
void put_down(Actor *carrier);
int nearest_ore(float x, float y, float reach, int type);
enum { CORE_SKILL_ROWS = SKILLS - 1 };
float context_distance(void);
float context_reach(void);
const char *context_name(void);
const char *context_action(void);
void spread_frontier(Game *game);
void place_wilderness_encounters(void);
extern const char *skillNames[SKILLS], *limbNames[LIMBS], *buildNames[], *jobNames[];
int level(const Actor *a, int skill);
void gain(Actor *a, int skill, float xp);
int inventory_count(const Actor *a, int item);
int inventory_add(Actor *a, int item, int qty);
int inventory_remove(Actor *a, int item, int qty);
int inventory_add_owned(Actor *a, int item, int qty, int owner);
int inventory_remove_slot(Actor *a, int slot, int qty);
int inventory_input_owner(const Actor *a, int item, int qty);
int inventory_transfer(Actor *from, Actor *to, int slot, int qty);
int pass_recipient(int row);
const char *pass_blocked(int actor);
int automatic_work(const Actor *a);
void work_notice(const Actor *a, const char *action, const char *target);
int wound_needs_dressing(const Actor *a);
float treatment_burden(const Actor *a);
int civilian_actor(const Actor *a);
int npc_last_resort(const Actor *a);
int npc_attacked(const Actor *a, const Actor *threat);
void raider_scraps(Actor *a, unsigned seed);
void dress_wounds(Actor *a);
int injury_needs_dressing(const Actor *a, const DepthActor *z);
void injury_dress(Actor *a, const DepthActor *z);
void injury_repair(Actor *a, const DepthActor *z);
int begin_treatment(int medic, int patient, int type, int slot);
int jail_authority(const Actor *guard, const Actor *patient);
void injury_knockout(Actor *a, float impact);
float coma_threshold(const Actor *a);
int recovery_coma(const Actor *a);
int fatal_injury(const Actor *a);
int injury_tick(Actor *a, DepthActor *z, float dt, int resting);
void roadside_aid_tick(float dt);
void roadside_aid_finished(int medic, int patient);
int jail_pick_start(int n);
int jail_bail(int n);
int jail_bail_cost(const Actor *a);
int jail_cell_open(const Actor *a);
void jail_exit(Actor *a);
void custody_tick(float dt);
int watch_collecting(int actor);
int conscious(const Actor *a);
int playing_dead(const Actor *a);
void faction_goodwill(int faction, int amount);
void trade_goodwill(int faction, int price);
int npc_sees(const Actor *observer, const Actor *target, float reach);
float npc_sight_range(const Actor *a);
int population_vendor(int n);
int road_trade(int item, int slot);
int market_quantity(int town, int item);
int slave_pick_start(int n);
int slave_camp_at(int building);
int slave_in_camp(const Actor *a);
enum { LOCK_SALVAGE, LOCK_CELL, LOCK_SHACKLES };
int lockpick_chance(const Actor *a, int lock);
float jail_pick_duration(const Actor *a, int tools);
void witnessed_theft(Actor *a, int owner, int range, int penalty);
int loot_take(int slot, int all);
int nearby_service(int role);
const char *job_status(const Actor *a);
const char *skill_training(int skill);
void recipe_status(int recipe, char *out, int size);
typedef struct {
    int output, qty, item[5], need[5], count, alternate, alternateNeed;
    int station, tech, smithing;
} CraftRequirements;
int recipe_requirements(int recipe, CraftRequirements *out);
int forge_requirements(int item, CraftRequirements *out);
int forge_selection_item(void);
const char *forge_selection_name(void);
int nearby_station(int type, float radius);
int build_wood_cost(int choice);
int research_cost(void);
int harvest_yield(const Actor *actor);
int needs_aid(const Actor *actor);
float encumbrance(const Actor *a);
float backpack_load_preview(const Actor *a, int item);
int backpack_load_percent(int item);
int backpack_combat_penalty(int item);
int item_effect_value(int item);
float movement_speed(const Actor *a);
const char *travel_hold_reason(const Actor *a);
float distance(float x, float y, float u, float v);
int can_walk(float x, float y);
int biome(float x, float y);
int town_near(float x, float y, float radius);
void game_new(uint32_t seed, int scenario);
void game_tick(float dt, Input in);
void simulate(float dt);
void move_actor(Actor *a, float dx, float dy, float dt);
int hostile(const Actor *a, const Actor *b);
void select_squad_member(int n);
void strike(int attacker, int defender);
void use_item(int slot);
int buy_item(int town, int item);
int sell_item(int town, int slot);
int recruit_actor(int index);
int build_structure(int choice);
int craft_item(int recipe);
int research_tech(void);
int heal_actor(int medic, int patient);
void interact(void);
void find_context(void);
void log_progress(const char *fmt, ...);
void log_level(const char *fmt, ...);
void log_msg(const char *fmt, ...);
void log_warn(const char *fmt, ...);
void log_tick(float dt);
const char *log_current(int *priority, int *offset);
int save_game(const char *path);
int load_game(const char *path);
int save_slot(int slot);
int load_slot(int slot);
void set_save_root(const char *path);
void render_game(uint32_t *pixels, int stride);
void render_title_art(uint32_t *pixels, int stride);
void debug_snapshot(const char *path);
extern const char *recipeNames[];
extern const int recipeCount, buildCount;

extern const char *factionNames[FACTIONS], *regionNames[9], *weatherNames[5];
extern const char *originNames[5], *raceNames[SPECIES], *effectNames[12], *materialNames[MATERIALS],
    *qualityNames[QUALITIES];
extern const char *familyNames[FAMILIES], *guideTitles[GUIDE_PAGES], *guideBodies[GUIDE_PAGES];
extern const uint32_t factionColors[FACTIONS];
void audio_event(int kind);
void audio_init(const char *eboot);
void audio_shutdown(void);
void audio_debug(unsigned stats[4]);
void content_init(void);
int gear_id(int family, int material, int quality);
int gear_family(int item);
int weapon_ranged(int item);
int weapon_rapier(int item);
int gear_bonus(const Actor *a, int stat);
int item_price(int town, int item, int selling);
int shop_count(void);
int shop_item(int index);
void init_expansion(void);
void tick_expansion(float dt);
int expansion_input(float dt, Input in);
void dialogue_open(int actor);
const char *dialogue_line(int actor, int page);
const char *ambient_line(int actor);
int forge_equipment(int item);
int storage_transfer(int building, int fromStorage, int slot);
int accept_contract(int town, int kind);
int contract_offer(int town, int row);
int journal_count(void);
int journal_slot(int row);
const Contract *journal_contract(int slot);
void remember_contract(const Contract *c);
int work_turn_in(int actor, int row);
int contract_reward(int town, int kind);
int contract_recipient(int slot);
const char *contract_title(int kind);
int contract_turn_in(int slot);
int contract_item_count(int item, int town);
int contract_use_items(int item, int qty, int town);
int contract_ready(int slot);
void contract_tick(void);
void contract_defeat(Actor *victim);
int contract_location(int slot, float *x, float *y);
int complete_contract(int index);
void contract_description(int index, char *dst, int size);
int pickpocket(int actor);
int pickpocket_slot(int actor, int slot);
float theft_chance(int actor, int slot);
int theft_witnesses(int actor);
int theft_behind(int actor);
int item_view_enabled(void);
int item_view_count(void);
int item_view_slot(int row);
int item_view_row(int slot);
void item_view_normalize(void);
void item_view_move(int direction);
void cycle_squad(int direction);
void skill_benefit(int skill, char *out, int size);
float pickpocket_chance(const Actor *actor);
int pickpocket_item(int actor);
int pause_menu_count(void);
const char *pause_menu_label(int row);
typedef struct { int town; const char *difficulty, *description; } StartScenario;
extern const StartScenario startScenarios[START_SCENARIOS];
void scenario_menu(int origin, int from);
void character_begin(int origin);
void reveal_area(Actor *a);
static inline float view_horizontal(float x, float y) { return x - y; }
static inline float view_vertical(float x, float y) { return x + y; }
static inline void view_to_world(float u, float v, float *x, float *y) {
    *x = (u + v) * .5f;
    *y = (v - u) * .5f;
}
void character_finish(void);
void starting_gear(Actor *a, int origin);
int save_inspect(int slot, SaveInfo *info);
void save_refresh(void);
int terrain_height(float x, float y);
int terrain_water(float x, float y);
int terrain_road(float x, float y);
void route_toward(Actor *a, float x, float y, float dt);
void walk_motion(Actor *a, float dx, float dy);
void walk_heading(const Actor *a, float *x, float *y);
int nearby_workstation(int type);
int start_sleep(int town);
int doctor_heal(int town);
void open_help(int topic);
int atlas_item(int row);
int atlas_count(void);
void atlas_select(int row);
void atlas_sync(void);
void render_catalog_scale(uint32_t *pixels, int stride, int item, int sc);
void render_catalog_sample(uint32_t *pixels, int stride, int item);
#include "depth.h"
#include "society.h"
#include "civilization.h"
#include "limbs.h"
#include "population.h"
#include "life.h"

void trail_init(void);
void trail_point(int site,float *x,float *y,float *dx,float *dy);
int trail_rock(int site,int rock,float *x,float *y);
int trail_blocked(float x,float y);
int trail_bed(int building);
int field_aid(int actor,float dt);
int combat_slot_available(int attacker,int target);
void recovery_hint(char *out,int size);

#endif
