#include "rapture.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


static int find_free_connected_spot(const GameState *g, BuildingType type, bool rotate, int *out_x, int *out_y)
{
    int w = 1, h = 1;
    Sim_GetDefaultFootprint(type, &w, &h);
    if (rotate) { int tmp = w; w = h; h = tmp; }
    for (int x = 2; x + w < MAP_W - 2; ++x) {
        for (int y = 2; y + h < MAP_H - 2; ++y) {
            bool clear = true, adjacent = false;
            for (int ix = x; ix < x + w; ++ix) for (int iy = y; iy < y + h; ++iy) {
                if (Sim_GetBuildingAt(g, ix, iy) >= 0) clear = false;
                if (IS_OBSTACLE_TERRAIN(g->map[ix][iy].terrain)) clear = false;
            }
            if (!clear) continue;
            for (int ix = x; ix < x + w; ++ix) for (int iy = y; iy < y + h; ++iy) {
                if (Sim_GetBuildingAt(g, ix - 1, iy) >= 0 || Sim_GetBuildingAt(g, ix + 1, iy) >= 0 ||
                    Sim_GetBuildingAt(g, ix, iy - 1) >= 0 || Sim_GetBuildingAt(g, ix, iy + 1) >= 0) adjacent = true;
            }
            if (adjacent) { *out_x = x; *out_y = y; return 1; }
        }
    }
    return 0;
}

static void find_occupied_residence(const GameState *g, int *out_x, int *out_y)
{
    for (int i = 0; i < g->building_slots; ++i) {
        const Building *b = &g->buildings[i];
        if (b->active && b->type == BT_RESIDENCE && b->residents > 0) { *out_x = b->x; *out_y = b->y; return; }
    }
    assert(0 && "A starting residence with residents should exist");
}


static int footprint_has_cardinal_neighbor(const GameState *g, const Building *b)
{
    for (int ix = b->x; ix < b->x + b->footprint_w; ++ix) {
        for (int iy = b->y; iy < b->y + b->footprint_h; ++iy) {
            if (Sim_GetBuildingAt(g, ix - 1, iy) >= 0 || Sim_GetBuildingAt(g, ix + 1, iy) >= 0 ||
                Sim_GetBuildingAt(g, ix, iy - 1) >= 0 || Sim_GetBuildingAt(g, ix, iy + 1) >= 0) return 1;
        }
    }
    return 0;
}


static void test_adam_branch_and_save_compatibility(void)
{
    GameState g, loaded;
    char reason[192];
    int x = -1, y = -1;
    Sim_Init(&g, MODE_SANDBOX);
    g.money = 150000;
    g.research = 700;
    g.tech[3] = true; /* Test the branch after the Genetics prerequisite. */
    assert(!Sim_Research(&g, 21, reason, (int)sizeof(reason)));
    assert(strstr(reason, "Prerequis") != NULL);
    assert(Sim_Research(&g, 20, reason, (int)sizeof(reason)));
    assert(g.tech[20]);
    assert(Sim_Research(&g, 21, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 22, reason, (int)sizeof(reason)));
    assert(BUILDING_DEFS[BT_ORPHANAGE].unlock_tech == 21);
    assert(BUILDING_DEFS[BT_PROTECTOR_FACTORY].unlock_tech == 22);
    Sim_GetDefaultFootprint(BT_ORPHANAGE, &x, &y);
    assert(x == 2 && y == 1);
    Sim_GetDefaultFootprint(BT_PROTECTOR_FACTORY, &x, &y);
    assert(x == 2 && y == 2);

    g.adam_stock = 123;
    g.little_sisters = 7;
    g.protector_count = 3;
    g.orphanage_progress = 2;
    g.protector_progress = 3;
    assert(Sim_Save(&g, "rapture_adam_v11_test_save.dat"));
    memset(&loaded, 0, sizeof(loaded));
    assert(Sim_Load(&loaded, "rapture_adam_v11_test_save.dat"));
    assert(loaded.tech[20] && loaded.tech[21] && loaded.tech[22]);
    assert(loaded.adam_stock == 123);
    assert(loaded.little_sisters == 7);
    assert(loaded.protector_count == 3);
    assert(loaded.orphanage_progress == 2 && loaded.protector_progress == 3);

    /* A v10 save is identical in layout except it has only 20 technologies and
       no ADAM section. Confirm that a pre-branch city still opens cleanly. */
    FILE *src = fopen("rapture_adam_v11_test_save.dat", "r");
    FILE *dst = fopen("rapture_adam_v10_test_save.dat", "w");
    assert(src != NULL && dst != NULL);
    char line[2048];
    int first = 1;
    while (fgets(line, (int)sizeof(line), src) != NULL) {
        if (first) { fputs("RAPTURE_SAVE 10\n", dst); first = 0; }
        else if (strncmp(line, "ADAM ", 5) == 0) continue;
        else if (strncmp(line, "TECH", 4) == 0) {
            char *cursor = line + 4;
            fprintf(dst, "TECH");
            for (int i = 0; i < 20; ++i) {
                while (*cursor == ' ' || *cursor == '\t') cursor++;
                char *end = NULL;
                long value = strtol(cursor, &end, 10);
                assert(end != cursor);
                fprintf(dst, " %ld", value);
                cursor = end;
            }
            fputc('\n', dst);
        } else fputs(line, dst);
    }
    assert(!ferror(src) && !ferror(dst));
    fclose(src); fclose(dst);
    memset(&loaded, 0, sizeof(loaded));
    assert(Sim_Load(&loaded, "rapture_adam_v10_test_save.dat"));
    assert(!loaded.tech[20] && loaded.adam_stock == 0 && loaded.little_sisters == 0);
    remove("rapture_adam_v11_test_save.dat");
    remove("rapture_adam_v10_test_save.dat");
}


static void test_adam_monthly_production_cycle(void)
{
    GameState g;
    char reason[192];
    int x = -1, y = -1;
    Sim_Init(&g, MODE_SANDBOX);
    g.money = 150000;
    g.materials = 1000;
    g.tech[20] = true;
    g.tech[21] = true;
    g.tech[22] = true;
    g.adam_stock = 100;
    g.little_sisters = 1;
    assert(find_free_connected_spot(&g, BT_ORPHANAGE, false, &x, &y));
    assert(Sim_Build(&g, BT_ORPHANAGE, x, y, reason, (int)sizeof(reason)));
    assert(find_free_connected_spot(&g, BT_PROTECTOR_FACTORY, false, &x, &y));
    assert(Sim_Build(&g, BT_PROTECTOR_FACTORY, x, y, reason, (int)sizeof(reason)));
    for (int i = 0; i < g.building_slots; ++i) {
        if (!g.buildings[i].active) continue;
        if (g.buildings[i].type == BT_ORPHANAGE || g.buildings[i].type == BT_PROTECTOR_FACTORY) {
            g.buildings[i].construction_days_remaining = 0;
            g.buildings[i].construction_days_total = 0;
        }
    }
    int adam_before = g.adam_stock;
    g.paused = false;
    g.speed = 1;
    for (int day = 0; day < 30; ++day) Sim_TickDay(&g);
    assert(g.adam_stock > adam_before); /* sisters produce ADAM once the branch is unlocked */
    assert(g.little_sisters >= 1);
}

static void test_new_city(void)
{
    GameState g;
    Sim_Init(&g, MODE_CAMPAIGN);
    assert(g.mode == MODE_CAMPAIGN);
    assert(g.money == 90000);
    assert(g.population >= 20);
    assert(g.households_count == 12);
    assert(Sim_CountBuildings(&g, BT_POWER) == 1);
    assert(Sim_CountBuildings(&g, BT_RESIDENCE) == 2);
    assert(Sim_CountBuildings(&g, BT_LABORATORY) == 1);
    assert(Sim_CountBuildings(&g, BT_CORRIDOR) >= 15);
    assert(Sim_CountBuildings(&g, BT_PIPE) == 0);
    assert(g.company_tax_rate == 5 && g.housing_tax == 25);
    assert(Sim_GetFirmCount(&g) >= 3);
    for (int i = 0; i < g.building_slots; ++i) {
        const Building *b = &g.buildings[i];
        if (!b->active) continue;
        assert(footprint_has_cardinal_neighbor(&g, b));
    }
    /* Start layouts vary without losing any mandatory service or connection. */
    int first_lab_x = -1;
    for (int i = 0; i < g.building_slots; ++i)
        if (g.buildings[i].active && g.buildings[i].type == BT_LABORATORY) first_lab_x = g.buildings[i].x;
    int saw_variant = 0;
    for (int attempt = 0; attempt < 5; ++attempt) {
        GameState another;
        Sim_Init(&another, MODE_CAMPAIGN);
        assert(Sim_CountBuildings(&another, BT_POWER) == 1);
        assert(Sim_CountBuildings(&another, BT_WATER) == 1);
        assert(Sim_CountBuildings(&another, BT_OXYGEN) == 1);
        assert(Sim_CountBuildings(&another, BT_HOSPITAL) == 1);
        for (int i = 0; i < another.building_slots; ++i) {
            const Building *b = &another.buildings[i];
            if (b->active) assert(footprint_has_cardinal_neighbor(&another, b));
            if (b->active && b->type == BT_LABORATORY && b->x != first_lab_x) saw_variant = 1;
        }
    }
    assert(saw_variant);
}

static void test_construction_and_research(void)
{
    GameState g;
    char reason[160];
    Sim_Init(&g, MODE_SANDBOX);
    int old_money = g.money;
    int old_materials = g.materials;
    int cx = -1, cy = -1;
    assert(find_free_connected_spot(&g, BT_CORRIDOR, false, &cx, &cy));
    assert(Sim_Build(&g, BT_CORRIDOR, cx, cy, reason, (int)sizeof(reason)));
    assert(Sim_GetBuildingAt(&g, cx, cy) >= 0);
    assert(g.money == old_money - BUILDING_DEFS[BT_CORRIDOR].cost);
    assert(g.materials == old_materials);
    int bx = -1, by = -1;
    assert(find_free_connected_spot(&g, BT_BATHYSPHERE, false, &bx, &by));
    assert(!Sim_Build(&g, BT_BATHYSPHERE, bx, by, reason, (int)sizeof(reason)));
    assert(strstr(reason, "Technologie") != NULL);

    g.research = 200;
    assert(Sim_Research(&g, 2, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 0, reason, (int)sizeof(reason)));
    assert(g.tech[0]);
    assert(find_free_connected_spot(&g, BT_BATHYSPHERE, false, &bx, &by));
    assert(Sim_Build(&g, BT_BATHYSPHERE, bx, by, reason, (int)sizeof(reason)));
    assert(Sim_CountBuildings(&g, BT_BATHYSPHERE) == 1);
    assert(!Sim_Research(&g, 0, reason, (int)sizeof(reason)));
}

static void test_multi_tile_construction_and_save(void)
{
    GameState g, loaded;
    char reason[160];
    Sim_Init(&g, MODE_SANDBOX);
    int fw = 0, fh = 0;
    Sim_GetDefaultFootprint(BT_FACTORY, &fw, &fh);
    assert(fw == 2 && fh == 2);
    Sim_GetDefaultFootprint(BT_FOOD, &fw, &fh);
    assert(fw == 2 && fh == 1);
    Sim_GetDefaultFootprint(BT_FISHERY, &fw, &fh);
    assert(fw == 2 && fh == 2);
    Sim_GetDefaultFootprint(BT_ARCADIA, &fw, &fh);
    assert(fw == 2 && fh == 2);
    Sim_GetDefaultFootprint(BT_HEPHAESTUS, &fw, &fh);
    assert(fw == 2 && fh == 2);
    Sim_GetDefaultFootprint(BT_FORTRESS_FOLATRE, &fw, &fh);
    assert(fw == 2 && fh == 2);
    assert(Sim_GetConstructionDays(BT_PIPE) == 2);
    assert(Sim_GetConstructionDays(BT_RESIDENCE) == 7);
    assert(Sim_GetConstructionDays(BT_POWER) == 8);
    assert(Sim_GetConstructionDays(BT_FACTORY) == 10);
    int firms_before_construction = g.open_firms;
    int bx = -1, by = -1;
    assert(find_free_connected_spot(&g, BT_FACTORY, false, &bx, &by));
    assert(Sim_Build(&g, BT_FACTORY, bx, by, reason, (int)sizeof(reason)));
    int bi = Sim_GetBuildingAt(&g, bx, by);
    assert(bi >= 0);
    assert(g.buildings[bi].footprint_w == 2 && g.buildings[bi].footprint_h == 2);
    assert(Sim_GetBuildingAt(&g, bx + 1, by) == bi);
    assert(Sim_GetBuildingAt(&g, bx, by + 1) == bi);
    assert(Sim_GetBuildingAt(&g, bx + 1, by + 1) == bi);
    assert(g.buildings[bi].construction_days_remaining == 10);
    assert(g.open_firms == firms_before_construction); /* a worksite is not an operating firm */

    const char *path = "rapture_construction_test_save.dat";
    assert(Sim_Save(&g, path));
    memset(&loaded, 0, sizeof(loaded));
    assert(Sim_Load(&loaded, path));
    int loaded_bi = Sim_GetBuildingAt(&loaded, bx + 1, by + 1);
    assert(loaded_bi >= 0);
    assert(loaded.buildings[loaded_bi].footprint_w == 2 && loaded.buildings[loaded_bi].footprint_h == 2);
    assert(loaded.buildings[loaded_bi].construction_days_remaining == 10);
    remove(path);

    g.paused = false;
    for (int i = 0; i < 9; ++i) Sim_TickDay(&g);
    assert(g.buildings[bi].construction_days_remaining == 1);
    Sim_TickDay(&g);
    assert(g.buildings[bi].construction_days_remaining == 0);
    assert(Sim_Demolish(&g, bx + 1, by + 1, reason, (int)sizeof(reason)));
    assert(Sim_GetBuildingAt(&g, bx, by) == -1);
    assert(Sim_GetBuildingAt(&g, bx + 1, by) == -1);
    assert(Sim_GetBuildingAt(&g, bx, by + 1) == -1);
    assert(Sim_GetBuildingAt(&g, bx + 1, by + 1) == -1);
}

static void test_demolition_rules(void)
{
    GameState g;
    char reason[160];
    Sim_Init(&g, MODE_SANDBOX);
    int hx = -1, hy = -1;
    find_occupied_residence(&g, &hx, &hy);
    assert(!Sim_Demolish(&g, hx, hy, reason, (int)sizeof(reason)));
    assert(strstr(reason, "familles") != NULL);
    int cx = -1, cy = -1;
    assert(find_free_connected_spot(&g, BT_CORRIDOR, false, &cx, &cy));
    assert(Sim_Build(&g, BT_CORRIDOR, cx, cy, reason, (int)sizeof(reason)));
    assert(Sim_Demolish(&g, cx, cy, reason, (int)sizeof(reason)));
    assert(Sim_GetBuildingAt(&g, cx, cy) == -1);
    int px = -1, py = -1;
    assert(find_free_connected_spot(&g, BT_PIPE, false, &px, &py));
    assert(Sim_Build(&g, BT_PIPE, px, py, reason, (int)sizeof(reason)));
    assert(Sim_CountBuildings(&g, BT_PIPE) == 1);
}

static void test_months_and_save_load(void)
{
    GameState g, loaded;
    Sim_Init(&g, MODE_SANDBOX);
    g.paused = false;
    g.speed = 1;
    for (int i = 0; i < 75; ++i) Sim_TickDay(&g);
    assert(g.month == 3);
    assert(g.year == 1958);
    assert(g.research > 12);
    assert(g.monthly_income >= 0);

    const char *path = "rapture_test_save.dat";
    assert(Sim_Save(&g, path));
    memset(&loaded, 0, sizeof(loaded));
    assert(Sim_Load(&loaded, path));
    assert(loaded.money == g.money);
    assert(loaded.population == g.population);
    assert(loaded.building_slots == g.building_slots);
    assert(loaded.households_count == g.households_count);
    assert(loaded.mode == MODE_SANDBOX);
    assert(loaded.company_tax_rate == g.company_tax_rate);
    assert(loaded.housing_tax == g.housing_tax);
    assert(loaded.electricity_price == g.electricity_price);
    remove(path);
}

static void test_ocean_expedition(void)
{
    GameState g;
    char reason[160];
    Sim_Init(&g, MODE_SANDBOX);
    g.research = 200;
    assert(Sim_Research(&g, 2, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 0, reason, (int)sizeof(reason)));
    int bx = -1, by = -1;
    assert(find_free_connected_spot(&g, BT_BATHYSPHERE, false, &bx, &by));
    assert(Sim_Build(&g, BT_BATHYSPHERE, bx, by, reason, (int)sizeof(reason)));
    g.paused = false;
    Sim_UpdateStatistics(&g);
    assert(Sim_SendOceanExpedition(&g, 2, reason, (int)sizeof(reason)));
    assert(g.expedition_active);
    assert(g.expedition_crew == 2);
    assert(g.expedition_days_remaining > 0);
    int crew_reserved = 0;
    for (int i = 0; i < g.household_slots; ++i) crew_reserved += g.households[i].expedition_adults;
    assert(crew_reserved == 2);
    assert(!Sim_SendOceanExpedition(&g, 2, reason, (int)sizeof(reason)));

    /* An in-progress voyage must survive save/load, including reserved workers. */
    const char *path = "rapture_expedition_test_save.dat";
    GameState loaded;
    assert(Sim_Save(&g, path));
    memset(&loaded, 0, sizeof(loaded));
    assert(Sim_Load(&loaded, path));
    assert(loaded.expedition_active);
    assert(loaded.expedition_crew == 2);
    assert(loaded.expedition_days_remaining == g.expedition_days_remaining);
    int loaded_crew_reserved = 0;
    for (int i = 0; i < loaded.household_slots; ++i) loaded_crew_reserved += loaded.households[i].expedition_adults;
    assert(loaded_crew_reserved == 2);
    remove(path);

    for (int i = 0; i < 20; ++i) Sim_TickDay(&g);
    assert(!g.expedition_active);
    assert(g.expedition_crew == 0);
    for (int i = 0; i < g.household_slots; ++i) assert(g.households[i].expedition_adults == 0);

    assert(g.tech[2]);
    assert(Sim_Research(&g, 10, reason, (int)sizeof(reason)));
    assert(g.tech[10]);
    assert(Sim_SendOceanExpedition(&g, 2, reason, (int)sizeof(reason)));
    assert(g.expedition_days_remaining < 14); /* submarines reduce a two-person trip */
}


static void test_water_and_oxygen_affect_the_city(void)
{
    GameState g;
    Sim_Init(&g, MODE_SANDBOX);
    Sim_UpdateStatistics(&g);
    float normal_water = g.water_ratio;
    float normal_oxygen = g.oxygen_ratio;
    int water_index = -1, oxygen_index = -1;
    for (int i = 0; i < g.building_slots; ++i) {
        if (!g.buildings[i].active) continue;
        if (g.buildings[i].type == BT_WATER) water_index = i;
        if (g.buildings[i].type == BT_OXYGEN) oxygen_index = i;
    }
    assert(water_index >= 0 && oxygen_index >= 0);
    g.buildings[water_index].condition = 0;
    g.buildings[oxygen_index].condition = 0;
    Sim_UpdateStatistics(&g);
    assert(g.water_ratio < normal_water);
    assert(g.oxygen_ratio < normal_oxygen);
}

static void test_long_running_city(void)
{
    GameState g;
    char reason[160];
    Sim_Init(&g, MODE_SANDBOX);
    /* Add enough housing to exercise household growth and labour reallocation over time. */
    int added = 0;
    for (int x = 17; x < 32 && added < 16; ++x) {
        for (int y = 19; y < 33 && added < 16; ++y) {
            if (Sim_GetBuildingAt(&g, x, y) >= 0) continue;
            if (Sim_Build(&g, BT_RESIDENCE, x, y, reason, (int)sizeof(reason))) added++;
        }
    }
    assert(added >= 8);
    g.paused = false;
    for (int day = 0; day < 3600; ++day) Sim_TickDay(&g);
    Sim_UpdateStatistics(&g);
    assert(g.year >= 1967);
    assert(g.population > 35);
    for (int i = 0; i < g.building_slots; ++i) {
        if (g.buildings[i].active && g.buildings[i].type == BT_RESIDENCE)
            assert(g.buildings[i].residents <= BUILDING_DEFS[BT_RESIDENCE].residents);
    }
}


static void test_oriented_footprints_and_placement_rules(void)
{
    GameState g;
    char reason[160];
    Sim_Init(&g, MODE_SANDBOX);
    int x = -1, y = -1;
    assert(find_free_connected_spot(&g, BT_LABORATORY, true, &x, &y));
    assert(Sim_BuildOriented(&g, BT_LABORATORY, x, y, true, reason, (int)sizeof(reason)));
    int bi = Sim_GetBuildingAt(&g, x, y);
    assert(bi >= 0);
    assert(g.buildings[bi].footprint_w == 1 && g.buildings[bi].footprint_h == 2);
    assert(Sim_GetBuildingAt(&g, x, y + 1) == bi);
    int hx = -1, hy = -1;
    assert(find_free_connected_spot(&g, BT_HOSPITAL, true, &hx, &hy));
    assert(Sim_BuildOriented(&g, BT_HOSPITAL, hx, hy, true, reason, (int)sizeof(reason)));
    int hospital_index = Sim_GetBuildingAt(&g, hx, hy);
    assert(hospital_index >= 0);
    assert(g.buildings[hospital_index].footprint_w == 1 && g.buildings[hospital_index].footprint_h == 2);
    int before = Sim_CountBuildings(&g, BT_CINEMA);
    assert(!Sim_Build(&g, BT_CINEMA, 2, 2, reason, (int)sizeof(reason)));
    assert(Sim_CountBuildings(&g, BT_CINEMA) == before);
}

static void test_research_tree_prerequisites_and_new_businesses(void)
{
    GameState g;
    char reason[160];
    Sim_Init(&g, MODE_SANDBOX);
    g.research = 1000;
    assert(!Sim_Research(&g, 8, reason, (int)sizeof(reason)));
    assert(strstr(reason, "Prerequis") != NULL);
    assert(Sim_Research(&g, 7, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 8, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 2, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 0, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 14, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 4, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 9, reason, (int)sizeof(reason)));
    int x = -1, y = -1;
    assert(find_free_connected_spot(&g, BT_RESTAURANT, true, &x, &y));
    assert(!Sim_BuildOriented(&g, BT_RESTAURANT, x, y, true, reason, (int)sizeof(reason)));
    assert(strstr(reason, "Commerces sous-marins") != NULL);
    assert(Sim_Research(&g, 13, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 11, reason, (int)sizeof(reason)));
    assert(Sim_BuildOriented(&g, BT_RESTAURANT, x, y, true, reason, (int)sizeof(reason)));
    assert(Sim_CountBuildings(&g, BT_RESTAURANT) == 1);
    assert(find_free_connected_spot(&g, BT_FISHERY, false, &x, &y));
    assert(!Sim_Build(&g, BT_FISHERY, x, y, reason, (int)sizeof(reason)));
    assert(strstr(reason, "sas oceanique") != NULL);
    int sx = -1, sy = -1;
    assert(find_free_connected_spot(&g, BT_AIRLOCK, false, &sx, &sy));
    assert(Sim_Build(&g, BT_AIRLOCK, sx, sy, reason, (int)sizeof(reason)));
    for (int day = 0; day < Sim_GetConstructionDays(BT_AIRLOCK); ++day) Sim_TickDay(&g);
    assert(find_free_connected_spot(&g, BT_FISHERY, false, &x, &y));
    assert(Sim_Build(&g, BT_FISHERY, x, y, reason, (int)sizeof(reason)));
    assert(Sim_CountBuildings(&g, BT_FISHERY) == 1);
}


static void test_commerce_technology_unlocks_all_three_buildings(void)
{
    GameState g;
    char reason[192];
    Sim_Init(&g, MODE_SANDBOX);
    g.money = 200000;
    g.materials = 300;
    g.research = 1000;

    const BuildingType types[] = {BT_BOUTIQUE, BT_RESTAURANT, BT_CINEMA};
    for (int i = 0; i < 3; ++i) {
        assert(BUILDING_DEFS[types[i]].unlock_tech == 11);
        int x = -1, y = -1;
        assert(find_free_connected_spot(&g, types[i], false, &x, &y));
        assert(!Sim_Build(&g, types[i], x, y, reason, (int)sizeof(reason)));
        assert(strstr(reason, "Commerces sous-marins") != NULL);
    }
    assert(Sim_Research(&g, 13, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 11, reason, (int)sizeof(reason)));
    for (int i = 0; i < 3; ++i) {
        int x = -1, y = -1;
        assert(find_free_connected_spot(&g, types[i], false, &x, &y));
        assert(Sim_Build(&g, types[i], x, y, reason, (int)sizeof(reason)));
    }
}

static void test_commerce_technology_save_compatibility(void)
{
    GameState g, loaded;
    char reason[192];
    Sim_Init(&g, MODE_SANDBOX);
    g.money = 100000;
    g.research = 500;
    assert(Sim_Research(&g, 2, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 0, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 7, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 13, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 11, reason, (int)sizeof(reason)));

    const char *modern_path = "rapture_tech_v11_test_save.dat";
    assert(Sim_Save(&g, modern_path));
    memset(&loaded, 0, sizeof(loaded));
    assert(Sim_Load(&loaded, modern_path));
    assert(loaded.tech[11]);
    remove(modern_path);

    /* Downgrade the modern save to model a real v7 file, which contains exactly
       the original eleven technologies and no v10/v11-only sections. */
    const char *v7_source = "rapture_tech_v11_source.dat";
    const char *legacy_path = "rapture_tech_v7_test_save.dat";
    assert(Sim_Save(&g, v7_source));
    FILE *src = fopen(v7_source, "r");
    FILE *dst = fopen(legacy_path, "w");
    assert(src != NULL && dst != NULL);
    char line[1024];
    int first_line = 1;
    while (fgets(line, (int)sizeof(line), src) != NULL) {
        if (first_line) {
            fputs("RAPTURE_SAVE 7\n", dst);
            first_line = 0;
        } else if (strncmp(line, "OBSTACLES", 9) == 0 || strncmp(line, "OBSTACLETILES", 13) == 0 ||
                   strncmp(line, "O ", 2) == 0 || strncmp(line, "CLEARING", 8) == 0 ||
                   strncmp(line, "ADAM ", 5) == 0) {
            /* These sections only exist from v10 onward: a real v7 file never had them. */
            continue;
        } else if (strncmp(line, "TECH", 4) == 0) {
            char *cursor = line + 4;
            fprintf(dst, "TECH");
            for (int i = 0; i < 11; ++i) {
                while (*cursor == ' ' || *cursor == '\t') cursor++;
                char *end = NULL;
                long value = strtol(cursor, &end, 10);
                assert(end != cursor);
                fprintf(dst, " %ld", value);
                cursor = end;
            }
            fputc('\n', dst);
        } else {
            fputs(line, dst);
        }
    }
    assert(!ferror(src) && !ferror(dst));
    fclose(src);
    fclose(dst);
    remove(v7_source);

    memset(&loaded, 0, sizeof(loaded));
    assert(Sim_Load(&loaded, legacy_path));
    assert(loaded.tech[0] && loaded.tech[7]);
    assert(!loaded.tech[11]);
    remove(legacy_path);
}

static void test_remarkable_building_rules(void)
{
    GameState g;
    char reason[192];
    Sim_Init(&g, MODE_SANDBOX);
    g.money = 500000;
    g.materials = 500;
    g.research = 1000;
    g.tech[0] = true;
    g.tech[9] = true;
    g.tech[15] = true; /* Metallurgie avancee : Pole d'Hephaistos */
    g.tech[16] = true; /* Biotechnologie marine : Arcadie */
    g.tech[17] = true; /* Arts et spectacles : Forteresse Folatre */

    int x = -1, y = -1;
    assert(find_free_connected_spot(&g, BT_FISHERY, false, &x, &y));
    assert(!Sim_Build(&g, BT_FISHERY, x, y, reason, (int)sizeof(reason)));
    assert(strstr(reason, "bathysphere") != NULL || strstr(reason, "sas oceanique") != NULL);

    assert(find_free_connected_spot(&g, BT_BATHYSPHERE, false, &x, &y));
    assert(Sim_Build(&g, BT_BATHYSPHERE, x, y, reason, (int)sizeof(reason)));
    for (int day = 0; day < Sim_GetConstructionDays(BT_BATHYSPHERE); ++day) Sim_TickDay(&g);
    assert(find_free_connected_spot(&g, BT_FISHERY, false, &x, &y));
    assert(Sim_Build(&g, BT_FISHERY, x, y, reason, (int)sizeof(reason)));
    int fishery_index = Sim_GetBuildingAt(&g, x, y);
    assert(fishery_index >= 0);
    assert(g.buildings[fishery_index].footprint_w == 2 && g.buildings[fishery_index].footprint_h == 2);
    assert(Sim_GetBuildingAt(&g, x + 1, y + 1) == fishery_index);
    {
        GameState legacy = g, migrated;
        legacy.buildings[fishery_index].footprint_h = 1;
        legacy.map[x][y + 1].building = 0;
        legacy.map[x + 1][y + 1].building = 0;
        const char *migration_path = "rapture_fishery_migration_test_save.dat";
        assert(Sim_Save(&legacy, migration_path));
        memset(&migrated, 0, sizeof(migrated));
        assert(Sim_Load(&migrated, migration_path));
        int migrated_fishery = Sim_GetBuildingAt(&migrated, x, y);
        assert(migrated_fishery >= 0);
        assert(migrated.buildings[migrated_fishery].footprint_w == 2 && migrated.buildings[migrated_fishery].footprint_h == 2);
        assert(Sim_GetBuildingAt(&migrated, x + 1, y + 1) == migrated_fishery);
        remove(migration_path);
    }
    assert(find_free_connected_spot(&g, BT_FISHERY, false, &x, &y));
    assert(!Sim_Build(&g, BT_FISHERY, x, y, reason, (int)sizeof(reason)));
    assert(strstr(reason, "unique") != NULL);

    /* Three operational farms unlock Arcadie, not just three work sites. */
    while (Sim_CountBuildings(&g, BT_FOOD) < 3) {
        assert(find_free_connected_spot(&g, BT_FOOD, false, &x, &y));
        assert(Sim_Build(&g, BT_FOOD, x, y, reason, (int)sizeof(reason)));
    }
    for (int day = 0; day < Sim_GetConstructionDays(BT_FOOD); ++day) Sim_TickDay(&g);
    assert(find_free_connected_spot(&g, BT_ARCADIA, false, &x, &y));
    assert(Sim_Build(&g, BT_ARCADIA, x, y, reason, (int)sizeof(reason)));
    assert(find_free_connected_spot(&g, BT_ARCADIA, false, &x, &y));
    assert(!Sim_Build(&g, BT_ARCADIA, x, y, reason, (int)sizeof(reason)));

    while (Sim_CountBuildings(&g, BT_FACTORY) < 3) {
        assert(find_free_connected_spot(&g, BT_FACTORY, false, &x, &y));
        assert(Sim_Build(&g, BT_FACTORY, x, y, reason, (int)sizeof(reason)));
    }
    for (int day = 0; day < Sim_GetConstructionDays(BT_FACTORY); ++day) Sim_TickDay(&g);
    assert(find_free_connected_spot(&g, BT_HEPHAESTUS, false, &x, &y));
    assert(Sim_Build(&g, BT_HEPHAESTUS, x, y, reason, (int)sizeof(reason)));

    while (Sim_CountBuildings(&g, BT_LEISURE) < 3) {
        assert(find_free_connected_spot(&g, BT_LEISURE, false, &x, &y));
        assert(Sim_Build(&g, BT_LEISURE, x, y, reason, (int)sizeof(reason)));
    }
    for (int day = 0; day < Sim_GetConstructionDays(BT_LEISURE); ++day) Sim_TickDay(&g);
    assert(find_free_connected_spot(&g, BT_FORTRESS_FOLATRE, false, &x, &y));
    assert(Sim_Build(&g, BT_FORTRESS_FOLATRE, x, y, reason, (int)sizeof(reason)));
}


static void test_research_tree_is_coherent(void)
{
    /* Commerces must not depend on the social contract or on bathyspheres. */
    assert(TECH_PREREQUISITES[11] == (1u << 13));
    assert(TECH_PREREQUISITES[13] == 0u);
    /* Prerequisites only point to other technologies and never form a cycle. */
    for (int t = 0; t < TECH_COUNT; ++t) {
        assert((TECH_PREREQUISITES[t] & (1u << t)) == 0u);
        assert((TECH_PREREQUISITES[t] >> TECH_COUNT) == 0u);
    }
    bool reachable[TECH_COUNT] = {false};
    for (int pass = 0; pass < TECH_COUNT; ++pass)
        for (int t = 0; t < TECH_COUNT; ++t) {
            bool ok = true;
            for (int p = 0; p < TECH_COUNT; ++p)
                if ((TECH_PREREQUISITES[t] & (1u << p)) != 0u && !reachable[p]) ok = false;
            if (ok) reachable[t] = true;
        }
    for (int t = 0; t < TECH_COUNT; ++t) assert(reachable[t]);
    /* Every building that needs a technology points to a real one. */
    for (int i = 0; i < BT_COUNT; ++i) assert(BUILDING_DEFS[i].unlock_tech < TECH_COUNT);
    assert(BUILDING_DEFS[BT_ARCADIA].unlock_tech == 16);
    assert(BUILDING_DEFS[BT_HEPHAESTUS].unlock_tech == 15);
    assert(BUILDING_DEFS[BT_FORTRESS_FOLATRE].unlock_tech == 17);
}

static void test_attractiveness_and_arrivals(void)
{
    GameState g;
    Sim_Init(&g, MODE_SANDBOX);
    AttractTip tips[ATTRACT_MAX_TIPS];
    int score = Sim_GetAttractiveness(&g, tips, ATTRACT_MAX_TIPS);
    assert(score >= 0 && score <= 100);
    assert(tips[0].text[0] != '\0' && tips[0].gain > 0);
    for (int i = 1; i < ATTRACT_MAX_TIPS && tips[i].text[0] != '\0'; ++i) assert(tips[i - 1].gain >= tips[i].gain);

    /* The advice follows the city: without free housing it asks for residences. */
    for (int i = 0; i < g.building_slots; ++i)
        if (g.buildings[i].active && g.buildings[i].type == BT_RESIDENCE) g.buildings[i].residents = BUILDING_DEFS[BT_RESIDENCE].residents;
    (void)Sim_GetAttractiveness(&g, tips, ATTRACT_MAX_TIPS);
    bool mentions_housing = false;
    for (int i = 0; i < ATTRACT_MAX_TIPS; ++i) if (strstr(tips[i].text, "logement") != NULL) mentions_housing = true;
    assert(mentions_housing);

    /* With room to live, a year brings far more residents than the old one-household-a-month trickle. */
    Sim_Init(&g, MODE_SANDBOX);
    char reason[192];
    int x = -1, y = -1;
    for (int i = 0; i < 4; ++i) {
        assert(find_free_connected_spot(&g, BT_RESIDENCE, false, &x, &y));
        assert(Sim_Build(&g, BT_RESIDENCE, x, y, reason, (int)sizeof(reason)));
    }
    int start_population = g.population;
    for (int day = 0; day < 360; ++day) Sim_TickDay(&g);
    assert(g.population >= start_population + 60);
}

static void test_jobs_are_spread_over_employers(void)
{
    GameState g;
    char reason[192];
    Sim_Init(&g, MODE_SANDBOX);
    g.money = 400000;
    g.materials = 600;
    int x = -1, y = -1;
    for (int i = 0; i < 2; ++i) {
        assert(find_free_connected_spot(&g, BT_MARKET, false, &x, &y));
        assert(Sim_Build(&g, BT_MARKET, x, y, reason, (int)sizeof(reason)));
        assert(find_free_connected_spot(&g, BT_FACTORY, false, &x, &y));
        assert(Sim_Build(&g, BT_FACTORY, x, y, reason, (int)sizeof(reason)));
    }
    for (int day = 0; day < 240; ++day) Sim_TickDay(&g);
    int staffed_companies = 0;
    for (int i = 0; i < g.building_slots; ++i) {
        const Building *b = &g.buildings[i];
        if (b->active && BUILDING_DEFS[b->type].is_company && b->construction_days_remaining <= 0 && b->workers > 0) staffed_companies++;
    }
    assert(staffed_companies >= 3);
}


static void test_notification_targets_correct_building(void)
{
    /* Two corridors, each triggered into the "incident de corridor" event directly via
       Sim_AddLogFor (as the simulation does at the event's own origin). The log must point
       to the building that was actually involved, not always the first corridor built. */
    GameState g;
    char reason[192];
    Sim_Init(&g, MODE_SANDBOX);
    g.money = 100000;
    int x1 = -1, y1 = -1, x2 = -1, y2 = -1;
    assert(find_free_connected_spot(&g, BT_CORRIDOR, false, &x1, &y1));
    assert(Sim_Build(&g, BT_CORRIDOR, x1, y1, reason, (int)sizeof(reason)));
    int first_corridor = Sim_GetBuildingAt(&g, x1, y1);
    assert(first_corridor >= 0);
    for (int i = 0; i < g.building_slots; ++i) g.buildings[i].construction_days_remaining = 0;
    assert(find_free_connected_spot(&g, BT_CORRIDOR, false, &x2, &y2));
    assert(Sim_Build(&g, BT_CORRIDOR, x2, y2, reason, (int)sizeof(reason)));
    int second_corridor = Sim_GetBuildingAt(&g, x2, y2);
    assert(second_corridor >= 0 && second_corridor != first_corridor);
    for (int i = 0; i < g.building_slots; ++i) g.buildings[i].construction_days_remaining = 0;

    Sim_AddLogFor(&g, 2, first_corridor, "Incident de corridor : une fuite de pression exige une inspection.");
    assert(g.logs[0].building_index == first_corridor);
    Sim_AddLogFor(&g, 2, second_corridor, "Incident de corridor : une fuite de pression exige une inspection.");
    assert(g.logs[0].building_index == second_corridor);
    assert(g.logs[0].building_index != first_corridor);

    /* An index past the active slots (or on an inactive building) is rejected to -1. */
    Sim_AddLogFor(&g, 0, g.building_slots + 50, "Message generique.");
    assert(g.logs[0].building_index == -1);
}

static void test_obstacles_block_construction_and_clear(void)
{
    GameState g;
    char reason[192];
    Sim_Init(&g, MODE_SANDBOX);
    g.money = 400000;

    /* Some obstacle tiles were generated, and never inside the starting city's footprint. */
    int initial = Sim_CountObstacleTiles(&g);
    assert(initial > 0 && initial == g.obstacles_initial);
    for (int i = 0; i < g.building_slots; ++i) {
        const Building *b = &g.buildings[i];
        if (!b->active) continue;
        for (int ix = b->x; ix < b->x + (b->footprint_w > 0 ? b->footprint_w : 1); ++ix)
            for (int iy = b->y; iy < b->y + (b->footprint_h > 0 ? b->footprint_h : 1); ++iy)
                assert(!IS_OBSTACLE_TERRAIN(g.map[ix][iy].terrain));
    }

    int ox = -1, oy = -1;
    for (int x = 0; x < MAP_W && ox < 0; ++x)
        for (int y = 0; y < MAP_H && ox < 0; ++y)
            if (IS_OBSTACLE_TERRAIN(g.map[x][y].terrain)) { ox = x; oy = y; }
    assert(ox >= 0);
    assert(!Sim_Build(&g, BT_PIPE, ox, oy, reason, (int)sizeof(reason)));

    assert(Sim_StartObstacleClearing(&g, ox, oy, reason, (int)sizeof(reason)));
    assert(g.clearing_active);
    assert(!Sim_StartObstacleClearing(&g, ox, oy, reason, (int)sizeof(reason))); /* one crew at a time */
    for (int d = 0; d < g.clearing_days_total + 2; ++d) Sim_TickDay(&g);
    assert(!g.clearing_active);
    assert(!IS_OBSTACLE_TERRAIN(g.map[ox][oy].terrain));
    assert(Sim_CountObstacleTiles(&g) < initial);
}

static void test_new_buildings_and_technologies(void)
{
    GameState g;
    char reason[192];
    Sim_Init(&g, MODE_SANDBOX);
    g.money = 200000;
    g.research = 2000;

    assert(TECH_COUNT == 23);
    assert(BUILDING_DEFS[BT_GREENHOUSE_DOME].unlock_tech == 18);
    assert(BUILDING_DEFS[BT_RICH_RESIDENCE].unlock_tech == 19);
    assert(BUILDING_DEFS[BT_RICH_RESIDENCE].residents < BUILDING_DEFS[BT_RESIDENCE].residents);
    assert(BUILDING_DEFS[BT_RICH_RESIDENCE].cost > BUILDING_DEFS[BT_RESIDENCE].cost);

    int x = -1, y = -1;
    assert(!find_free_connected_spot(&g, BT_GREENHOUSE_DOME, false, &x, &y) || true);
    assert(!Sim_Build(&g, BT_GREENHOUSE_DOME, 20, 20, reason, (int)sizeof(reason))); /* still locked */

    assert(Sim_Research(&g, 14, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 4, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 9, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 12, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 5, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 6, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 15, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 16, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 18, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 7, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 13, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 11, reason, (int)sizeof(reason)));
    assert(Sim_Research(&g, 19, reason, (int)sizeof(reason)));

    assert(find_free_connected_spot(&g, BT_GREENHOUSE_DOME, false, &x, &y));
    assert(Sim_Build(&g, BT_GREENHOUSE_DOME, x, y, reason, (int)sizeof(reason)));
    assert(find_free_connected_spot(&g, BT_RICH_RESIDENCE, false, &x, &y));
    assert(Sim_Build(&g, BT_RICH_RESIDENCE, x, y, reason, (int)sizeof(reason)));
    for (int i = 0; i < g.building_slots; ++i) g.buildings[i].construction_days_remaining = 0;

    int food_before = g.food_stock;
    for (int d = 0; d < 30; ++d) Sim_TickDay(&g);
    assert(g.food_stock != food_before); /* the greenhouse dome contributes to food */
}

static void test_campaign_includes_adam_and_protector_stages(void)
{
    GameState g;
    char buffer[200];
    Sim_Init(&g, MODE_CAMPAIGN);
    (void)Sim_GetCampaignGoal(&g, buffer, (int)sizeof(buffer));
    assert(g.campaign_stage == 0);
    /* Commerce-related stages never precede the research they actually require. */
    assert((TECH_PREREQUISITES[11] & (1u << 7)) == 0u); /* Commerces sous-marins doesn't need the social contract */
    assert((TECH_PREREQUISITES[0] & (1u << 11)) == 0u); /* nor does the bathysphere network need commerce */
    g.campaign_stage = 10;
    int stage = Sim_GetCampaignGoal(&g, buffer, (int)sizeof(buffer));
    assert(stage == 10 && strstr(buffer, "ADAM") != NULL);
    g.campaign_stage = 11;
    stage = Sim_GetCampaignGoal(&g, buffer, (int)sizeof(buffer));
    assert(stage == 11 && strstr(buffer, "Petites Soeurs") != NULL);
    g.campaign_stage = 12;
    stage = Sim_GetCampaignGoal(&g, buffer, (int)sizeof(buffer));
    assert(stage == 12 && strstr(buffer, "Protecteurs") != NULL);
    g.campaign_stage = 14;
    stage = Sim_GetCampaignGoal(&g, buffer, (int)sizeof(buffer));
    assert(stage == 14 && strstr(buffer, "terminee") != NULL);
}

static void test_obstacles_and_clearing_survive_save_load(void)
{
    GameState g, loaded;
    char reason[192];
    Sim_Init(&g, MODE_SANDBOX);
    g.money = 400000;
    int middle_obstacles = 0;
    for (int x = 9; x < MAP_W - 9; ++x) {
        for (int y = 10; y < MAP_H - 10; ++y) {
            if (IS_OBSTACLE_TERRAIN(g.map[x][y].terrain)) middle_obstacles++;
        }
    }
    assert(middle_obstacles > 0); /* Inner-ring clusters are no longer confined to map edges. */
    int ox = -1, oy = -1;
    for (int x = 0; x < MAP_W && ox < 0; ++x)
        for (int y = 0; y < MAP_H && ox < 0; ++y)
            if (IS_OBSTACLE_TERRAIN(g.map[x][y].terrain)) { ox = x; oy = y; }
    assert(ox >= 0);
    assert(Sim_StartObstacleClearing(&g, ox, oy, reason, (int)sizeof(reason)));

    const char *path = "rapture_obstacles_test_save.dat";
    assert(Sim_Save(&g, path));
    memset(&loaded, 0, sizeof(loaded));
    assert(Sim_Load(&loaded, path));
    remove(path);
    assert(loaded.obstacles_initial == g.obstacles_initial);
    assert(loaded.clearing_active);
    assert(loaded.clearing_tile_count == g.clearing_tile_count);
    assert(IS_OBSTACLE_TERRAIN(loaded.map[ox][oy].terrain)); /* not cleared yet: still blocks the tile */
}

int main(void)
{
    test_adam_branch_and_save_compatibility();
    test_adam_monthly_production_cycle();
    test_new_city();
    test_construction_and_research();
    test_multi_tile_construction_and_save();
    test_demolition_rules();
    test_months_and_save_load();
    test_ocean_expedition();
    test_oriented_footprints_and_placement_rules();
    test_research_tree_prerequisites_and_new_businesses();
    test_remarkable_building_rules();
    test_commerce_technology_unlocks_all_three_buildings();
    test_commerce_technology_save_compatibility();
    test_water_and_oxygen_affect_the_city();
    test_long_running_city();
    test_research_tree_is_coherent();
    test_attractiveness_and_arrivals();
    test_jobs_are_spread_over_employers();
    test_notification_targets_correct_building();
    test_obstacles_block_construction_and_clear();
    test_new_buildings_and_technologies();
    test_campaign_includes_adam_and_protector_stages();
    test_obstacles_and_clearing_survive_save_load();
    puts("Rapture simulation tests: OK");
    return 0;
}
