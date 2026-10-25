#ifndef RAPTURE_H
#define RAPTURE_H

#include <stdbool.h>
#include <stdint.h>

#define MAP_W 48
#define MAP_H 48
#define MAX_BUILDINGS 768
#define MAX_HOUSEHOLDS 640
#define MAX_FIRMS 384
#define MAX_LOGS 8
#define TECH_COUNT 20
#define OCEAN_EXPEDITION_MAX_CREW 8
#define SAVE_PATH "rapture_save.dat"
#define SAVE_SLOT_COUNT 5
#define OBSTACLE_CLUSTER_MAX 6
/* Seabed terrain: 0-4 are cosmetic variants, 10+ block construction and can be cleared. */
#define TERRAIN_OBSTACLE_ROCK 10
#define TERRAIN_OBSTACLE_FISSURE 11
#define IS_OBSTACLE_TERRAIN(t) ((t) >= TERRAIN_OBSTACLE_ROCK)

typedef enum GameMode { MODE_CAMPAIGN = 0, MODE_SANDBOX = 1 } GameMode;
typedef enum ScreenState { SCREEN_TITLE = 0, SCREEN_PLAYING = 1 } ScreenState;

typedef enum BuildingType {
    BT_CORRIDOR = 0,
    BT_RESIDENCE,
    BT_MARKET,
    BT_FACTORY,
    BT_LABORATORY,
    BT_POWER,
    BT_WATER,
    BT_OXYGEN,
    BT_FOOD,
    BT_HOSPITAL,
    BT_LEISURE,
    BT_BATHYSPHERE,
    BT_METRO,
    BT_AIRLOCK,
    BT_RECYCLING,
    BT_GENETICS,
    BT_SECURITY,
    BT_PIPE,
    BT_CINEMA,
    BT_RESTAURANT,
    BT_BOUTIQUE,
    BT_FISHERY,
    BT_ARCADIA,
    BT_HEPHAESTUS,
    BT_FORTRESS_FOLATRE,
    BT_GREENHOUSE_DOME,
    BT_RICH_RESIDENCE,
    BT_COUNT
} BuildingType;

typedef struct BuildingDef {
    const char *name;
    const char *description;
    int cost;
    int material_cost;
    int upkeep;
    int power_use;
    int jobs;
    int residents;
    int unlock_tech; /* -1 = available from start */
    bool is_company;
    uint8_t visual;
} BuildingDef;

typedef struct GridCell {
    int16_t building; /* index + 1; zero means empty */
    uint8_t terrain;
} GridCell;

typedef struct Building {
    bool active;
    bool closed;
    int x, y;
    BuildingType type;
    int workers;
    int residents;
    int condition;
    int firm_index; /* index + 1; zero means no firm */
    int footprint_w; /* grid width; persisted so older saves can keep 1x1 footprints */
    int footprint_h; /* grid height */
    int construction_days_remaining;
    int construction_days_total;
    float local_happiness;
} Building;

typedef struct Household {
    bool active;
    int home_building; /* index + 1 */
    int adults;
    int children;
    int employed_adults;
    int expedition_adults;
    int monthly_income;
    int savings;
    int happiness;
    int unemployed_months;
    int basic_needs_met;
    int social_satisfaction; /* service and leisure coverage, 0-100 */
} Household;

typedef struct Firm {
    bool active;
    int building_index; /* index + 1 */
    int owner_household; /* index + 1, zero = municipal / founder not yet selected */
    int cash;
    int monthly_revenue;
    int employees;
    int wage;
    int loss_months;
    int total_sales;
} Firm;

#define ATTRACT_MAX_TIPS 8

/* One thing the city lacks to attract more residents. */
typedef struct AttractTip {
    char text[112];
    int gain; /* attractiveness points the city would gain by fixing it */
} AttractTip;

typedef struct CityLog {
    char text[112];
    int severity; /* 0 info, 1 warning, 2 alert */
    int day_stamp;
    uint32_t serial; /* runtime-only sequence used to deliver every new notification */
    int building_index; /* the building this event is about, or -1. Set once, at the event's
                            own origin: never guessed afterwards from the message text, which
                            would match the first building of that type instead of the real one. */
} CityLog;

typedef struct GameState {
    GridCell map[MAP_W][MAP_H];
    Building buildings[MAX_BUILDINGS];
    Household households[MAX_HOUSEHOLDS];
    Firm firms[MAX_FIRMS];
    int building_slots;
    int household_slots;
    int firm_slots;

    int money;
    int research;
    int materials;
    int food_stock;
    int goods_stock;
    int tax_rate; /* personal income tax, percent */
    int company_tax_rate; /* company tax, percent */
    int housing_tax; /* monthly charge per household */
    int electricity_price; /* monthly utility tariff per resident */
    int water_price; /* monthly utility tariff per resident */
    bool expedition_active;
    int expedition_crew;
    int expedition_days_remaining;
    int expedition_risk;
    int day;
    int month;
    int year;
    int speed;
    int mode;
    bool paused;
    bool lost;
    bool won;
    bool show_grid;
    bool show_overlay;

    bool tech[TECH_COUNT];
    int tech_progress[TECH_COUNT];
    float power_ratio;
    float water_ratio;
    float oxygen_ratio;
    float food_ratio;
    float goods_ratio;
    float employment_ratio;
    float average_happiness;
    int population;
    int employed;
    int households_count;
    int available_jobs;
    int open_firms;
    int bankruptcies;
    int monthly_income;
    int monthly_expenses;
    int campaign_stage;
    int campaign_progress;

    CityLog logs[MAX_LOGS];
    int log_count;
    uint32_t rng;
    uint32_t notification_serial; /* incremented by Sim_AddLog for important UI alerts */
    int arrivals_households; /* runtime-only: arrivals since the last monthly report */
    int arrivals_people;

    /* Ocean obstacles (rock formations, fissures) block construction until cleared. */
    int obstacles_initial; /* total obstacle tiles generated at the start of the game */
    bool clearing_active;
    int clearing_tile_x[OBSTACLE_CLUSTER_MAX];
    int clearing_tile_y[OBSTACLE_CLUSTER_MAX];
    int clearing_tile_count;
    int clearing_days_remaining;
    int clearing_days_total;
    int clearing_workers;
} GameState;

extern const BuildingDef BUILDING_DEFS[BT_COUNT];
extern const char *TECH_NAMES[TECH_COUNT];
extern const char *TECH_DESCRIPTIONS[TECH_COUNT];
extern const int TECH_COST_MONEY[TECH_COUNT];
extern const int TECH_COST_POINTS[TECH_COUNT];
extern const unsigned int TECH_PREREQUISITES[TECH_COUNT];

void Sim_Init(GameState *g, GameMode mode);
void Sim_TickDay(GameState *g);
bool Sim_Build(GameState *g, BuildingType type, int x, int y, char *reason, int reason_size);
bool Sim_BuildOriented(GameState *g, BuildingType type, int x, int y, bool rotate, char *reason, int reason_size);
bool Sim_Demolish(GameState *g, int x, int y, char *reason, int reason_size);
bool Sim_Research(GameState *g, int tech_index, char *reason, int reason_size);
bool Sim_SendOceanExpedition(GameState *g, int crew, char *reason, int reason_size);
bool Sim_Save(const GameState *g, const char *path);
bool Sim_Load(GameState *g, const char *path);
void Sim_AddLog(GameState *g, int severity, const char *message);
void Sim_AddLogFor(GameState *g, int severity, int building_index, const char *message);
int Sim_CountObstacleTiles(const GameState *g);
bool Sim_StartObstacleClearing(GameState *g, int x, int y, char *reason, int reason_size);
int Sim_GetBuildingAt(const GameState *g, int x, int y);
void Sim_GetDefaultFootprint(BuildingType type, int *width, int *height);
int Sim_GetConstructionDays(BuildingType type);
int Sim_CountBuildings(const GameState *g, BuildingType type);
int Sim_CountAvailableHousing(const GameState *g);
int Sim_GetPopulation(const GameState *g);
int Sim_GetFirmCount(const GameState *g);
int Sim_GetJobsAvailable(const GameState *g);
int Sim_GetAttractiveness(const GameState *g, AttractTip *tips, int max_tips);
int Sim_GetCampaignGoal(const GameState *g, char *buffer, int buffer_size);
void Sim_UpdateStatistics(GameState *g);

#endif
