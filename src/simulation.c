#include "rapture.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

const BuildingDef BUILDING_DEFS[BT_COUNT] = {
    {"Corridor",       "Passages pressurises entre les quartiers.",             70,    0,   8, 0, 0,  0, -1, false, 0},
    {"Residence",      "Logements pour les familles de Rapture.",             1700,   2,  55, 2, 1, 22, -1, false, 1},
    {"Marche",         "Commerce de proximite et biens de consommation.",     2300,   3,  70, 3, 8,  0, -1, true,  2},
    {"Usine",          "Produit des biens et des materiaux industriels.",     3600,   5, 130, 6,14,  0, -1, true,  3},
    {"Laboratoire",    "Genere des points de recherche chaque mois.",         4900,   6, 170, 5,10,  0, -1, false, 4},
    {"Centrale",       "Fournit de l'electricite aux installations.",         5200,   7, 195, 3, 8,  0, -1, false, 5},
    {"Usine a eau",    "Traite et distribue l'eau potable.",                 3600,   4, 135, 4, 5,  0, -1, false, 6},
    {"Oxygenateur",    "Maintient l'atmosphere respirable.",                 4300,   5, 160, 5, 6,  0, -1, false, 7},
    {"Ferme marine",   "Produit la nourriture de la cite.",                  2700,   3,  80, 3, 7,  0, -1, true,  8},
    {"Hopital",        "Soins, prevention et secours.",                       5700,   7, 190, 6,10,  0, -1, false, 9},
    {"Salle de fete",  "Loisirs et vie culturelle.",                          4200,   5, 125, 5, 8,  0, -1, true, 10},
    {"Bathysphere",    "Relie Rapture aux points d'arrivee sous-marins.",     6500,   6, 155, 4, 5,  0,  0, true, 11},
    {"Metro",          "Transport rapide entre les quartiers.",              9300,   8, 230, 8, 8,  0,  1, true, 12},
    {"Sas oceanique",  "Permet les sorties en scaphandre dans l'ocean.",      4600,   5, 125, 3, 4,  0,  2, false,13},
    {"Recyclage",      "Recupere les dechets et economise les ressources.",  4900,   6, 130, 4, 8,  0,  6, false,14},
    {"Institut ADAM",  "Recherche genetique et traitements experimentaux.", 11800,  10, 290, 8,14,  0,  3, true, 15},
    {"Securite",       "Batiment de service non traversable : emploie 7 personnes, ameliore legerement le bonheur et reduit le risque des expeditions de 3 points par installation. Un corridor sert de passage ; Securite ne permet pas de traverser la ville.",     3900,   4, 100, 2, 7,  0, -1, false,16},
    {"Tuyau cuivre",   "Conduite auxiliaire : chaque tuyau au contact d'une centrale, station d'eau ou d'oxygene ajoute +8 % de capacite (max. +24 %). Ne remplace pas les corridors.", 35, 0, 4, 0, 0, 0, -1, false,17},
    {"Cinema",         "Projection de films : detente, culture et rencontres. Requiert la recherche Commerces sous-marins.", 4600, 5, 145, 4, 8, 0, 11, true, 18},
    {"Restaurant",     "Repas varies, vie sociale et emplois de service. Requiert la recherche Commerces sous-marins.", 3100, 4, 105, 3, 7, 0, 11, true, 19},
    {"Boutique",       "Articles du quotidien et biens de confort pour les habitants. Requiert la recherche Commerces sous-marins.", 2800, 3, 85, 2, 6, 0, 11, true, 20},
    {"Pecherie-fontaine", "Batiment remarquable unique : aquaculture intensive. Produit beaucoup de nourriture, mais consomme eau et oxygene. Requiert la recherche d'aquaculture et un sas ou une bathysphere operationnelle.", 8200, 8, 210, 7, 12, 0, 9, true, 21},
    {"Arcadie", "Jardin sous cloche de Rapture. Produit enormement d'oxygene, au prix d'une forte consommation d'eau. Requiert la recherche Biotechnologie marine et au moins trois fermes marines operationnelles.", 13800, 12, 390, 10, 18, 0, 16, false, 22},
    {"Pole d'Hephaistos", "Complexe metallurgique remarquable. Fournit une grande quantite d'electricite, en consommant beaucoup de materiaux chaque mois. Requiert la recherche Metallurgie avancee et trois usines operationnelles.", 15900, 14, 430, 9, 20, 0, 15, false, 23},
    {"Forteresse Folatre", "Palais de spectacles, salons et fetes. Ameliore fortement le divertissement et le moral, mais exige beaucoup d'energie et d'oxygene. Requiert la recherche Arts et spectacles et trois salles de fete operationnelles.", 14900, 12, 370, 12, 17, 0, 17, false, 24},
    {"Foret sous cloche", "Dome vitre abritant une foret cultivee. Produit davantage de nourriture qu'une ferme marine et ameliore legerement la satisfaction des quartiers voisins. Requiert la recherche Sylviculture sous cloche.", 6400, 7, 165, 4, 9, 0, 18, true, 25},
    {"Residence de prestige", "Logements de standing pour les familles les plus aisees de Rapture : moins d'occupants qu'une residence classique, mais un cadre de vie recherche. Requiert la recherche Urbanisme de prestige.", 5200, 4, 95, 3, 0, 10, 19, false, 26}
};

const char *TECH_NAMES[TECH_COUNT] = {
    "Reseau de bathyspheres", "Metro sous-marin", "Sas a pression", "Recherche genetique",
    "Support vital avance", "Production de masse", "Economie circulaire", "Contrat social",
    "Equipements sociaux", "Aquaculture oceanique", "Sous-marins d'exploration", "Commerces sous-marins",
    "Ingenierie des pressions", "Economie de marche", "Instrumentation scientifique", "Metallurgie avancee",
    "Biotechnologie marine", "Arts et spectacles", "Sylviculture sous cloche", "Urbanisme de prestige"
};
const char *TECH_DESCRIPTIONS[TECH_COUNT] = {
    "Debloque les terminaux de bathysphere et de nouvelles liaisons. Demande la maitrise des sas a pression.",
    "Debloque les stations de metro et augmente la mobilite.",
    "Debloque les sas pour les sorties en scaphandre. Premiere technologie de la branche oceanique.",
    "Debloque l'Institut ADAM et ses recherches controverses. S'appuie sur le support vital avance.",
    "Ameliore l'efficacite de l'eau et de l'oxygene de 25 %. Demande l'instrumentation scientifique.",
    "Les usines produisent davantage avec moins de materiaux. Demande l'ingenierie des pressions.",
    "Debloque le recyclage et reduit les couts d'entretien.",
    "Ameliore la satisfaction et attenue l'effet des taxes. Base de la branche sociale.",
    "Ameliore le tissu social et renforce les effets des services collectifs existants.",
    "Debloque la Pecherie-fontaine unique : il faut un sas ou une bathysphere operationnelle pour acceder a la mer. Demande le support vital avance.",
    "Debloque des sous-marins d'exploration : les expeditions reviennent beaucoup plus vite, avec un equipage mieux protege.",
    "Debloque les Boutiques, Restaurants et Cinemas. Ces commerces creent des emplois, generent des recettes et repondent aux besoins de biens, de repas et de loisirs des habitants. S'appuie sur l'economie de marche.",
    "Maitrise des structures sous pression : l'usure des batiments est reduite d'environ 40 %. Base de la branche industrielle.",
    "Marches organises : toutes les entreprises gagnent environ 8 % de recettes en plus. Prerequis des commerces sous-marins.",
    "Capteurs et appareils de mesure : les laboratoires et l'Institut ADAM produisent environ 20 % de recherche en plus. Base de la branche scientifique.",
    "Fonderies de precision : chaque usine fournit 2 materiaux de plus par mois. Debloque le Pole d'Hephaistos.",
    "Cultures sous cloche et selection marine : les fermes produisent environ 18 % de nourriture en plus. Debloque l'Arcadie.",
    "Troupes, salons et grandes fetes : les loisirs apportent davantage de moral. Debloque la Forteresse Folatre. Demande les equipements sociaux et les commerces.",
    "Cultures forestieres sous cloche de verre : debloque la Foret sous cloche, qui produit plus de nourriture qu'une ferme marine. Demande la biotechnologie marine.",
    "Quartiers residentiels de standing et fiscalite adaptee : debloque la Residence de prestige. Demande le contrat social et les commerces sous-marins."
};
const int TECH_COST_MONEY[TECH_COUNT] = {5000, 9000, 6000, 16000, 8500, 12000, 9000, 7000, 7600, 9800, 14500, 7600,
                                         4500, 6000, 5500, 11000, 12500, 10500, 10800, 9200};
const int TECH_COST_POINTS[TECH_COUNT] = {28, 58, 42, 95, 50, 68, 56, 45, 48, 58, 82, 48,
                                          24, 32, 30, 70, 74, 62, 66, 54};
/* Bit n means that technology n must be researched first.
   Industry : Ingenierie(12) > Production(5) > Economie circulaire(6) > Metallurgie(15)
   Science  : Instrumentation(14) > Support vital(4) > Genetique(3), Aquaculture(9) > Biotechnologie(16)
   Society  : Contrat social(7) > Equipements sociaux(8) > Arts et spectacles(17, also needs Commerces)
   Commerce : Economie de marche(13) > Commerces(11) > Arts et spectacles(17)
   Ocean    : Sas(2) > Bathyspheres(0) > Metro(1), Sas(2) > Sous-marins(10) */
const unsigned int TECH_PREREQUISITES[TECH_COUNT] = {
    (1u << 2), (1u << 0), 0u, (1u << 4), (1u << 14), (1u << 12), (1u << 5), 0u, (1u << 7), (1u << 4), (1u << 2), (1u << 13),
    0u, 0u, 0u, (1u << 6), (1u << 9), (1u << 8) | (1u << 11),
    (1u << 16), (1u << 7) | (1u << 11)
};

static int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static int min_i(int a, int b) { return a < b ? a : b; }
static int max_i(int a, int b) { return a > b ? a : b; }
static uint32_t rnd(GameState *g) {
    uint32_t x = g->rng ? g->rng : 0xA341316Cu;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    g->rng = x;
    return x;
}
static int randi(GameState *g, int max_exclusive) { return max_exclusive > 0 ? (int)(rnd(g) % (uint32_t)max_exclusive) : 0; }

void Sim_GetDefaultFootprint(BuildingType type, int *width, int *height)
{
    int w = 1, h = 1;
    switch (type) {
        case BT_FACTORY: case BT_GENETICS: case BT_FISHERY:
        case BT_ARCADIA: case BT_HEPHAESTUS: case BT_FORTRESS_FOLATRE: case BT_GREENHOUSE_DOME: w = 2; h = 2; break;
        case BT_FOOD: case BT_LABORATORY: case BT_POWER: case BT_HOSPITAL:
        case BT_METRO: case BT_RECYCLING: case BT_CINEMA: case BT_RESTAURANT: case BT_RICH_RESIDENCE: w = 2; h = 1; break;
        default: break;
    }
    if (width) *width = w;
    if (height) *height = h;
}

int Sim_GetConstructionDays(BuildingType type)
{
    switch (type) {
        case BT_CORRIDOR: case BT_PIPE: return 2;
        case BT_FACTORY: case BT_GENETICS: return 10;
        case BT_FISHERY: return 12;
        case BT_ARCADIA: case BT_HEPHAESTUS: return 16;
        case BT_FORTRESS_FOLATRE: return 14;
        case BT_POWER: case BT_WATER: case BT_OXYGEN: case BT_HOSPITAL: return 8;
        default: return 7;
    }
}

static bool building_ready(const Building *b)
{
    return b && b->active && b->construction_days_remaining <= 0;
}

static bool is_residential(BuildingType t) { return t == BT_RESIDENCE || t == BT_RICH_RESIDENCE; }

static bool footprint_is_clear(const GameState *g, int x, int y, int w, int h)
{
    if (!g || w < 1 || h < 1 || x < 0 || y < 0 || x + w > MAP_W || y + h > MAP_H) return false;
    for (int ix = x; ix < x + w; ++ix)
        for (int iy = y; iy < y + h; ++iy) {
            if (g->map[ix][iy].building != 0) return false;
            if (IS_OBSTACLE_TERRAIN(g->map[ix][iy].terrain)) return false;
        }
    return true;
}

void Sim_AddLogFor(GameState *g, int severity, int building_index, const char *message)
{
    if (!g || !message) return;
    if (g->log_count < MAX_LOGS) g->log_count++;
    for (int i = g->log_count - 1; i > 0; --i) g->logs[i] = g->logs[i - 1];
    (void)snprintf(g->logs[0].text, sizeof(g->logs[0].text), "%s", message);
    g->logs[0].severity = clampi(severity, 0, 2);
    g->notification_serial++;
    g->logs[0].serial = g->notification_serial;
    g->logs[0].day_stamp = g->year * 360 + g->month * 30 + g->day;
    g->logs[0].building_index = (building_index >= 0 && building_index < g->building_slots &&
                                  g->buildings[building_index].active) ? building_index : -1;
}

void Sim_AddLog(GameState *g, int severity, const char *message) { Sim_AddLogFor(g, severity, -1, message); }

int Sim_GetBuildingAt(const GameState *g, int x, int y)
{
    if (!g || x < 0 || y < 0 || x >= MAP_W || y >= MAP_H) return -1;
    int id = (int)g->map[x][y].building;
    if (id <= 0 || id > g->building_slots || !g->buildings[id - 1].active) return -1;
    return id - 1;
}

int Sim_CountBuildings(const GameState *g, BuildingType type)
{
    int n = 0;
    for (int i = 0; i < g->building_slots; ++i)
        if (g->buildings[i].active && g->buildings[i].type == type) n++;
    return n;
}

static int count_operational_buildings(const GameState *g, BuildingType type)
{
    int n = 0;
    for (int i = 0; i < g->building_slots; ++i)
        if (building_ready(&g->buildings[i]) && !g->buildings[i].closed && g->buildings[i].type == type) n++;
    return n;
}

static bool has_operational_sea_access(const GameState *g)
{
    return count_operational_buildings(g, BT_AIRLOCK) > 0 ||
           count_operational_buildings(g, BT_BATHYSPHERE) > 0;
}

static bool is_unique_landmark(BuildingType type)
{
    return type == BT_FISHERY || type == BT_ARCADIA ||
           type == BT_HEPHAESTUS || type == BT_FORTRESS_FOLATRE;
}

/* Ocean obstacles: rock formations and fissures scattered outside the starting city
   footprint. They block construction until a team is sent to clear them (see
   Sim_StartObstacleClearing); the terrain value alone marks a tile as an obstacle, so no
   separate per-tile bookkeeping is needed once they are generated. */
static bool tile_in_city_safezone(int x, int y)
{
    /* Generous margin around the starting cross-shaped spine and its service buildings. */
    return x >= 9 && x <= 39 && y >= 11 && y <= 37;
}

static bool tile_in_large_obstacle_safezone(int x, int y)
{
    /* A wider margin reserved for big formations (3-4 tile fissures, 2x2 rocks), so they
       land further out toward the map's border rather than merely outside the city. */
    return x >= 4 && x <= 43 && y >= 6 && y <= 41;
}

static bool tiles_free(const GameState *g, const int *txs, const int *tys, int count, bool large)
{
    for (int i = 0; i < count; ++i) {
        int x = txs[i], y = tys[i];
        if (x < 0 || y < 0 || x >= MAP_W || y >= MAP_H) return false;
        if (large ? tile_in_large_obstacle_safezone(x, y) : tile_in_city_safezone(x, y)) return false;
        if (g->map[x][y].building != 0 || IS_OBSTACLE_TERRAIN(g->map[x][y].terrain)) return false;
    }
    return true;
}

static int place_tiles(GameState *g, const int *txs, const int *tys, int count, uint8_t kind)
{
    for (int i = 0; i < count; ++i) g->map[txs[i]][tys[i]].terrain = kind;
    return count;
}

static void generate_ocean_obstacles(GameState *g)
{
    int clusters = 13 + (int)randi(g, 6); /* 13 to 18 formations */
    int placed_tiles = 0;
    for (int c = 0; c < clusters; ++c) {
        /* About a third of formations are large: a long fissure (3-4 tiles) or a 2x2 rock
           block, kept toward the outer band of the map; the rest are the usual small ones. */
        bool want_large = randi(g, 100) < 34;
        uint8_t kind = (randi(g, 100) < 55) ? TERRAIN_OBSTACLE_ROCK : TERRAIN_OBSTACLE_FISSURE;
        bool placed = false;
        for (int attempt = 0; attempt < 140 && !placed; ++attempt) {
            int x = 1 + randi(g, MAP_W - 2);
            int y = 1 + randi(g, MAP_H - 2);
            int txs[4], tys[4];
            int count;
            if (kind == TERRAIN_OBSTACLE_ROCK && want_large) {
                /* A 2x2 boulder field. */
                count = 4;
                txs[0] = x; tys[0] = y; txs[1] = x + 1; tys[1] = y;
                txs[2] = x; tys[2] = y + 1; txs[3] = x + 1; tys[3] = y + 1;
            } else if (kind == TERRAIN_OBSTACLE_FISSURE) {
                /* A straight crack, long when "large", short otherwise, along a random axis. */
                count = want_large ? 3 + (int)randi(g, 2) : 1 + (int)randi(g, 2);
                bool horizontal = randi(g, 2) == 0;
                for (int i = 0; i < count; ++i) {
                    txs[i] = horizontal ? x + i : x;
                    tys[i] = horizontal ? y : y + i;
                }
            } else {
                /* A small rock: a single tile or a short two-tile outcrop. */
                count = 1 + randi(g, 2);
                bool horizontal = randi(g, 2) == 0;
                for (int i = 0; i < count; ++i) {
                    txs[i] = horizontal ? x + i : x;
                    tys[i] = horizontal ? y : y + i;
                }
            }
            if (!tiles_free(g, txs, tys, count, want_large)) continue;
            placed_tiles += place_tiles(g, txs, tys, count, kind);
            placed = true;
        }
        if (!placed && want_large) {
            /* The outer band was full: retry once as a small formation so the obstacle
               count stays close to what was promised. */
            for (int attempt = 0; attempt < 60 && !placed; ++attempt) {
                int x = 1 + randi(g, MAP_W - 2);
                int y = 1 + randi(g, MAP_H - 2);
                int txs[2] = {x, x}, tys[2] = {y, y};
                int count = 1;
                if (!tiles_free(g, txs, tys, count, false)) continue;
                placed_tiles += place_tiles(g, txs, tys, count, kind);
                placed = true;
            }
        }
    }
    g->obstacles_initial = placed_tiles;
}

int Sim_CountObstacleTiles(const GameState *g)
{
    if (!g) return 0;
    int n = 0;
    for (int x = 0; x < MAP_W; ++x) for (int y = 0; y < MAP_H; ++y)
        if (IS_OBSTACLE_TERRAIN(g->map[x][y].terrain)) n++;
    return n;
}

bool Sim_StartObstacleClearing(GameState *g, int x, int y, char *reason, int reason_size)
{
    if (!g) return false;
    if (g->clearing_active) {
        if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Une equipe degage deja un obstacle.");
        return false;
    }
    if (x < 0 || y < 0 || x >= MAP_W || y >= MAP_H || !IS_OBSTACLE_TERRAIN(g->map[x][y].terrain)) {
        if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Aucun obstacle sur cette case.");
        return false;
    }
    /* Flood-fill the contiguous obstacle so one click clears a whole formation. */
    uint8_t kind = g->map[x][y].terrain;
    int tx[OBSTACLE_CLUSTER_MAX], ty[OBSTACLE_CLUSTER_MAX];
    int count = 0;
    int qx[OBSTACLE_CLUSTER_MAX], qy[OBSTACLE_CLUSTER_MAX];
    int qn = 0;
    qx[qn] = x; qy[qn] = y; qn++;
    tx[count] = x; ty[count] = y; count++;
    for (int head = 0; head < qn && count < OBSTACLE_CLUSTER_MAX; ++head) {
        const int dx[4] = {1, -1, 0, 0}, dy[4] = {0, 0, 1, -1};
        for (int d = 0; d < 4 && count < OBSTACLE_CLUSTER_MAX; ++d) {
            int nx = qx[head] + dx[d], ny = qy[head] + dy[d];
            if (nx < 0 || ny < 0 || nx >= MAP_W || ny >= MAP_H) continue;
            if (g->map[nx][ny].terrain != kind) continue;
            bool already = false;
            for (int k = 0; k < count; ++k) if (tx[k] == nx && ty[k] == ny) { already = true; break; }
            if (already) continue;
            tx[count] = nx; ty[count] = ny; count++;
            if (qn < OBSTACLE_CLUSTER_MAX) { qx[qn] = nx; qy[qn] = ny; qn++; }
        }
    }
    int free_workers = 0;
    for (int i = 0; i < g->household_slots; ++i) {
        const Household *h = &g->households[i];
        if (h->active) free_workers += max_i(0, h->adults - h->expedition_adults);
    }
    int needed = max_i(2, count);
    if (free_workers < needed) {
        if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Il faut %d ouvriers disponibles ; seuls %d sont libres.", needed, free_workers);
        return false;
    }
    int cost = 380 * count;
    if (g->money < cost) {
        if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Budget insuffisant : le chantier de degagement coute %d $.", cost);
        return false;
    }
    g->money -= cost;
    g->clearing_active = true;
    g->clearing_tile_count = count;
    for (int k = 0; k < count; ++k) { g->clearing_tile_x[k] = tx[k]; g->clearing_tile_y[k] = ty[k]; }
    g->clearing_workers = needed;
    g->clearing_days_total = 3 + count * 2 + (kind == TERRAIN_OBSTACLE_FISSURE ? 2 : 0);
    g->clearing_days_remaining = g->clearing_days_total;
    if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Equipe envoyee : %d ouvriers, %d jours estimes.", needed, g->clearing_days_total);
    Sim_AddLog(g, 0, "Une equipe degage un obstacle rocheux pour agrandir le terrain constructible.");
    return true;
}

int Sim_CountAvailableHousing(const GameState *g)
{
    int capacity = 0;
    int occupied = 0;
    for (int i = 0; i < g->building_slots; ++i) {
        const Building *b = &g->buildings[i];
        if (building_ready(b) && is_residential(b->type)) {
            capacity += BUILDING_DEFS[b->type].residents;
            occupied += b->residents;
        }
    }
    return max_i(0, capacity - occupied);
}

int Sim_GetPopulation(const GameState *g)
{
    int p = 0;
    for (int i = 0; i < g->household_slots; ++i)
        if (g->households[i].active) p += g->households[i].adults + g->households[i].children;
    return p;
}

int Sim_GetFirmCount(const GameState *g)
{
    int n = 0;
    for (int i = 0; i < g->firm_slots; ++i) if (g->firms[i].active) n++;
    return n;
}

static int firm_for_building(const GameState *g, int building_index)
{
    int id = g->buildings[building_index].firm_index;
    if (id <= 0 || id > g->firm_slots || !g->firms[id - 1].active) return -1;
    return id - 1;
}

static int company_job_capacity(const GameState *g, BuildingType type)
{
    int pop = g->population;
    switch (type) {
        case BT_MARKET: return min_i(BUILDING_DEFS[type].jobs, 3 + pop / 85);
        case BT_FACTORY: return min_i(BUILDING_DEFS[type].jobs, 4 + pop / 95);
        case BT_FOOD: return min_i(BUILDING_DEFS[type].jobs, 3 + pop / 90);
        case BT_LEISURE: return min_i(BUILDING_DEFS[type].jobs, 2 + pop / 110);
        case BT_BATHYSPHERE: return min_i(BUILDING_DEFS[type].jobs, 2 + pop / 130);
        case BT_METRO: return min_i(BUILDING_DEFS[type].jobs, 3 + pop / 100);
        case BT_GENETICS: return min_i(BUILDING_DEFS[type].jobs, 5 + pop / 100);
        case BT_CINEMA: return min_i(BUILDING_DEFS[type].jobs, 2 + pop / 95);
        case BT_RESTAURANT: return min_i(BUILDING_DEFS[type].jobs, 3 + pop / 90);
        case BT_BOUTIQUE: return min_i(BUILDING_DEFS[type].jobs, 2 + pop / 80);
        case BT_FISHERY: return min_i(BUILDING_DEFS[type].jobs, 4 + pop / 100);
        default: return BUILDING_DEFS[type].jobs;
    }
}

static int count_techs(const GameState *g)
{
    int n = 0;
    for (int i = 0; i < TECH_COUNT; ++i) if (g->tech[i]) n++;
    return n;
}

static int add_building_with_footprint(GameState *g, BuildingType type, int x, int y, int w, int h)
{
    if (!footprint_is_clear(g, x, y, w, h)) return -1;
    int slot = -1;
    for (int i = 0; i < g->building_slots; ++i) if (!g->buildings[i].active) { slot = i; break; }
    if (slot < 0) {
        if (g->building_slots >= MAX_BUILDINGS) return -1;
        slot = g->building_slots++;
    }
    Building *b = &g->buildings[slot];
    memset(b, 0, sizeof(*b));
    b->active = true;
    b->x = x; b->y = y; b->type = type; b->condition = 100; b->local_happiness = 50.0f;
    b->footprint_w = w; b->footprint_h = h;
    for (int ix = x; ix < x + w; ++ix)
        for (int iy = y; iy < y + h; ++iy) {
            g->map[ix][iy].building = (int16_t)(slot + 1);
            g->map[ix][iy].terrain = 0;
        }
    return slot;
}

static int add_building_internal(GameState *g, BuildingType type, int x, int y)
{
    int w = 1, h = 1;
    Sim_GetDefaultFootprint(type, &w, &h);
    return add_building_with_footprint(g, type, x, y, w, h);
}

static int choose_owner(GameState *g)
{
    int best = -1;
    int best_savings = 900;
    for (int i = 0; i < g->household_slots; ++i) {
        Household *h = &g->households[i];
        if (h->active && h->savings > best_savings) { best = i; best_savings = h->savings; }
    }
    if (best >= 0 && best_savings > 1800 && randi(g, 100) < 70) return best + 1;
    return 0;
}

static int create_firm(GameState *g, int building_index, int owner_id)
{
    int slot = -1;
    for (int i = 0; i < g->firm_slots; ++i) if (!g->firms[i].active) { slot = i; break; }
    if (slot < 0) {
        if (g->firm_slots >= MAX_FIRMS) return -1;
        slot = g->firm_slots++;
    }
    Firm *f = &g->firms[slot];
    memset(f, 0, sizeof(*f));
    f->active = true;
    f->building_index = building_index + 1;
    f->owner_household = owner_id;
    f->cash = 3200;
    f->wage = 260;
    switch (g->buildings[building_index].type) {
        case BT_MARKET: f->wage = 255; break;
        case BT_FACTORY: f->wage = 315; break;
        case BT_FOOD: case BT_FISHERY: case BT_GREENHOUSE_DOME: f->wage = 245; break;
        case BT_LEISURE: case BT_CINEMA: case BT_RESTAURANT: f->wage = 250; break;
        case BT_BOUTIQUE: f->wage = 235; break;
        case BT_BATHYSPHERE: f->wage = 240; break;
        case BT_METRO: f->wage = 260; break;
        case BT_GENETICS: f->wage = 330; break;
        default: break;
    }
    g->buildings[building_index].firm_index = slot + 1;
    return slot;
}

static int create_household(GameState *g, int home_building, int adults, int children, int savings)
{
    int slot = -1;
    for (int i = 0; i < g->household_slots; ++i) if (!g->households[i].active) { slot = i; break; }
    if (slot < 0) {
        if (g->household_slots >= MAX_HOUSEHOLDS) return -1;
        slot = g->household_slots++;
    }
    Household *h = &g->households[slot];
    memset(h, 0, sizeof(*h));
    h->active = true;
    h->home_building = home_building + 1;
    h->adults = adults;
    h->children = children;
    h->savings = savings;
    h->happiness = 57;
    h->basic_needs_met = 1;
    if (home_building >= 0 && home_building < g->building_slots) g->buildings[home_building].residents += adults + children;
    return slot;
}

static bool has_neighboring_city(const GameState *g, int x, int y, int distance)
{
    for (int bx = max_i(0, x - distance); bx <= min_i(MAP_W - 1, x + distance); ++bx) {
        for (int by = max_i(0, y - distance); by <= min_i(MAP_H - 1, y + distance); ++by) {
            if (abs(bx - x) + abs(by - y) > distance) continue;
            if (Sim_GetBuildingAt(g, bx, by) >= 0) return true;
        }
    }
    return false;
}

static bool has_neighboring_city_footprint(const GameState *g, int x, int y, int w, int h, int distance)
{
    for (int ix = x; ix < x + w; ++ix) {
        for (int iy = y; iy < y + h; ++iy) {
            if (has_neighboring_city(g, ix, iy, distance)) return true;
        }
    }
    return false;
}

static void recalc_building_residents(GameState *g)
{
    for (int i = 0; i < g->building_slots; ++i) g->buildings[i].residents = 0;
    for (int i = 0; i < g->household_slots; ++i) {
        Household *h = &g->households[i];
        if (!h->active) continue;
        int home = h->home_building - 1;
        if (home >= 0 && home < g->building_slots && g->buildings[home].active)
            g->buildings[home].residents += h->adults + h->children;
    }
}

static int adjacent_copper_pipe_count(const GameState *g, const Building *b)
{
    if (!g || !b) return 0;
    int fw = b->footprint_w > 0 ? b->footprint_w : 1;
    int fh = b->footprint_h > 0 ? b->footprint_h : 1;
    int count = 0;
    for (int x = b->x; x < b->x + fw; ++x) {
        int coords[2][2] = {{x, b->y - 1}, {x, b->y + fh}};
        for (int k = 0; k < 2; ++k) {
            int px = coords[k][0], py = coords[k][1];
            int bi = Sim_GetBuildingAt(g, px, py);
            if (bi >= 0 && bi < g->building_slots && g->buildings[bi].type == BT_PIPE) count++;
        }
    }
    for (int y = b->y; y < b->y + fh; ++y) {
        int coords[2][2] = {{b->x - 1, y}, {b->x + fw, y}};
        for (int k = 0; k < 2; ++k) {
            int px = coords[k][0], py = coords[k][1];
            int bi = Sim_GetBuildingAt(g, px, py);
            if (bi >= 0 && bi < g->building_slots && g->buildings[bi].type == BT_PIPE) count++;
        }
    }
    return min_i(count, 3);
}

static void recalculate_utilities(GameState *g)
{
    float power = 0.0f, water = 0.0f, oxygen = 0.0f;
    float power_need = 4.0f, water_need = 9.0f, oxygen_need = 8.0f;
    int population = Sim_GetPopulation(g);
    for (int i = 0; i < g->building_slots; ++i) {
        const Building *b = &g->buildings[i];
        if (!building_ready(b) || b->closed) continue;
        power_need += (float)BUILDING_DEFS[b->type].power_use * (0.55f + (float)b->condition / 220.0f);
        water_need += 0.42f + (float)BUILDING_DEFS[b->type].power_use * 0.045f;
        oxygen_need += 0.38f + (float)BUILDING_DEFS[b->type].power_use * 0.035f;
        float distribution_bonus = 1.0f + 0.08f * (float)adjacent_copper_pipe_count(g, b);
        if (b->type == BT_POWER) power += 120.0f * (float)b->condition / 100.0f * distribution_bonus;
        if (b->type == BT_WATER) water += 78.0f * (float)b->condition / 100.0f * distribution_bonus;
        if (b->type == BT_OXYGEN) oxygen += 83.0f * (float)b->condition / 100.0f * distribution_bonus;
        if (b->type == BT_ARCADIA) {
            oxygen += 195.0f * (float)b->condition / 100.0f;
            water_need += 28.0f; /* irrigation for the enclosed gardens */
        }
        if (b->type == BT_HEPHAESTUS) {
            /* Hephaestus consumes stored materials; an empty foundry keeps only
               a small emergency output rather than collapsing instantly. */
            float material_factor = g->materials >= 6 ? 1.0f : (g->materials > 0 ? 0.48f : 0.18f);
            power += 235.0f * (float)b->condition / 100.0f * material_factor;
        }
        if (b->type == BT_FISHERY) {
            water_need += 17.0f;
            oxygen_need += 14.0f;
        }
        if (b->type == BT_GREENHOUSE_DOME) water_need += 10.0f;
        if (b->type == BT_FORTRESS_FOLATRE) oxygen_need += 13.0f;
    }
    /* Utilities scale with residents, not just the number of buildings. One
       installation comfortably supports the opening district, but expansion
       eventually requires another treatment/oxygen plant or advanced science. */
    water_need += (float)population * 1.25f;
    oxygen_need += (float)population * 1.30f;
    power_need += (float)population * 0.28f;
    g->power_ratio = clampf(power / (float)max_i(1, (int)ceilf(power_need)), 0.0f, 1.0f);
    /* Treatment and electrolysis require electricity. Preserve a small emergency
       output to avoid an instantaneous game-over after a short blackout. */
    float utility_power_factor = 0.18f + 0.82f * g->power_ratio;
    water *= utility_power_factor;
    oxygen *= utility_power_factor;
    if (g->tech[4]) { water *= 1.25f; oxygen *= 1.25f; }
    g->water_ratio = clampf(water / (float)max_i(1, (int)ceilf(water_need)), 0.0f, 1.0f);
    g->oxygen_ratio = clampf(oxygen / (float)max_i(1, (int)ceilf(oxygen_need)), 0.0f, 1.0f);
    float food_need = (float)max_i(8, population) * 0.58f;
    float goods_need = (float)max_i(8, population) * 0.66f;
    g->food_ratio = clampf((float)g->food_stock / (float)max_i(1, (int)ceilf(food_need)), 0.0f, 1.0f);
    g->goods_ratio = clampf((float)g->goods_stock / (float)max_i(1, (int)ceilf(goods_need)), 0.0f, 1.0f);
}

void Sim_UpdateStatistics(GameState *g)
{
    if (!g) return;
    g->population = 0; g->households_count = 0; g->employed = 0; g->available_jobs = 0;
    g->open_firms = 0; g->bankruptcies = 0;
    int adult_total = 0, happiness_total = 0, happiness_homes = 0;
    for (int i = 0; i < g->household_slots; ++i) {
        const Household *h = &g->households[i];
        if (!h->active) continue;
        g->households_count++;
        g->population += h->adults + h->children;
        adult_total += max_i(0, h->adults - h->expedition_adults);
        g->employed += h->employed_adults;
        happiness_total += h->happiness;
        happiness_homes++;
    }
    g->average_happiness = happiness_homes ? (float)happiness_total / (float)happiness_homes : 50.0f;
    g->employment_ratio = adult_total ? clampf((float)g->employed / (float)adult_total, 0.0f, 1.0f) : 1.0f;
    for (int i = 0; i < g->firm_slots; ++i) {
        const Firm *firm = &g->firms[i];
        if (firm->active) {
            int bi = firm->building_index - 1;
            if (bi >= 0 && bi < g->building_slots && building_ready(&g->buildings[bi]) &&
                !g->buildings[bi].closed) g->open_firms++;
        } else if (firm->building_index > 0) {
            g->bankruptcies++;
        }
    }
    for (int i = 0; i < g->building_slots; ++i) {
        const Building *b = &g->buildings[i];
        if (!building_ready(b) || b->closed) continue;
        int jobs = BUILDING_DEFS[b->type].is_company ? 0 : BUILDING_DEFS[b->type].jobs;
        if (BUILDING_DEFS[b->type].is_company) {
            int fi = firm_for_building(g, i);
            if (fi >= 0) jobs = company_job_capacity(g, b->type);
        }
        g->available_jobs += max_i(0, jobs - b->workers);
    }
    recalculate_utilities(g);
}

int Sim_GetJobsAvailable(const GameState *g) { return g ? g->available_jobs : 0; }

static void assign_jobs_ex(GameState *g, bool count_unemployment)
{
    for (int i = 0; i < g->building_slots; ++i) g->buildings[i].workers = 0;
    for (int i = 0; i < g->firm_slots; ++i) if (g->firms[i].active) g->firms[i].employees = 0;
    for (int i = 0; i < g->household_slots; ++i) {
        Household *h = &g->households[i];
        if (h->active) { h->employed_adults = 0; h->monthly_income = 0; }
    }

    for (int hi = 0; hi < g->household_slots; ++hi) {
        Household *h = &g->households[hi];
        if (!h->active) continue;
        for (int adult = 0; adult < h->adults - h->expedition_adults; ++adult) {
            int chosen = -1;
            int chosen_wage = -1;
            int chosen_fill = 0;
            /* Workers spread over every employer: take the one whose staff is the least
               full (as a share of its capacity), then the best wage. Picking the highest
               wage first sent every worker to the foundry and left other firms empty. */
            for (int bi = 0; bi < g->building_slots; ++bi) {
                Building *b = &g->buildings[bi];
                if (!building_ready(b) || b->closed) continue;
                int capacity = 0, wage = 0;
                int fi = firm_for_building(g, bi);
                if (BUILDING_DEFS[b->type].is_company) {
                    if (fi < 0) continue;
                    capacity = company_job_capacity(g, b->type);
                    wage = g->firms[fi].wage;
                } else {
                    capacity = BUILDING_DEFS[b->type].jobs;
                    wage = 225;
                }
                if (capacity <= 0 || b->workers >= capacity) continue;
                int fill = b->workers * 1000 / capacity;
                if (chosen < 0 || fill < chosen_fill || (fill == chosen_fill && wage > chosen_wage)) {
                    chosen = bi; chosen_wage = wage; chosen_fill = fill;
                }
            }
            if (chosen < 0) break;
            Building *b = &g->buildings[chosen];
            b->workers++;
            int fi = firm_for_building(g, chosen);
            if (fi >= 0) g->firms[fi].employees++;
            h->employed_adults++;
            h->monthly_income += chosen_wage;
        }
        if (count_unemployment) {
            if (h->employed_adults < h->adults - h->expedition_adults) h->unemployed_months++;
            else h->unemployed_months = 0;
        }
    }
}

static void assign_jobs(GameState *g) { assign_jobs_ex(g, true); }

static int company_base_revenue(const GameState *g, BuildingType type)
{
    int pop = g->population;
    int revenue = 0;
    switch (type) {
        case BT_MARKET: revenue = 700 + pop * 35; break;
        case BT_FACTORY: revenue = 1050 + pop * 19; break;
        case BT_FOOD: revenue = 760 + pop * 27; break;
        case BT_LEISURE: revenue = 140 + pop * 16; break;
        case BT_BATHYSPHERE: revenue = 120 + pop * 10; break;
        case BT_METRO: revenue = 180 + pop * 12; break;
        case BT_GENETICS: revenue = 700 + pop * 18; break;
        case BT_CINEMA: revenue = 420 + pop * 17; break;
        case BT_RESTAURANT: revenue = 530 + pop * 23; break;
        case BT_BOUTIQUE: revenue = 390 + pop * 21; break;
        case BT_FISHERY: revenue = 900 + pop * 24; break;
        case BT_GREENHOUSE_DOME: revenue = 640 + pop * 20; break;
        default: revenue = 0; break;
    }
    return revenue;
}

static void bankrupt_firm(GameState *g, int fi)
{
    Firm *f = &g->firms[fi];
    if (!f->active) return;
    int bi = f->building_index - 1;
    if (bi >= 0 && bi < g->building_slots && g->buildings[bi].active) {
        Building *b = &g->buildings[bi];
        b->closed = true;
        b->workers = 0;
        b->firm_index = 0;
    }
    f->active = false;
    Sim_AddLog(g, 1, "Faillite : une entreprise ferme ses portes.");
}

static void update_household_happiness(GameState *g)
{
    int hospitals = count_operational_buildings(g, BT_HOSPITAL);
    int leisure = count_operational_buildings(g, BT_LEISURE);
    int cinemas = count_operational_buildings(g, BT_CINEMA);
    int restaurants = count_operational_buildings(g, BT_RESTAURANT);
    int boutiques = count_operational_buildings(g, BT_BOUTIQUE);
    int fortress = count_operational_buildings(g, BT_FORTRESS_FOLATRE);
    int markets = count_operational_buildings(g, BT_MARKET);
    int security = count_operational_buildings(g, BT_SECURITY);
    int transport = count_operational_buildings(g, BT_BATHYSPHERE) + count_operational_buildings(g, BT_METRO);
    int industrial_pressure = max_i(0, Sim_CountBuildings(g, BT_FACTORY) - 1 - Sim_CountBuildings(g, BT_RECYCLING) * 2);
    for (int hi = 0; hi < g->household_slots; ++hi) {
        Household *h = &g->households[hi];
        if (!h->active) continue;
        int score = 29;
        if (h->employed_adults == h->adults) score += 11;
        else if (h->employed_adults > 0) score += 4;
        else score -= 18;
        if (h->monthly_income >= (h->adults + h->children) * 105) score += 7;
        else if (h->monthly_income < (h->adults + h->children) * 50) score -= 13;
        else score += 2;
        if (h->home_building > 0) {
            score += 6;
            if (h->home_building - 1 < g->building_slots && g->buildings[h->home_building - 1].type == BT_RICH_RESIDENCE) score += 10;
        } else score -= 20;
        score += (int)((g->power_ratio + g->water_ratio + g->oxygen_ratio - 2.0f) * 15.0f);
        if (g->water_ratio < 0.60f) score -= 8;
        if (g->oxygen_ratio < 0.60f) score -= 10;
        if (g->power_ratio < 0.55f) score -= 7;
        if (g->food_ratio > 0.85f) score += 6; else if (g->food_ratio < 0.35f) score -= 18;
        if (g->goods_ratio > 0.75f) score += 4; else if (g->goods_ratio < 0.25f) score -= 8;
        int social_categories = (leisure + cinemas + fortress > 0) + (restaurants > 0) + (boutiques + markets > 0) + (hospitals > 0);
        h->social_satisfaction = social_categories * 25;
        if (g->population > 55 && social_categories < 3) score -= 5;
        if (g->population > 110 && social_categories < 4) score -= 5;
        score += h->social_satisfaction / 20;
        if (hospitals > 0) score += 2; else if (g->population > 75) score -= 5;
        if (leisure + cinemas + fortress > 0) score += g->tech[17] ? 5 : 3; else if (g->population > 45) score -= 3;
        if (fortress > 0) score += 9;
        if (restaurants > 0) score += 2;
        if (boutiques > 0) score += 2;
        if (security > 0) score += 1; else if (g->population > 100) score -= 4;
        if (transport > 0) score += min_i(5, transport * 2);
        score -= min_i(10, industrial_pressure * 2);
        score -= max_i(0, g->tax_rate - 12) * (g->tech[7] ? 0 : 2);
        score -= max_i(0, g->company_tax_rate - 5);
        score -= max_i(0, g->electricity_price + g->water_price - 5) / 2;
        score -= max_i(0, g->housing_tax - 30) / 10;
        if (h->unemployed_months > 2) score -= min_i(15, h->unemployed_months * 2);
        if (h->savings < 0) score -= 6;
        h->happiness = clampi((h->happiness * 2 + score) / 3, 0, 100);
        h->basic_needs_met = (g->food_ratio > 0.3f && g->water_ratio > 0.35f && g->oxygen_ratio > 0.35f) ? 1 : 0;
    }
}

static int get_free_home_building(const GameState *g, int people)
{
    for (int i = 0; i < g->building_slots; ++i) {
        const Building *b = &g->buildings[i];
        if (building_ready(b) && !b->closed && is_residential(b->type) &&
            b->residents + people <= BUILDING_DEFS[b->type].residents) return i;
    }
    return -1;
}

static int find_empty_near_city(GameState *g, BuildingType type, int *ox, int *oy)
{
    int w = 1, h = 1;
    Sim_GetDefaultFootprint(type, &w, &h);
    for (int attempt = 0; attempt < 1200; ++attempt) {
        int x = 3 + randi(g, MAP_W - 6);
        int y = 3 + randi(g, MAP_H - 6);
        if (x + w > MAP_W - 2 || y + h > MAP_H - 2) continue;
        if (footprint_is_clear(g, x, y, w, h) && has_neighboring_city_footprint(g, x, y, w, h, 1)) {
            *ox = x; *oy = y; return 1;
        }
    }
    return 0;
}

static void simulate_npc_business(GameState *g)
{
    if (g->population < 26 || Sim_GetFirmCount(g) >= 46 || randi(g, 100) > 11) return;
    int owner = -1;
    int richest = 0;
    for (int i = 0; i < g->household_slots; ++i) {
        Household *h = &g->households[i];
        if (h->active && h->savings > richest && h->savings > 3200) { richest = h->savings; owner = i; }
    }
    if (owner < 0) return;
    BuildingType type = BT_MARKET;
    int markets = Sim_CountBuildings(g, BT_MARKET);
    int factories = Sim_CountBuildings(g, BT_FACTORY);
    if (markets > g->population / 60 && factories < g->population / 90 + 1) type = BT_FACTORY;
    if (type == BT_FACTORY && g->materials < 30) type = BT_MARKET;
    int x, y;
    if (!find_empty_near_city(g, type, &x, &y)) return;
    int cost = BUILDING_DEFS[type].cost;
    Household *founder = &g->households[owner];
    int personal_cost = (cost * 65) / 100;
    if (founder->savings < personal_cost) return;
    int bi = add_building_internal(g, type, x, y);
    if (bi < 0) return;
    g->buildings[bi].construction_days_total = Sim_GetConstructionDays(type);
    g->buildings[bi].construction_days_remaining = g->buildings[bi].construction_days_total;
    founder->savings -= personal_cost;
    g->money -= cost - personal_cost;
    int material_cost = BUILDING_DEFS[type].material_cost;
    g->materials = max_i(0, g->materials - material_cost);
    (void)create_firm(g, bi, owner + 1);
    char message[112];
    (void)snprintf(message, sizeof(message), "Entrepreneuriat : une nouvelle entreprise ouvre.");
    Sim_AddLog(g, 0, message);
}

typedef struct AttractFactor {
    int max, got;
    char text[112];
} AttractFactor;

static void add_attract_factor(AttractFactor *factors, int *count, int max, int got, const char *text)
{
    AttractFactor *f = &factors[(*count)++];
    f->max = max;
    f->got = clampi(got, 0, max);
    (void)snprintf(f->text, sizeof(f->text), "%s", text);
}

/* City attractiveness, 0-100. It drives how many newcomers settle in Rapture and, when
   `tips` is given, lists what is missing, biggest possible gain first. Everything is
   recomputed from the current city, so the advice follows the player's construction. */
int Sim_GetAttractiveness(const GameState *g, AttractTip *tips, int max_tips)
{
    if (!g) return 0;
    AttractFactor f[24];
    int n = 0;
    int counts[BT_COUNT];
    bool employer_seen[BT_COUNT];
    memset(counts, 0, sizeof(counts));
    memset(employer_seen, 0, sizeof(employer_seen));
    int employer_types = 0;
    for (int i = 0; i < g->building_slots; ++i) {
        const Building *b = &g->buildings[i];
        if (!building_ready(b) || b->closed) continue;
        counts[b->type]++;
        if (BUILDING_DEFS[b->type].jobs >= 4 && !employer_seen[b->type]) { employer_seen[b->type] = true; employer_types++; }
    }
    int adults = 0;
    for (int i = 0; i < g->household_slots; ++i) {
        const Household *h = &g->households[i];
        if (h->active) adults += max_i(0, h->adults - h->expedition_adults);
    }
    int unemployed = max_i(0, adults - g->employed);
    int pop = g->population;
    char text[112];

    int free_homes = Sim_CountAvailableHousing(g);
    if (free_homes <= 0) (void)snprintf(text, sizeof(text), (counts[BT_RESIDENCE] + counts[BT_RICH_RESIDENCE]) == 0
        ? "Aucun logement : construisez des residences pour accueillir des habitants."
        : "Aucun logement libre : construisez une residence.");
    else (void)snprintf(text, sizeof(text), "Peu de logements libres (%d) : construisez une residence.", free_homes);
    add_attract_factor(f, &n, 15, 15 * min_i(free_homes, 8) / 8, text);

    int slack = g->available_jobs - unemployed;
    if (unemployed > 0 && slack <= 0)
        (void)snprintf(text, sizeof(text), "Chomage : %d adulte(s) sans emploi. Creez des entreprises qui embauchent.", unemployed);
    else (void)snprintf(text, sizeof(text), "Peu d'emplois libres : construisez des entreprises (usine, marche, ferme...).");
    add_attract_factor(f, &n, 12, slack >= 6 ? 12 : (slack <= 0 ? 0 : 12 * slack / 6), text);

    (void)snprintf(text, sizeof(text), "Peu de varietes d'emplois (%d types d'employeurs) : diversifiez les entreprises.", employer_types);
    add_attract_factor(f, &n, 5, 5 * min_i(employer_types, 6) / 6, text);

    (void)snprintf(text, sizeof(text), "Satisfaction moyenne basse (%d %%) : services, loisirs, emplois, taxes.", (int)g->average_happiness);
    add_attract_factor(f, &n, 18, (int)(g->average_happiness * 18.0f / 100.0f), text);

    (void)snprintf(text, sizeof(text), "Electricite insuffisante (%d %%) : construisez ou reparez une centrale.", (int)(g->power_ratio * 100.0f));
    add_attract_factor(f, &n, 5, (int)(g->power_ratio * 5.0f + 0.5f), text);
    (void)snprintf(text, sizeof(text), "Eau insuffisante (%d %%) : construisez ou reparez une usine a eau.", (int)(g->water_ratio * 100.0f));
    add_attract_factor(f, &n, 5, (int)(g->water_ratio * 5.0f + 0.5f), text);
    (void)snprintf(text, sizeof(text), "Oxygene insuffisant (%d %%) : construisez ou reparez un oxygenateur.", (int)(g->oxygen_ratio * 100.0f));
    add_attract_factor(f, &n, 5, (int)(g->oxygen_ratio * 5.0f + 0.5f), text);

    add_attract_factor(f, &n, 5, (int)(g->food_ratio * 5.0f + 0.5f), "Reserves de nourriture trop basses : construisez des fermes marines.");
    add_attract_factor(f, &n, 3, (int)(g->goods_ratio * 3.0f + 0.5f), "Pas assez de biens de consommation : usines et marches.");

    add_attract_factor(f, &n, 4, counts[BT_HOSPITAL] > 0 ? 4 : 0, "Pas d'hopital : construisez-en un pour les soins.");
    int fun = counts[BT_LEISURE] + counts[BT_CINEMA] + counts[BT_FORTRESS_FOLATRE];
    add_attract_factor(f, &n, 4, fun > 0 ? 4 : 0, g->tech[11]
        ? "Aucun loisir : construisez une salle de fete ou un cinema."
        : "Aucun loisir : construisez une salle de fete.");
    int shops = counts[BT_MARKET] + counts[BT_RESTAURANT] + counts[BT_BOUTIQUE];
    int shops_needed = 1 + pop / 120;
    (void)snprintf(text, sizeof(text), "Commerces insuffisants (%d pour %d habitants) : marche, restaurant ou boutique.", shops, pop);
    add_attract_factor(f, &n, 3, 3 * min_i(shops, shops_needed) / shops_needed, text);
    add_attract_factor(f, &n, 2, counts[BT_SECURITY] > 0 ? 2 : 0, "Aucune securite : construisez un batiment Securite.");
    add_attract_factor(f, &n, 4, (counts[BT_BATHYSPHERE] + counts[BT_METRO]) > 0 ? 4 : 0, g->tech[0]
        ? "Ville isolee : construisez une bathysphere ou un metro."
        : "Ville isolee : recherchez le Reseau de bathyspheres pour la relier.");

    int tax_penalty = max_i(0, g->tax_rate - 12) / 2 + max_i(0, g->company_tax_rate - 5) / 2 +
                      max_i(0, g->electricity_price + g->water_price - 5) / 2 + max_i(0, g->housing_tax - 30) / 10;
    (void)snprintf(text, sizeof(text), "Taxes et tarifs eleves (impot %d %%) : baissez-les pour attirer du monde.", g->tax_rate);
    add_attract_factor(f, &n, 5, 5 - tax_penalty, text);

    int pressure = max_i(0, Sim_CountBuildings(g, BT_FACTORY) - 1 - Sim_CountBuildings(g, BT_RECYCLING) * 2);
    add_attract_factor(f, &n, 5, 5 - min_i(5, pressure * 2), g->tech[6]
        ? "Pollution industrielle : construisez un centre de recyclage."
        : "Pollution industrielle : recherchez l'Economie circulaire (recyclage).");

    int score = 0;
    for (int i = 0; i < n; ++i) score += f[i].got;
    score = clampi(score, 0, 100);

    if (tips && max_tips > 0) {
        int written = 0;
        bool used[24];
        memset(used, 0, sizeof(used));
        while (written < max_tips) {
            int best = -1, best_gap = 0;
            for (int i = 0; i < n; ++i) {
                int gap = f[i].max - f[i].got;
                if (!used[i] && gap > best_gap) { best = i; best_gap = gap; }
            }
            if (best < 0) break;
            used[best] = true;
            (void)snprintf(tips[written].text, sizeof(tips[written].text), "%s", f[best].text);
            tips[written].gain = best_gap;
            written++;
        }
        for (int i = written; i < max_tips; ++i) { tips[i].text[0] = '\0'; tips[i].gain = 0; }
    }
    return score;
}

/* Newcomers arrive all through the month, in proportion to the city's attractiveness,
   and look for housing and work immediately. A monthly report summarises them. */
static void attract_new_households_daily(GameState *g)
{
    if (g->population >= 1400 || g->average_happiness < 30.0f) return;
    int score = Sim_GetAttractiveness(g, NULL, 0);
    if (score < 28) return;
    if (Sim_CountAvailableHousing(g) <= 0) return;
    int free_jobs = max_i(0, g->available_jobs);
    if (free_jobs < 1 && g->employment_ratio < 0.85f) return;
    int chance_permille = (score - 25) * 5; /* score 60 : 17 % per day, score 90 : 32 % */
    if (randi(g, 1000) >= chance_permille) return;
    int adults = 1 + randi(g, 2);
    int children = randi(g, 3);
    if (free_jobs < adults || (g->employment_ratio < 0.58f && adults > 1)) adults = 1;
    int home = get_free_home_building(g, adults + children);
    if (home < 0) { children = 0; home = get_free_home_building(g, adults); }
    if (home < 0) return;
    int hi = create_household(g, home, adults, children, 380 + randi(g, 650));
    if (hi < 0) return;
    g->arrivals_households++;
    g->arrivals_people += adults + children;
    assign_jobs_ex(g, false); /* the newcomers look for work right away */
    Sim_UpdateStatistics(g);
}

static void monthly_economy(GameState *g)
{
    Sim_UpdateStatistics(g);
    assign_jobs(g);
    Sim_UpdateStatistics(g);

    int income_tax = 0;
    int household_spending = 0;
    for (int i = 0; i < g->household_slots; ++i) {
        Household *h = &g->households[i];
        if (!h->active) continue;
        int people = h->adults + h->children;
        int bill = people * (48 + g->electricity_price + g->water_price) + 30 + g->housing_tax;
        int tax = h->monthly_income * g->tax_rate / 100;
        if (g->tech[7]) tax = tax * 85 / 100;
        int sales_tax = bill * 18 / 100;
        income_tax += tax;
        household_spending += bill;
        h->savings += h->monthly_income - bill - tax - sales_tax;
        if (h->savings < -900) {
            h->savings = -900;
            if (h->happiness > 15) h->happiness -= 2;
        }
    }

    int company_tax = 0;
    int firm_count = 0;
    int firm_upkeep = 0;
    for (int fi = 0; fi < g->firm_slots; ++fi) {
        Firm *f = &g->firms[fi];
        if (!f->active) continue;
        int bi = f->building_index - 1;
        if (bi < 0 || bi >= g->building_slots || !g->buildings[bi].active) { f->active = false; continue; }
        Building *b = &g->buildings[bi];
        if (b->closed || !building_ready(b)) { f->monthly_revenue = 0; f->employees = 0; continue; }
        int revenue = company_base_revenue(g, b->type);
        if (b->type == BT_MARKET && g->goods_stock < g->population / 2) revenue = revenue * 75 / 100;
        if ((b->type == BT_FOOD || b->type == BT_FISHERY || b->type == BT_GREENHOUSE_DOME) && g->food_stock < g->population / 2) revenue = revenue * 80 / 100;
        if (g->tech[5] && b->type == BT_FACTORY) revenue = revenue * 118 / 100;
        if (g->tech[13]) revenue = revenue * 108 / 100;
        int payroll = f->employees * f->wage;
        int supplies = (b->type == BT_FACTORY ? 420 : (b->type == BT_FOOD) ? 300 : b->type == BT_FISHERY ? 470 : b->type == BT_GREENHOUSE_DOME ? 340 : b->type == BT_MARKET ? 380 : (b->type == BT_RESTAURANT || b->type == BT_CINEMA) ? 200 : 120);
        int upkeep = BUILDING_DEFS[b->type].upkeep;
        int profit = revenue - payroll - supplies - upkeep;
        f->monthly_revenue = revenue;
        int firm_tax = max_i(0, revenue) * g->company_tax_rate / 100;
        f->cash += profit - firm_tax;
        f->total_sales += revenue;
        company_tax += firm_tax;
        firm_count++;
        firm_upkeep += upkeep;
        if (profit < -450) f->loss_months++; else f->loss_months = max_i(0, f->loss_months - 1);
        if (f->loss_months >= 4 || f->cash < -2400) bankrupt_firm(g, fi);
        /* Vital services affect output as well as morale. Keep a 50-60% floor
           during temporary outages so a short shortage hurts without instantly
           collapsing the economy. */
        if (b->type == BT_FACTORY) {
            float production = 0.60f + 0.20f * g->power_ratio + 0.10f * g->water_ratio + 0.10f * g->oxygen_ratio;
            g->goods_stock += (int)((float)(g->tech[5] ? 44 : 32) * production);
        }
        if (b->type == BT_FOOD) {
            float production = 0.60f + 0.40f * g->water_ratio;
            g->food_stock += (int)(36.0f * production * (g->tech[16] ? 1.18f : 1.0f));
        }
        if (b->type == BT_FISHERY) {
            float production = 0.50f + 0.30f * g->water_ratio + 0.20f * g->oxygen_ratio;
            g->food_stock += (int)(100.0f * production);
        }
        if (b->type == BT_GREENHOUSE_DOME) {
            /* More food than an open-air farm (BT_FOOD), closer to the marine fishery, and it
               shares the Biotechnologie marine yield bonus since it is the same cultivation science. */
            float production = 0.58f + 0.42f * g->water_ratio;
            g->food_stock += (int)(58.0f * production * (g->tech[16] ? 1.18f : 1.0f));
        }
    }
    (void)firm_count;

    int city_maintenance = 0;
    int city_payroll = 0;
    int research_gain = 0;
    int recycling_count = count_operational_buildings(g, BT_RECYCLING);
    int hephaestus_count = count_operational_buildings(g, BT_HEPHAESTUS);
    if (hephaestus_count > 0) {
        int required_materials = 6 * hephaestus_count;
        int consumed = min_i(g->materials, required_materials);
        g->materials -= consumed;
        if (consumed < required_materials && g->day == 1) {
            int any_hephaestus = -1;
            for (int hi = 0; hi < g->building_slots; ++hi)
                if (g->buildings[hi].active && g->buildings[hi].type == BT_HEPHAESTUS) { any_hephaestus = hi; break; }
            Sim_AddLogFor(g, 1, any_hephaestus, "Hephaistos manque de materiaux : sa production electrique est reduite.");
        }
    }
    for (int bi = 0; bi < g->building_slots; ++bi) {
        Building *b = &g->buildings[bi];
        if (!building_ready(b) || b->closed) continue;
        const BuildingDef *def = &BUILDING_DEFS[b->type];
        if (!def->is_company) {
            city_maintenance += def->upkeep;
            city_payroll += b->workers * 150;
        }
        if (b->type == BT_LABORATORY && g->power_ratio > 0.45f) {
            int gain = 4 + b->workers / 2;
            research_gain += g->tech[14] ? gain * 6 / 5 + 1 : gain;
        }
        if (b->type == BT_GENETICS && g->power_ratio > 0.45f) {
            int gain = 7 + b->workers / 2;
            research_gain += g->tech[14] ? gain * 6 / 5 + 1 : gain;
        }
        if (b->type == BT_POWER && g->power_ratio < 0.6f) b->condition = max_i(15, b->condition - 2);
    }
    if (recycling_count > 0) city_maintenance = city_maintenance * max_i(55, 100 - 8 * recycling_count) / 100;
    city_maintenance += firm_upkeep / 4; /* municipal inspection and shared systems */
    /* Sales levies and small civic service fees keep the public network solvent. */
    int sales_levy = household_spending * 18 / 100;
    int civic_fee = g->population * 30;
    int utility_income = g->population * (g->electricity_price + g->water_price);
    int housing_income = g->households_count * g->housing_tax;
    int tax_total = income_tax + company_tax + sales_levy + civic_fee + utility_income + housing_income;
    int civic_fund = tax_total;
    int expenses = city_maintenance + city_payroll;
    g->money += civic_fund - expenses;
    g->monthly_income = civic_fund;
    g->monthly_expenses = expenses;
    g->research += research_gain;
    if (recycling_count > 0) g->materials += 2 * recycling_count;
    g->materials += count_operational_buildings(g, BT_FACTORY) * ((g->tech[5] ? 6 : 4) + (g->tech[15] ? 2 : 0));
    int airlocks = count_operational_buildings(g, BT_AIRLOCK);
    if (airlocks > 0 && randi(g, 100) < min_i(65, 18 * airlocks)) {
        int recovered = airlocks * (2 + randi(g, 3));
        g->materials += recovered;
        g->money += airlocks * 90;
        Sim_AddLog(g, 0, "Sortie oceanique : des materiaux ont ete recuperes.");
    }
    g->materials = clampi(g->materials, 0, 9999);

    /* Consume citywide food and manufactured goods after production. */
    int food_consumption = (g->population * 3) / 5;
    int goods_consumption = (g->population * 2) / 3;
    g->food_stock = max_i(0, g->food_stock - food_consumption);
    g->goods_stock = max_i(0, g->goods_stock - goods_consumption);
    if (recycling_count > 0) g->goods_stock += recycling_count * 4;
    g->food_stock = clampi(g->food_stock, 0, 99999);
    g->goods_stock = clampi(g->goods_stock, 0, 99999);

    Sim_UpdateStatistics(g);
    update_household_happiness(g);
    Sim_UpdateStatistics(g);
    if (g->arrivals_households > 0) {
        char msg[112];
        (void)snprintf(msg, sizeof(msg), "De nouveaux residents s'installent a Rapture : %d habitants ce mois-ci.", g->arrivals_people);
        Sim_AddLog(g, 0, msg);
    }
    g->arrivals_households = 0;
    g->arrivals_people = 0;
    simulate_npc_business(g);
    Sim_UpdateStatistics(g);

    if (g->average_happiness < 35.0f) Sim_AddLog(g, 1, "Le mecontentement grandit dans les quartiers.");
    else if (g->average_happiness >= 72.0f && randi(g, 100) < 30) Sim_AddLog(g, 0, "La population salue les progres de Rapture.");
    if (g->power_ratio < 0.55f) Sim_AddLog(g, 2, "ALERTE : le reseau electrique est en sous-capacite.");
    if (g->oxygen_ratio < 0.55f) Sim_AddLog(g, 2, "ALERTE : les reserves d'oxygene sont insuffisantes.");
    if (g->water_ratio < 0.55f) Sim_AddLog(g, 1, "Alerte : la distribution d'eau est limitee.");
    if (g->food_stock < max_i(20, g->population / 2)) Sim_AddLog(g, 1, "Les reserves alimentaires deviennent faibles.");
    if (g->money < 12000) Sim_AddLog(g, 1, "Tresorerie basse : surveillez les depenses publiques.");
    if (g->month % 3 == 0) {
        char msg[112];
        (void)snprintf(msg, sizeof(msg), "Bilan trimestriel : %d habitants, satisfaction %.0f%%.", g->population, g->average_happiness);
        Sim_AddLog(g, 0, msg);
    }

    /* Campaign milestones. The sandbox deliberately has no victory/defeat state. */
    if (g->mode == MODE_CAMPAIGN && !g->won && !g->lost) {
        bool goal = false;
        int obstacles_cleared = max_i(0, g->obstacles_initial - Sim_CountObstacleTiles(g));
        int commerce_count = count_operational_buildings(g, BT_MARKET) + count_operational_buildings(g, BT_RESTAURANT) +
                             count_operational_buildings(g, BT_BOUTIQUE) + count_operational_buildings(g, BT_CINEMA);
        int landmark_count = count_operational_buildings(g, BT_ARCADIA) + count_operational_buildings(g, BT_HEPHAESTUS) +
                             count_operational_buildings(g, BT_FORTRESS_FOLATRE);
        switch (g->campaign_stage) {
            /* I. Fondation : premiers habitants et moral correct. */
            case 0: goal = g->population >= 35 && g->average_happiness >= 38.0f; break;
            /* II. Services publics : un hopital et des reseaux vitaux stables. */
            case 1: goal = g->population >= 70 && count_operational_buildings(g, BT_HOSPITAL) > 0 &&
                          g->power_ratio >= 0.45f && g->water_ratio >= 0.45f && g->oxygen_ratio >= 0.45f; break;
            /* III. Ouverture : la cite se relie a l'exterieur. */
            case 2: goal = count_operational_buildings(g, BT_BATHYSPHERE) + count_operational_buildings(g, BT_METRO) > 0; break;
            /* IV. Expansion sous-marine : un premier obstacle degage grace a l'ouverture precedente. */
            case 3: goal = g->population >= 100 && obstacles_cleared >= 1; break;
            /* V. Metropole : services vitaux et moral a un palier superieur. */
            case 4: goal = g->population >= 150 && g->average_happiness >= 55.0f &&
                          g->power_ratio >= 0.50f && g->water_ratio >= 0.50f && g->oxygen_ratio >= 0.50f; break;
            /* VI. Economie locale : le commerce se diversifie. */
            case 5: goal = commerce_count >= 2 && g->open_firms >= 4; break;
            /* VII. Progres : recherche et tissu economique approfondis. */
            case 6: goal = g->population >= 220 && count_techs(g) >= 6 && g->open_firms >= 5; break;
            /* VIII. Recherche de pointe : un batiment remarquable, fruit d'une recherche avancee. */
            case 7: goal = count_techs(g) >= 10 && landmark_count >= 1; break;
            /* IX. Classes sociales : logements de prestige et agriculture avancee. */
            case 8: goal = count_operational_buildings(g, BT_RICH_RESIDENCE) >= 1 && count_operational_buildings(g, BT_GREENHOUSE_DOME) >= 1; break;
            /* X. Utopie : une cite prospere et bien geree. */
            case 9: goal = g->population >= 350 && g->average_happiness >= 65.0f && count_techs(g) >= 14 && g->money > 15000; break;
            /* XI. Transcendance : Rapture a conquis l'ocean qui l'entoure. */
            case 10: goal = g->population >= 550 && g->average_happiness >= 72.0f && obstacles_cleared >= 3 && count_techs(g) >= 18; break;
            default: break;
        }
        if (goal) {
            g->campaign_stage++;
            g->money += 4500 + g->campaign_stage * 900;
            char msg[112];
            (void)snprintf(msg, sizeof(msg), "Objectif de campagne atteint ! Prime de developpement recue.");
            Sim_AddLog(g, 0, msg);
            if (g->campaign_stage >= 11) {
                g->won = true;
                Sim_AddLog(g, 0, "Rapture est devenue une metropole autonome. Victoire !");
            }
        }
        if (g->money < -24000 || (g->average_happiness < 8.0f && g->population > 20)) {
            g->lost = true;
            Sim_AddLog(g, 2, "La crise a depasse le point de rupture. La campagne est terminee.");
        }
    }
}

void Sim_Init(GameState *g, GameMode mode)
{
    if (!g) return;
    static uint32_t init_counter = 0u;
    init_counter += 0x9E3779B9u;
    memset(g, 0, sizeof(*g));
    g->rng = 0x71A9D3E5u ^ (uint32_t)time(NULL) ^ (uint32_t)(uintptr_t)g ^ init_counter;
    if (g->rng == 0u) g->rng = 0xA341316Cu;
    g->money = mode == MODE_SANDBOX ? 150000 : 90000;
    g->materials = mode == MODE_SANDBOX ? 450 : 260;
    g->food_stock = 190;
    g->goods_stock = 170;
    g->research = 12;
    g->tax_rate = 12;
    g->company_tax_rate = 5;
    g->housing_tax = 25;
    g->electricity_price = 2;
    g->water_price = 2;
    g->day = 1; g->month = 1; g->year = 1958;
    g->speed = 1; g->mode = (int)mode; g->paused = false;
    g->show_grid = true; g->show_overlay = true;
    for (int x = 0; x < MAP_W; ++x) for (int y = 0; y < MAP_H; ++y)
        g->map[x][y].terrain = (uint8_t)((x * 13 + y * 7 + (x * y) % 5) % 5);
    generate_ocean_obstacles(g);

    /* A connected cross-shaped transit spine is the foundation of every generated city. */
    for (int x = 16; x <= 32; ++x) (void)add_building_internal(g, BT_CORRIDOR, x, 24);
    for (int y = 19; y <= 29; ++y) (void)add_building_internal(g, BT_CORRIDOR, 24, y);

    /* Keep the opening service mix fair, but vary its order, orientation and exact location. */
    BuildingType starting_types[] = {
        BT_POWER, BT_WATER, BT_OXYGEN, BT_LABORATORY,
        BT_RESIDENCE, BT_MARKET, BT_RESIDENCE, BT_HOSPITAL,
        BT_FOOD, BT_FACTORY, BT_LEISURE, BT_SECURITY
    };
    const int start_count = (int)(sizeof(starting_types) / sizeof(starting_types[0]));
    for (int i = start_count - 1; i > 0; --i) {
        int j = randi(g, i + 1);
        BuildingType tmp = starting_types[i]; starting_types[i] = starting_types[j]; starting_types[j] = tmp;
    }
    for (int i = 0; i < start_count; ++i) {
        BuildingType type = starting_types[i];
        int fw = 1, fh = 1;
        Sim_GetDefaultFootprint(type, &fw, &fh);
        if (fw != fh && randi(g, 2) == 1) { int tmp = fw; fw = fh; fh = tmp; }
        bool placed = false;
        for (int attempt = 0; attempt < 6000 && !placed; ++attempt) {
            int x = 16 + randi(g, 17);
            int y = 18 + randi(g, 13);
            if (x < 2 || y < 2 || x + fw >= MAP_W - 2 || y + fh >= MAP_H - 2) continue;
            if (!footprint_is_clear(g, x, y, fw, fh)) continue;
            if (!has_neighboring_city_footprint(g, x, y, fw, fh, 1)) continue;
            placed = add_building_with_footprint(g, type, x, y, fw, fh) >= 0;
        }
        /* The reserved grid is generous; this fallback keeps essential starts guaranteed. */
        if (!placed) {
            for (int radius = 1; radius <= 12 && !placed; ++radius) {
                for (int x = 15; x <= 33 && !placed; ++x) for (int y = 17; y <= 32 && !placed; ++y) {
                    if (abs(x - 24) + abs(y - 24) > radius) continue;
                    if (!footprint_is_clear(g, x, y, fw, fh) || !has_neighboring_city_footprint(g, x, y, fw, fh, 1)) continue;
                    placed = add_building_with_footprint(g, type, x, y, fw, fh) >= 0;
                }
            }
        }
    }
    int homes[2] = {-1, -1};
    int home_count = 0;
    for (int i = 0; i < g->building_slots; ++i) {
        if (g->buildings[i].active && g->buildings[i].type == BT_RESIDENCE && home_count < 2)
            homes[home_count++] = i;
    }
    int home_a = homes[0], home_b = homes[1] >= 0 ? homes[1] : homes[0];
    if (home_a < 0 || home_b < 0) {
        /* Defensive fallback should only be reached if the map constants change. */
        if (home_a < 0) home_a = add_building_internal(g, BT_RESIDENCE, 23, 23);
        if (home_b < 0) home_b = add_building_internal(g, BT_RESIDENCE, 25, 25);
    }
    for (int i = 0; i < 12; ++i) {
        int home = (i % 2 == 0) ? home_a : home_b;
        int adults = 1 + randi(g, 2);
        int children = randi(g, 3);
        (void)create_household(g, home, adults, children, 500 + randi(g, 1900));
    }
    for (int i = 0; i < g->building_slots; ++i) {
        if (g->buildings[i].active && BUILDING_DEFS[g->buildings[i].type].is_company)
            (void)create_firm(g, i, choose_owner(g));
    }
    Sim_UpdateStatistics(g);
    assign_jobs(g);
    Sim_UpdateStatistics(g);
    update_household_happiness(g);
    Sim_UpdateStatistics(g);
    Sim_AddLog(g, 0, "Bienvenue a Rapture, Monsieur Ryan.");
    Sim_AddLog(g, 0, "Les systemes vitaux sont operationnels. Developpez la cite avec prudence.");
    Sim_AddLog(g, 0, "Astuce : reliez les nouveaux batiments au reseau existant par des corridors.");
}

bool Sim_BuildOriented(GameState *g, BuildingType type, int x, int y, bool rotate, char *reason, int reason_size)
{
    if (!g || type < 0 || type >= BT_COUNT) { if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Type invalide."); return false; }
    const BuildingDef *def = &BUILDING_DEFS[type];
    int fw = 1, fh = 1;
    Sim_GetDefaultFootprint(type, &fw, &fh);
    if (rotate) { int tmp = fw; fw = fh; fh = tmp; }
    if (x < 2 || y < 2 || x + fw > MAP_W - 2 || y + fh > MAP_H - 2) {
        if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Le terrain est trop proche de la limite pour cette emprise (%dx%d).", fw, fh);
        return false;
    }
    if (!footprint_is_clear(g, x, y, fw, fh)) { if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "L'emprise %dx%d est deja occupee.", fw, fh); return false; }
    if (def->unlock_tech >= 0 && !g->tech[def->unlock_tech]) {
        if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Technologie requise : %s.", TECH_NAMES[def->unlock_tech]);
        return false;
    }
    if (is_unique_landmark(type) && Sim_CountBuildings(g, type) > 0) {
        if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "%s est un batiment remarquable unique : il existe deja dans cette cite.", def->name);
        return false;
    }
    if (type == BT_FISHERY && !has_operational_sea_access(g)) {
        if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "La pecherie-fontaine necessite un sas oceanique ou une bathysphere operationnelle.");
        return false;
    }
    if (type == BT_ARCADIA && count_operational_buildings(g, BT_FOOD) < 3) {
        if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Arcadie necessite au moins trois fermes marines operationnelles.");
        return false;
    }
    if (type == BT_HEPHAESTUS && count_operational_buildings(g, BT_FACTORY) < 3) {
        if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Le Pole d'Hephaistos necessite au moins trois usines operationnelles.");
        return false;
    }
    if (type == BT_FORTRESS_FOLATRE && count_operational_buildings(g, BT_LEISURE) < 3) {
        if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "La Forteresse Folatre necessite au moins trois salles de fete operationnelles.");
        return false;
    }
    if (!has_neighboring_city_footprint(g, x, y, fw, fh, 1)) {
        if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Le batiment doit toucher un corridor ou un batiment existant.");
        return false;
    }
    if (g->money < def->cost) { if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Tresorerie insuffisante : %d $ requis.", def->cost); return false; }
    if (g->materials < def->material_cost) { if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Materiaux insuffisants : %d requis.", def->material_cost); return false; }
    int bi = add_building_with_footprint(g, type, x, y, fw, fh);
    if (bi < 0) { if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "La ville a atteint sa capacite de construction ou le terrain ne convient pas."); return false; }
    int days = Sim_GetConstructionDays(type);
    g->buildings[bi].construction_days_total = days;
    g->buildings[bi].construction_days_remaining = days;
    g->money -= def->cost;
    g->materials -= def->material_cost;
    if (def->is_company) (void)create_firm(g, bi, choose_owner(g));
    char message[112];
    (void)snprintf(message, sizeof(message), "Nouveau chantier : %s (%d jours).", def->name, days);
    Sim_AddLogFor(g, 0, bi, message);
    if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Chantier lance : %s (%d jours, emprise %dx%d).", def->name, days, fw, fh);
    Sim_UpdateStatistics(g);
    return true;
}

bool Sim_Build(GameState *g, BuildingType type, int x, int y, char *reason, int reason_size)
{
    return Sim_BuildOriented(g, type, x, y, false, reason, reason_size);
}

bool Sim_Demolish(GameState *g, int x, int y, char *reason, int reason_size)
{
    int bi = Sim_GetBuildingAt(g, x, y);
    if (bi < 0) { if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Aucun batiment sur cette case."); return false; }
    Building *b = &g->buildings[bi];
    if (is_residential(b->type) && b->residents > 0) {
        if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Des familles vivent encore ici. Construisez des logements ailleurs d'abord.");
        return false;
    }
    int fi = firm_for_building(g, bi);
    if (fi >= 0) { g->firms[fi].active = false; g->firms[fi].building_index = 0; }
    for (int ix = b->x; ix < b->x + max_i(1, b->footprint_w); ++ix)
        for (int iy = b->y; iy < b->y + max_i(1, b->footprint_h); ++iy)
            if (ix >= 0 && iy >= 0 && ix < MAP_W && iy < MAP_H && g->map[ix][iy].building == bi + 1)
                g->map[ix][iy].building = 0;
    int refund = BUILDING_DEFS[b->type].cost / 8;
    g->money += refund;
    b->active = false;
    b->closed = false;
    if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Batiment demoli. Recuperation : %d $.", refund);
    Sim_AddLog(g, 0, "Un batiment a ete demoli.");
    Sim_UpdateStatistics(g);
    return true;
}

bool Sim_Research(GameState *g, int tech_index, char *reason, int reason_size)
{
    if (!g || tech_index < 0 || tech_index >= TECH_COUNT) { if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Recherche invalide."); return false; }
    if (g->tech[tech_index]) { if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Cette technologie est deja acquise."); return false; }
    for (int prereq = 0; prereq < TECH_COUNT; ++prereq) {
        if ((TECH_PREREQUISITES[tech_index] & (1u << prereq)) != 0u && !g->tech[prereq]) {
            if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Prerequis manquant : %s.", TECH_NAMES[prereq]);
            return false;
        }
    }
    if (g->research < TECH_COST_POINTS[tech_index]) { if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Recherche insuffisante : %d points requis.", TECH_COST_POINTS[tech_index]); return false; }
    if (g->money < TECH_COST_MONEY[tech_index]) { if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Budget insuffisant : %d $ requis.", TECH_COST_MONEY[tech_index]); return false; }
    g->research -= TECH_COST_POINTS[tech_index];
    g->money -= TECH_COST_MONEY[tech_index];
    g->tech[tech_index] = true;
    char message[112];
    (void)snprintf(message, sizeof(message), "Recherche terminee : %s.", TECH_NAMES[tech_index]);
    Sim_AddLog(g, 1, message);
    if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Technologie debloquee : %s.", TECH_NAMES[tech_index]);
    return true;
}

bool Sim_SendOceanExpedition(GameState *g, int crew, char *reason, int reason_size)
{
    if (!g) return false;
    if (crew < 2 || crew > OCEAN_EXPEDITION_MAX_CREW) {
        if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Une expedition doit compter de 2 a 8 ouvriers.");
        return false;
    }
    if (g->expedition_active) {
        if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Une expedition est deja en mer.");
        return false;
    }
    if (Sim_CountBuildings(g, BT_AIRLOCK) + Sim_CountBuildings(g, BT_BATHYSPHERE) <= 0) {
        if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Construisez un sas oceanique ou une bathysphere avant l'expedition.");
        return false;
    }
    int free_workers = 0;
    for (int i = 0; i < g->household_slots; ++i) {
        const Household *h = &g->households[i];
        if (h->active) free_workers += max_i(0, h->adults - h->expedition_adults);
    }
    if (free_workers < crew) {
        if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Il faut %d ouvriers disponibles ; seuls %d sont libres.", crew, free_workers);
        return false;
    }
    int launch_cost = 450 + crew * 115;
    if (g->money < launch_cost) {
        if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Budget insuffisant : l'equipement coute %d $.", launch_cost);
        return false;
    }
    int remaining = crew;
    /* Reserve adults for the voyage, then reassign remaining labour to open jobs. */
    for (int i = g->household_slots - 1; i >= 0 && remaining > 0; --i) {
        Household *h = &g->households[i];
        if (!h->active) continue;
        int available_here = max_i(0, h->adults - h->expedition_adults);
        int take = min_i(remaining, available_here);
        h->expedition_adults += take;
        remaining -= take;
    }
    g->money -= launch_cost;
    g->expedition_active = true;
    g->expedition_crew = crew;
    g->expedition_days_remaining = g->tech[10] ? (5 + (crew + 1) / 2) : (12 + crew);
    g->expedition_risk = clampi(27 - Sim_CountBuildings(g, BT_SECURITY) * 3 - (g->tech[2] ? 8 : 0) - (g->tech[10] ? 5 : 0), 6, 30);
    assign_jobs(g);
    Sim_UpdateStatistics(g);
    if (reason && reason_size > 0) (void)snprintf(reason, (size_t)reason_size, "Expedition lancee : %d ouvriers, retour estime dans %d jours.", crew, g->expedition_days_remaining);
    Sim_AddLog(g, 0, "Une equipe d'ouvriers quitte Rapture pour recuperer des materiaux dans l'ocean.");
    return true;
}

static void generate_city_event(GameState *g)
{
    if (randi(g, 100) >= 5) return; /* roughly one event every three weeks */
    switch (randi(g, 6)) {
        case 0: {
            int chosen = -1;
            for (int tries = 0; tries < 16 && chosen < 0; ++tries) {
                int bi = randi(g, g->building_slots);
                if (bi >= 0 && bi < g->building_slots && g->buildings[bi].active &&
                    g->buildings[bi].type == BT_CORRIDOR && g->buildings[bi].condition > 20) chosen = bi;
            }
            if (chosen >= 0) {
                Building *b = &g->buildings[chosen];
                b->condition = max_i(12, b->condition - (10 + randi(g, 8)));
                Sim_AddLogFor(g, 2, chosen, "Incident de corridor : une fuite de pression exige une inspection.");
            } else {
                g->money -= 450;
                Sim_AddLog(g, 1, "Incident technique : une equipe de maintenance a ete mobilisee.");
            }
            break;
        }
        case 1:
            if (g->average_happiness < 63.0f) {
                for (int i = 0; i < g->household_slots; ++i) if (g->households[i].active)
                    g->households[i].happiness = max_i(0, g->households[i].happiness - (2 + randi(g, 4)));
                g->money -= 350 + randi(g, 500);
                Sim_AddLog(g, 2, "Debut de revolte ouvriere : salaires et conditions de travail sont contestes.");
            } else {
                Sim_AddLog(g, 1, "Une assemblee syndicale reclame une meilleure representation des travailleurs.");
            }
            break;
        case 2: {
            int grant = 1500 + randi(g, 1801);
            g->money += grant;
            Sim_AddLog(g, 1, "Bonne nouvelle : un investisseur finance les infrastructures de Rapture.");
            break;
        }
        case 3: {
            int recovered = 8 + randi(g, 17);
            g->materials = clampi(g->materials + recovered, 0, 9999);
            Sim_AddLog(g, 1, "Bonne nouvelle : une caisse de materiaux a ete remontee des fonds marins.");
            break;
        }
        case 4: {
            int chosen = -1;
            for (int i = 0; i < g->building_slots; ++i) {
                if (g->buildings[i].active && g->buildings[i].type == BT_POWER && building_ready(&g->buildings[i])) { chosen = i; break; }
            }
            if (chosen >= 0) g->buildings[chosen].condition = max_i(15, g->buildings[chosen].condition - 8);
            g->money -= 650;
            Sim_AddLogFor(g, 2, chosen, "Surtension electrique : une intervention est necessaire dans le reseau.");
            break;
        }
        default:
            if (g->average_happiness > 55.0f) {
                for (int i = 0; i < g->household_slots; ++i) if (g->households[i].active)
                    g->households[i].happiness = min_i(100, g->households[i].happiness + 2);
                Sim_AddLog(g, 1, "Bonne nouvelle : un festival local renforce le moral des habitants.");
            } else {
                Sim_AddLog(g, 1, "Le journal local signale une hausse de la demande de logements et de services.");
            }
            break;
    }
    Sim_UpdateStatistics(g);
}

void Sim_TickDay(GameState *g)
{
    if (!g || g->paused || g->speed <= 0 || g->won || g->lost) return;
    for (int i = 0; i < g->building_slots; ++i) {
        Building *b = &g->buildings[i];
        if (!b->active || b->construction_days_remaining <= 0) continue;
        b->construction_days_remaining--;
        if (b->construction_days_remaining == 0) {
            char msg[112];
            (void)snprintf(msg, sizeof(msg), "Chantier termine : %s est operationnel.", BUILDING_DEFS[b->type].name);
            Sim_AddLogFor(g, 1, i, msg);
            Sim_UpdateStatistics(g);
        }
    }
    g->day++;
    if (g->day > 30) {
        g->day = 1;
        g->month++;
        if (g->month > 12) { g->month = 1; g->year++; }
        monthly_economy(g);
    }
    attract_new_households_daily(g);
    if (g->clearing_active) {
        g->clearing_days_remaining--;
        if (g->clearing_days_remaining <= 0) {
            int cleared_tiles = g->clearing_tile_count;
            for (int k = 0; k < cleared_tiles; ++k) {
                int cx = g->clearing_tile_x[k], cy = g->clearing_tile_y[k];
                if (cx >= 0 && cy >= 0 && cx < MAP_W && cy < MAP_H)
                    g->map[cx][cy].terrain = (uint8_t)((cx * 13 + cy * 7 + (cx * cy) % 5) % 5);
            }
            g->clearing_active = false;
            g->clearing_tile_count = 0;
            char msg[112];
            (void)snprintf(msg, sizeof(msg), "Obstacle degage : %d case(s) sont desormais constructibles.", max_i(1, cleared_tiles));
            Sim_AddLog(g, 1, msg);
        }
    }
    /* Ocean crews return after their field trip; outcomes trade time and risk for materials. */
    if (g->expedition_active && g->expedition_days_remaining > 0) {
        g->expedition_days_remaining--;
        if (g->expedition_days_remaining <= 0) {
            int crew = g->expedition_crew;
            int chance = 100 - g->expedition_risk + (g->tech[2] ? 8 : 0);
            if (chance < 60) chance = 60;
            if (chance > 98) chance = 98;
            if (randi(g, 100) < chance) {
                int recovered = 16 + crew * 7 + randi(g, crew * 4 + 1);
                g->materials = clampi(g->materials + recovered, 0, 9999);
                char msg[112];
                (void)snprintf(msg, sizeof(msg), "Expedition oceanique rentree : %d materiaux recuperes.", recovered);
                Sim_AddLog(g, 1, msg);
            } else {
                g->money -= crew * 90;
                Sim_AddLog(g, 2, "Expedition endommagee : pertes d'equipement et cout de sauvetage.");
            }
            int to_release = crew;
            for (int i = g->household_slots - 1; i >= 0 && to_release > 0; --i) {
                Household *h = &g->households[i];
                if (!h->active || h->expedition_adults <= 0) continue;
                int release = min_i(to_release, h->expedition_adults);
                h->expedition_adults -= release;
                to_release -= release;
            }
            g->expedition_active = false;
            g->expedition_crew = 0;
            g->expedition_risk = 0;
            g->expedition_days_remaining = 0;
            assign_jobs(g);
            Sim_UpdateStatistics(g);
        }
    }
    generate_city_event(g);
    /* Daily wear and tiny utility fluctuations make the city feel alive. */
    if (g->day % 5 == 0) {
        for (int i = 0; i < g->building_slots; ++i) {
            Building *b = &g->buildings[i];
            if (building_ready(b) && !b->closed && b->condition > 20 && randi(g, 100) < (g->tech[12] ? 3 : 5)) {
                b->condition--;
                if (b->condition == 25) {
                    char warning[112];
                    (void)snprintf(warning, sizeof(warning), "Maintenance : %s se degrade et doit etre repare.", BUILDING_DEFS[b->type].name);
                    Sim_AddLogFor(g, 2, i, warning);
                }
            }
        }
    }
}

int Sim_GetCampaignGoal(const GameState *g, char *buffer, int buffer_size)
{
    if (!g || !buffer || buffer_size <= 0) return 0;
    if (g->mode == MODE_SANDBOX) {
        (void)snprintf(buffer, (size_t)buffer_size, "Bac a sable : construisez Rapture sans objectif impose.");
        return 0;
    }
    static const char *goals[] = {
        "I. Fondation : 35 habitants et satisfaction d'au moins 38 %.",
        "II. Services publics : 70 habitants, un hopital et electricite/eau/oxygene a 45 % minimum.",
        "III. Ouverture : une bathysphere ou une station de metro operationnelle.",
        "IV. Expansion sous-marine : 100 habitants et un obstacle oceanique degage.",
        "V. Metropole : 150 habitants, satisfaction a 55 % et services vitaux a 50 % minimum.",
        "VI. Economie locale : au moins 2 commerces et 4 entreprises en activite.",
        "VII. Progres : 220 habitants, 6 technologies et 5 entreprises en activite.",
        "VIII. Recherche de pointe : 10 technologies et un batiment remarquable operationnel.",
        "IX. Classes sociales : une residence de prestige et une foret sous cloche operationnelles.",
        "X. Utopie : 350 habitants, 14 technologies, satisfaction a 65 % et 15 000 $ en caisse.",
        "XI. Transcendance : 550 habitants, satisfaction a 72 %, 3 obstacles degages et 18 technologies.",
        "Campagne terminee : Rapture est autonome."
    };
    int stage = clampi(g->campaign_stage, 0, 11);
    (void)snprintf(buffer, (size_t)buffer_size, "%s", goals[stage]);
    return stage;
}

bool Sim_Save(const GameState *g, const char *path)
{
    if (!g || !path) return false;
    FILE *fp = fopen(path, "w");
    if (!fp) return false;
    fprintf(fp, "RAPTURE_SAVE 10\n");
    fprintf(fp, "CORE %d %d %d %d %d %d %d %d %d %d %d %u %d %d\n",
            g->money, g->research, g->materials, g->food_stock, g->goods_stock, g->tax_rate,
            g->day, g->month, g->year, g->mode, g->campaign_stage, g->rng, g->won ? 1 : 0, g->lost ? 1 : 0);
    fprintf(fp, "POLICY %d %d %d %d %d\n", g->company_tax_rate, g->housing_tax, g->electricity_price, g->water_price, 0);
    fprintf(fp, "EXPEDITION %d %d %d %d\n", g->expedition_active ? 1 : 0, g->expedition_crew, g->expedition_days_remaining, g->expedition_risk);
    fprintf(fp, "OBSTACLES %d\n", g->obstacles_initial);
    int obstacle_tiles = 0;
    for (int x = 0; x < MAP_W; ++x) for (int y = 0; y < MAP_H; ++y) if (IS_OBSTACLE_TERRAIN(g->map[x][y].terrain)) obstacle_tiles++;
    fprintf(fp, "OBSTACLETILES %d\n", obstacle_tiles);
    for (int x = 0; x < MAP_W; ++x) for (int y = 0; y < MAP_H; ++y)
        if (IS_OBSTACLE_TERRAIN(g->map[x][y].terrain)) fprintf(fp, "O %d %d %d\n", x, y, (int)g->map[x][y].terrain);
    fprintf(fp, "CLEARING %d %d %d %d %d", g->clearing_active ? 1 : 0, g->clearing_tile_count, g->clearing_days_remaining, g->clearing_days_total, g->clearing_workers);
    for (int k = 0; k < OBSTACLE_CLUSTER_MAX; ++k) fprintf(fp, " %d %d", g->clearing_tile_x[k], g->clearing_tile_y[k]);
    fprintf(fp, "\n");
    fprintf(fp, "TECH");
    for (int i = 0; i < TECH_COUNT; ++i) fprintf(fp, " %d", g->tech[i] ? 1 : 0);
    fprintf(fp, "\nBUILDINGS %d\n", g->building_slots);
    for (int i = 0; i < g->building_slots; ++i) {
        const Building *b = &g->buildings[i];
        fprintf(fp, "B %d %d %d %d %d %d %d %d %d %d %d\n", b->active ? 1 : 0, (int)b->type, b->x, b->y, b->condition, b->closed ? 1 : 0,
                b->firm_index, b->construction_days_remaining, b->construction_days_total, b->footprint_w, b->footprint_h);
    }
    fprintf(fp, "HOUSEHOLDS %d\n", g->household_slots);
    for (int i = 0; i < g->household_slots; ++i) {
        const Household *h = &g->households[i];
        fprintf(fp, "H %d %d %d %d %d %d %d %d %d %d %d\n", h->active ? 1 : 0, h->home_building, h->adults, h->children,
                h->savings, h->happiness, h->unemployed_months, h->employed_adults, h->monthly_income, h->expedition_adults, h->social_satisfaction);
    }
    fprintf(fp, "FIRMS %d\n", g->firm_slots);
    for (int i = 0; i < g->firm_slots; ++i) {
        const Firm *f = &g->firms[i];
        fprintf(fp, "F %d %d %d %d %d %d %d %d %d\n", f->active ? 1 : 0, f->building_index, f->owner_household,
                f->cash, f->monthly_revenue, f->employees, f->wage, f->loss_months, f->total_sales);
    }
    fprintf(fp, "LOGS %d\n", g->log_count);
    for (int i = 0; i < g->log_count; ++i) {
        /* Game logs are intentionally not persisted: avoid ambiguous delimiters and keep saves robust. */
        fprintf(fp, "L %d\n", g->logs[i].severity);
    }
    bool ok = (fclose(fp) == 0);
    return ok;
}

bool Sim_Load(GameState *g, const char *path)
{
    if (!g || !path) return false;
    FILE *fp = fopen(path, "r");
    if (!fp) return false;
    char tag[32];
    int version = 0;
    if (fscanf(fp, "%31s %d", tag, &version) != 2 || strcmp(tag, "RAPTURE_SAVE") != 0 || version < 1 || version > 10) { fclose(fp); return false; }
    GameState temp;
    memset(&temp, 0, sizeof(temp));
    if (fscanf(fp, "%31s", tag) != 1 || strcmp(tag, "CORE") != 0) { fclose(fp); return false; }
    unsigned int rng = 0;
    int won = 0, lost = 0;
    if (fscanf(fp, "%d %d %d %d %d %d %d %d %d %d %d %u %d %d",
               &temp.money, &temp.research, &temp.materials, &temp.food_stock, &temp.goods_stock, &temp.tax_rate,
               &temp.day, &temp.month, &temp.year, &temp.mode, &temp.campaign_stage, &rng, &won, &lost) != 14) { fclose(fp); return false; }
    temp.rng = rng; temp.won = won != 0; temp.lost = lost != 0; temp.speed = 1; temp.paused = false; temp.show_grid = true; temp.show_overlay = true;
    temp.company_tax_rate = 5; temp.housing_tax = 25; temp.electricity_price = 2; temp.water_price = 2;
    if (version >= 3) {
        int unused_policy = 0, active_expedition = 0;
        if (fscanf(fp, "%31s %d %d %d %d %d", tag, &temp.company_tax_rate, &temp.housing_tax,
                   &temp.electricity_price, &temp.water_price, &unused_policy) != 6 || strcmp(tag, "POLICY") != 0) { fclose(fp); return false; }
        if (fscanf(fp, "%31s %d %d %d %d", tag, &active_expedition, &temp.expedition_crew,
                   &temp.expedition_days_remaining, &temp.expedition_risk) != 5 || strcmp(tag, "EXPEDITION") != 0) { fclose(fp); return false; }
        temp.expedition_active = active_expedition != 0;
    }
    for (int x = 0; x < MAP_W; ++x) for (int y = 0; y < MAP_H; ++y)
        temp.map[x][y].terrain = (uint8_t)((x * 13 + y * 7 + (x * y) % 5) % 5);
    if (version >= 10) {
        int obstacle_tile_count = 0;
        if (fscanf(fp, "%31s %d", tag, &temp.obstacles_initial) != 2 || strcmp(tag, "OBSTACLES") != 0) { fclose(fp); return false; }
        if (fscanf(fp, "%31s %d", tag, &obstacle_tile_count) != 2 || strcmp(tag, "OBSTACLETILES") != 0 ||
            obstacle_tile_count < 0 || obstacle_tile_count > MAP_W * MAP_H) { fclose(fp); return false; }
        for (int i = 0; i < obstacle_tile_count; ++i) {
            int ox = 0, oy = 0, okind = 0;
            if (fscanf(fp, "%31s %d %d %d", tag, &ox, &oy, &okind) != 4 || strcmp(tag, "O") != 0) { fclose(fp); return false; }
            if (ox >= 0 && oy >= 0 && ox < MAP_W && oy < MAP_H && (okind == TERRAIN_OBSTACLE_ROCK || okind == TERRAIN_OBSTACLE_FISSURE))
                temp.map[ox][oy].terrain = (uint8_t)okind;
        }
        int clearing_active_flag = 0;
        if (fscanf(fp, "%31s %d %d %d %d %d", tag, &clearing_active_flag, &temp.clearing_tile_count,
                   &temp.clearing_days_remaining, &temp.clearing_days_total, &temp.clearing_workers) != 6 ||
            strcmp(tag, "CLEARING") != 0) { fclose(fp); return false; }
        temp.clearing_active = clearing_active_flag != 0;
        for (int k = 0; k < OBSTACLE_CLUSTER_MAX; ++k)
            if (fscanf(fp, "%d %d", &temp.clearing_tile_x[k], &temp.clearing_tile_y[k]) != 2) { fclose(fp); return false; }
        temp.clearing_tile_count = clampi(temp.clearing_tile_count, 0, OBSTACLE_CLUSTER_MAX);
    }
    bool needs_fresh_obstacles = version < 10;
    if (fscanf(fp, "%31s", tag) != 1 || strcmp(tag, "TECH") != 0) { fclose(fp); return false; }
    int tech_fields = version >= 10 ? TECH_COUNT : (version >= 9 ? 18 : (version >= 8 ? 12 : (version >= 6 ? 11 : (version >= 5 ? 10 : 8))));
    for (int i = 0; i < tech_fields; ++i) { int v = 0; if (fscanf(fp, "%d", &v) != 1) { fclose(fp); return false; } temp.tech[i] = v != 0; }
    int n = 0;
    if (fscanf(fp, "%31s %d", tag, &n) != 2 || strcmp(tag, "BUILDINGS") != 0 || n < 0 || n > MAX_BUILDINGS) { fclose(fp); return false; }
    temp.building_slots = n;
    for (int i = 0; i < n; ++i) {
        int active, type, x, y, condition, closed, firm_index;
        int remaining = 0, total = 0, footprint_w = 1, footprint_h = 1;
        int fields_read;
        if (version >= 4) {
            fields_read = fscanf(fp, "%31s %d %d %d %d %d %d %d %d %d %d %d", tag, &active, &type, &x, &y, &condition, &closed, &firm_index, &remaining, &total, &footprint_w, &footprint_h);
            if (fields_read != 12) { fclose(fp); return false; }
        } else {
            fields_read = fscanf(fp, "%31s %d %d %d %d %d %d %d", tag, &active, &type, &x, &y, &condition, &closed, &firm_index);
            if (fields_read != 8) { fclose(fp); return false; }
        }
        if (strcmp(tag, "B") != 0 || type < 0 || type >= BT_COUNT || x < 0 || x >= MAP_W || y < 0 || y >= MAP_H) { fclose(fp); return false; }
        Building *b = &temp.buildings[i];
        b->active = active != 0; b->type = (BuildingType)type; b->x = x; b->y = y; b->condition = clampi(condition, 0, 100); b->closed = closed != 0; b->firm_index = firm_index; b->local_happiness = 50.0f;
        if (version >= 4) {
            b->construction_days_remaining = clampi(remaining, 0, 30);
            b->construction_days_total = clampi(total, b->construction_days_remaining, 30);
            b->footprint_w = clampi(footprint_w, 1, 2);
            b->footprint_h = clampi(footprint_h, 1, 2);
        } else {
            /* Old saves used single-tile buildings; keep their footprints intact. */
            b->footprint_w = 1; b->footprint_h = 1;
            b->construction_days_remaining = 0; b->construction_days_total = 0;
        }
        if (b->active) {
            if (!footprint_is_clear(&temp, x, y, b->footprint_w, b->footprint_h)) { fclose(fp); return false; }
            for (int ix = x; ix < x + b->footprint_w; ++ix)
                for (int iy = y; iy < y + b->footprint_h; ++iy) {
                    temp.map[ix][iy].building = (int16_t)(i + 1);
                    temp.map[ix][iy].terrain = 0;
                }
        }
    }
    /* Version 0.5 saved the Pêcherie-fontaine as 2x1. Migrate it to its new
       2x2 landmark footprint only when the additional row is free, preserving
       an older city rather than rejecting a save if that space was occupied. */
    for (int i = 0; i < temp.building_slots; ++i) {
        Building *b = &temp.buildings[i];
        if (!b->active || b->type != BT_FISHERY || b->footprint_w != 2 || b->footprint_h != 1) continue;
        if (b->y + 2 > MAP_H) continue;
        bool can_expand = true;
        for (int ix = b->x; ix < b->x + 2; ++ix) {
            int occupant = Sim_GetBuildingAt(&temp, ix, b->y + 1);
            if (occupant >= 0 && occupant != i) can_expand = false;
        }
        if (!can_expand) continue;
        b->footprint_h = 2;
        for (int ix = b->x; ix < b->x + 2; ++ix) {
            temp.map[ix][b->y + 1].building = (int16_t)(i + 1);
            temp.map[ix][b->y + 1].terrain = 0;
        }
    }
    /* Older saves never had obstacles: generate a fresh field now that every loaded building
       occupies the map, so a newly placed obstacle can never overlap an existing building. */
    if (needs_fresh_obstacles) generate_ocean_obstacles(&temp);
    if (fscanf(fp, "%31s %d", tag, &n) != 2 || strcmp(tag, "HOUSEHOLDS") != 0 || n < 0 || n > MAX_HOUSEHOLDS) { fclose(fp); return false; }
    temp.household_slots = n;
    for (int i = 0; i < n; ++i) {
        int active;
        Household *h = &temp.households[i];
        int fields_read;
        if (version >= 5) {
            fields_read = fscanf(fp, "%31s %d %d %d %d %d %d %d %d %d %d %d", tag, &active, &h->home_building, &h->adults, &h->children,
                                 &h->savings, &h->happiness, &h->unemployed_months, &h->employed_adults, &h->monthly_income, &h->expedition_adults, &h->social_satisfaction);
            if (fields_read != 12) { fclose(fp); return false; }
        } else if (version >= 3) {
            fields_read = fscanf(fp, "%31s %d %d %d %d %d %d %d %d %d %d", tag, &active, &h->home_building, &h->adults, &h->children,
                                 &h->savings, &h->happiness, &h->unemployed_months, &h->employed_adults, &h->monthly_income, &h->expedition_adults);
            if (fields_read != 11) { fclose(fp); return false; }
            h->social_satisfaction = 0;
        } else {
            fields_read = fscanf(fp, "%31s %d %d %d %d %d %d %d %d %d", tag, &active, &h->home_building, &h->adults, &h->children,
                                 &h->savings, &h->happiness, &h->unemployed_months, &h->employed_adults, &h->monthly_income);
            if (fields_read != 10) { fclose(fp); return false; }
            h->expedition_adults = 0;
            h->social_satisfaction = 0;
        }
        if (strcmp(tag, "H") != 0) { fclose(fp); return false; }
        h->active = active != 0; h->happiness = clampi(h->happiness, 0, 100);
    }
    if (fscanf(fp, "%31s %d", tag, &n) != 2 || strcmp(tag, "FIRMS") != 0 || n < 0 || n > MAX_FIRMS) { fclose(fp); return false; }
    temp.firm_slots = n;
    for (int i = 0; i < n; ++i) {
        int active;
        Firm *f = &temp.firms[i];
        if (fscanf(fp, "%31s %d %d %d %d %d %d %d %d %d", tag, &active, &f->building_index, &f->owner_household,
                   &f->cash, &f->monthly_revenue, &f->employees, &f->wage, &f->loss_months, &f->total_sales) != 10 || strcmp(tag, "F") != 0) { fclose(fp); return false; }
        f->active = active != 0;
    }
    fclose(fp);
    temp.tax_rate = clampi(temp.tax_rate, 0, 35);
    temp.company_tax_rate = clampi(temp.company_tax_rate, 0, 20);
    temp.housing_tax = clampi(temp.housing_tax, 0, 120);
    temp.electricity_price = clampi(temp.electricity_price, 0, 12);
    temp.water_price = clampi(temp.water_price, 0, 12);
    temp.expedition_crew = clampi(temp.expedition_crew, 0, OCEAN_EXPEDITION_MAX_CREW);
    temp.expedition_days_remaining = clampi(temp.expedition_days_remaining, 0, 30);
    temp.expedition_risk = clampi(temp.expedition_risk, 0, 100);
    if (!temp.expedition_active) {
        temp.expedition_crew = 0;
        temp.expedition_days_remaining = 0;
        temp.expedition_risk = 0;
        for (int i = 0; i < temp.household_slots; ++i) temp.households[i].expedition_adults = 0;
    }
    temp.day = clampi(temp.day, 1, 30); temp.month = clampi(temp.month, 1, 12); temp.mode = (temp.mode == MODE_SANDBOX) ? MODE_SANDBOX : MODE_CAMPAIGN;
    temp.log_count = 0;
    recalc_building_residents(&temp);
    Sim_UpdateStatistics(&temp);
    assign_jobs(&temp);
    Sim_UpdateStatistics(&temp);
    update_household_happiness(&temp);
    Sim_AddLog(&temp, 0, "Partie chargee. Bienvenue de retour a Rapture.");
    *g = temp;
    return true;
}
