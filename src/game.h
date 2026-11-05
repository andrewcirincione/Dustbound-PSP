#ifndef DUSTBOUND_GAME_H
#define DUSTBOUND_GAME_H
#include <stdint.h>
#define VERSION "0.5.1"
#define WORLD 1536
#define FOG_SIZE 384
#define WORK_REACH 1.15f
#define TALK_REACH 1.6f
#define MAX_ACTORS 192
#define MAX_BUILDINGS 320
#define MAX_NODES 640
#define PACK 24
#define SKILLS 20
#define LIMBS 6
#define BASE_ITEMS 36
#define FAMILIES 88
#define WEAPON_FAMILIES 36
#define MATERIALS 6
#define QUALITIES 4
#define ITEMS (BASE_ITEMS + FAMILIES * MATERIALS * QUALITIES)
#define GEARS 6
#define FACTIONS 8
#define CONTRACTS 24
#define GUIDE_PAGES 20
#define SAVE_SLOTS 7
#define TOWNS 16
#define SQUAD 8
#define LOGS 6
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
enum { HOUSE, SHOP, BAR, TOWER, WINDMILL, WELL, CAMPFIRE, BED, FARM, FORGE, BENCH, WALL, STORE };
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
    SCREEN_LOOT
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
        carriedBy, lostMask, prostheticMask, bounty, arrested, lootTaken;
    int body, skin, hair, hairColor, beard, scar, origin, race, facing, homeTown;
    int voice, speech, prisonTown, stolen[PACK], bountyReward;
    float speechTime, speechCooldown, prisonTime, stun, exposure, bleedTimer;
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
typedef struct {
    int exists, valid, version, day, squad, money;
    char name[16];
} SaveInfo;
typedef struct {
    uint32_t seed, rng;
    float time, dayTime, raidTimer, autosave;
    int day, money, selected, squadCount, tech, rep[FACTIONS], kills, discovered[TOWNS], stock[TOWNS][ITEMS],
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
} Game;
enum { ROOM_NONE, ROOM_TRADE, ROOM_REST, ROOM_FORGE, ROOM_RESEARCH, ROOM_STORAGE };
typedef struct {
    float x, y, w, d;
    int art, action;
} InteriorProp;
int building_has_interior(int type);
int interior_enter(int building);
void interior_leave(void);
int actor_inside(const Actor *a);
int interior_props(int building, InteriorProp *props);
int interior_context(void);
const char *interior_prompt(void);
void interior_interact(void);
void interior_move(float dx, float dy, float dt);
typedef struct {
    int screen, selection, scroll, tab, paused, speed, contextKind, contextIndex, tradeTown, sell, slot,
        confirmAction, titleChoice, waypoint;
    float mapX, mapY, camX, camY, noticeTime, repeat, travelX, travelY;
    char logs[LOGS][80], notice[100];
    int logCount;
    int helpReturn, helpTopic, dialogActor, dialogPage, forgeSlot, forgeFamily, forgeMaterial, forgeQuality;
    int createRow, createOrigin, createBody, createSkin, createHair, createHairColor, createBeard, createScar,
        createRace;
    int nameCursor, nameCell, storageIndex, storageSide, shopFilter, saveMode, lootActor, mapZoom, mapTown;
    char createName[16];
    SaveInfo saves[SAVE_SLOTS];
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
void world_layout(int expanded);
int world_is_expanded(void);
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
    ROLE_MERCENARY
};
int npc_role(const Actor *a);
int weapon_family_index(int i);
int weapon_strength(int item);
float weapon_handling(const Actor *a, int item);
void frontier_identity(void);
void frontier_restock(int town);
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
float encumbrance(const Actor *a);
float backpack_load_preview(const Actor *a, int item);
int backpack_load_percent(int item);
int backpack_combat_penalty(int item);
int item_effect_value(int item);
float movement_speed(const Actor *a);
float distance(float x, float y, float u, float v);
int can_walk(float x, float y);
int biome(float x, float y);
int town_near(float x, float y, float radius);
void game_new(uint32_t seed, int scenario);
void game_tick(float dt, Input in);
void simulate(float dt);
void move_actor(Actor *a, float dx, float dy, float dt);
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
void log_msg(const char *fmt, ...);
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
extern const char *originNames[5], *raceNames[3], *effectNames[12], *materialNames[MATERIALS],
    *qualityNames[QUALITIES];
extern const char *familyNames[FAMILIES], *guideTitles[GUIDE_PAGES], *guideBodies[GUIDE_PAGES];
extern const uint32_t factionColors[FACTIONS];
void audio_event(int kind);
void audio_init(void);
void audio_shutdown(void);
void content_init(void);
int gear_id(int family, int material, int quality);
int gear_family(int item);
int weapon_ranged(int item);
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
int complete_contract(int index);
void contract_description(int index, char *dst, int size);
int pickpocket(int actor);
void character_begin(int origin);
void character_finish(void);
int save_inspect(int slot, SaveInfo *info);
void save_refresh(void);
int terrain_height(float x, float y);
int terrain_water(float x, float y);
int terrain_road(float x, float y);
void route_toward(Actor *a, float x, float y, float dt);
int nearby_workstation(int type);
int start_sleep(int town);
int doctor_heal(int town);
void open_help(int topic);
void render_catalog_scale(uint32_t *pixels, int stride, int item, int sc);
void render_catalog_sample(uint32_t *pixels, int stride, int item);
#endif
