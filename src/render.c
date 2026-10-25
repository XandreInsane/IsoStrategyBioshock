#include "render.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#define TOP_H 64
#define FOOT_H 40
#define LEFT_W 258
#define RIGHT_W 286
#define TILE_W 52.0f
#define TILE_H 27.0f

typedef struct Layout {
    Rectangle viewport;
    Rectangle left_panel;
    Rectangle right_panel;
    int screen_w;
    int screen_h;
} Layout;

static const Color C_BG = {7, 18, 33, 255};
static const Color C_PANEL = {12, 29, 47, 255};
static const Color C_PANEL_2 = {17, 42, 61, 255};
static const Color C_EDGE = {47, 92, 109, 255};
static const Color C_TEXT = {222, 230, 221, 255};
static const Color C_MUTED = {135, 161, 169, 255};
static const Color C_GOLD = {227, 183, 100, 255};
static const Color C_TEAL = {62, 204, 191, 255};
static const Color C_RED = {221, 91, 82, 255};
static const Color C_GREEN = {115, 194, 133, 255};

static Layout get_layout(void)
{
    Layout l;
    l.screen_w = GetScreenWidth();
    l.screen_h = GetScreenHeight();
    l.left_panel = (Rectangle){0, TOP_H, (float)LEFT_W, (float)(l.screen_h - TOP_H - FOOT_H)};
    l.right_panel = (Rectangle){(float)(l.screen_w - RIGHT_W), TOP_H, (float)RIGHT_W, (float)(l.screen_h - TOP_H - FOOT_H)};
    l.viewport = (Rectangle){LEFT_W, TOP_H, (float)(l.screen_w - LEFT_W - RIGHT_W), (float)(l.screen_h - TOP_H - FOOT_H)};
    return l;
}

static bool point_in(Vector2 p, Rectangle r) { return CheckCollisionPointRec(p, r); }
static int min_int(int a, int b) { return a < b ? a : b; }
static int max_int(int a, int b) { return a > b ? a : b; }
static float clamp_float(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static float max_float(float a, float b) { return a > b ? a : b; }
static float min_float(float a, float b) { return a < b ? a : b; }
static int clamp_int(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

/* Small, non-intrusive scroll indicator used by every long list. Scrolling still
   uses the mouse wheel; the thumb simply makes the current position visible. */
static void draw_scrollbar(float x, float y, float width, float height,
                           int total_rows, int visible_rows, int scroll)
{
    if (total_rows <= visible_rows || visible_rows <= 0 || height <= 8.0f) return;
    float thumb_h = max_float(18.0f, height * (float)visible_rows / (float)total_rows);
    if (thumb_h > height) thumb_h = height;
    int max_scroll = max_int(1, total_rows - visible_rows);
    float thumb_y = y + (height - thumb_h) * (float)clamp_int(scroll, 0, max_scroll) / (float)max_scroll;
    DrawRectangleRounded((Rectangle){x, y, width, height}, 0.35f, 3, (Color){28, 48, 62, 210});
    DrawRectangleRounded((Rectangle){x, thumb_y, width, thumb_h}, 0.35f, 3, (Color){111, 132, 143, 230});
}

static Color color_alpha(Color c, unsigned char a) { c.a = a; return c; }

static void open_save_menu(GameUI *ui, GameState *g, bool is_load, bool from_title, bool from_pause);
static void draw_save_load_modal(GameUI *ui, GameState *g);
static void open_news_history(GameUI *ui, GameState *g);
static Rectangle toast_rectangle(const GameUI *ui);

static Color building_color(BuildingType t)
{
    static const Color palette[BT_COUNT] = {
        {46, 105, 120, 255}, {42, 116, 126, 255}, {180, 132, 72, 255}, {111, 91, 75, 255},
        {91, 129, 150, 255}, {133, 80, 61, 255}, {57, 118, 136, 255}, {57, 137, 146, 255},
        {69, 126, 103, 255}, {181, 190, 166, 255}, {153, 103, 125, 255}, {180, 145, 80, 255},
        {79, 129, 154, 255}, {119, 145, 151, 255}, {92, 128, 111, 255}, {163, 87, 118, 255},
        {84, 101, 119, 255}, {145, 91, 49, 255},
        {77, 153, 177, 255}, {155, 86, 64, 255}, {190, 151, 83, 255}, {42, 159, 165, 255},
        {78, 151, 102, 255}, {201, 89, 49, 255}, {133, 75, 155, 255},
        {52, 100, 68, 255}, {176, 150, 104, 255}
    };
    if (t < 0 || t >= BT_COUNT) return C_TEXT;
    return palette[t];
}

static Color shade(Color c, int amount)
{
    c.r = (unsigned char)clamp_int((int)c.r + amount, 0, 255);
    c.g = (unsigned char)clamp_int((int)c.g + amount, 0, 255);
    c.b = (unsigned char)clamp_int((int)c.b + amount, 0, 255);
    return c;
}

static bool tech_locked(const GameState *g, BuildingType t)
{
    int tech = BUILDING_DEFS[t].unlock_tech;
    return tech >= 0 && !g->tech[tech];
}

static int render_operational_count(const GameState *g, BuildingType type)
{
    int count = 0;
    for (int i = 0; i < g->building_slots; ++i) {
        const Building *b = &g->buildings[i];
        if (b->active && !b->closed && b->construction_days_remaining <= 0 && b->type == type) count++;
    }
    return count;
}

static bool render_special_requirement(const GameState *g, BuildingType type, char *reason, size_t reason_size)
{
    if (reason && reason_size > 0) reason[0] = '\0';
    if (type == BT_FISHERY || type == BT_ARCADIA || type == BT_HEPHAESTUS || type == BT_FORTRESS_FOLATRE) {
        if (Sim_CountBuildings(g, type) > 0) {
            if (reason && reason_size > 0) (void)snprintf(reason, reason_size, "Batiment remarquable unique : deja construit.");
            return false;
        }
    }
    if (type == BT_FISHERY) {
        if (!g->tech[9]) {
            if (reason && reason_size > 0) (void)snprintf(reason, reason_size, "Recherche requise : Aquaculture oceanique.");
            return false;
        }
        if (render_operational_count(g, BT_AIRLOCK) <= 0 && render_operational_count(g, BT_BATHYSPHERE) <= 0) {
            if (reason && reason_size > 0) (void)snprintf(reason, reason_size, "Necessite un sas ou une bathysphere operationnelle.");
            return false;
        }
    } else if (type == BT_ARCADIA && render_operational_count(g, BT_FOOD) < 3) {
        if (reason && reason_size > 0) (void)snprintf(reason, reason_size, "Necessite 3 fermes marines operationnelles.");
        return false;
    } else if (type == BT_HEPHAESTUS && render_operational_count(g, BT_FACTORY) < 3) {
        if (reason && reason_size > 0) (void)snprintf(reason, reason_size, "Necessite 3 usines operationnelles.");
        return false;
    } else if (type == BT_FORTRESS_FOLATRE && render_operational_count(g, BT_LEISURE) < 3) {
        if (reason && reason_size > 0) (void)snprintf(reason, reason_size, "Necessite 3 salles de fete operationnelles.");
        return false;
    }
    return true;
}

static void draw_diamond(Vector2 c, float w, float h, Color top, Color left, Color right, bool outline)
{
    Vector2 p_top = {c.x, c.y - h * 0.5f};
    Vector2 p_right = {c.x + w * 0.5f, c.y};
    Vector2 p_bottom = {c.x, c.y + h * 0.5f};
    Vector2 p_left = {c.x - w * 0.5f, c.y};
    DrawTriangle(p_top, p_left, p_bottom, left);
    DrawTriangle(p_top, p_bottom, p_right, right);
    if (outline) {
        DrawLineV(p_top, p_right, color_alpha((Color){101, 156, 169, 255}, 85));
        DrawLineV(p_right, p_bottom, color_alpha((Color){101, 156, 169, 255}, 85));
        DrawLineV(p_bottom, p_left, color_alpha((Color){101, 156, 169, 255}, 85));
        DrawLineV(p_left, p_top, color_alpha((Color){101, 156, 169, 255}, 85));
    }
    (void)top;
}

static Vector2 tile_to_screen(const GameUI *ui, Rectangle vp, float x, float y)
{
    float w = TILE_W * ui->zoom;
    float h = TILE_H * ui->zoom;
    Vector2 p;
    p.x = vp.x + vp.width * 0.5f + ui->camera_x + (float)(x - y) * w * 0.5f;
    p.y = vp.y + vp.height * 0.47f + ui->camera_y + (float)(x + y - (MAP_W + MAP_H) / 2) * h * 0.5f;
    return p;
}

static bool screen_to_tile(const GameUI *ui, Rectangle vp, Vector2 m, int *out_x, int *out_y)
{
    if (!point_in(m, vp)) return false;
    float w = TILE_W * ui->zoom * 0.5f;
    float h = TILE_H * ui->zoom * 0.5f;
    if (w < 1.0f || h < 1.0f) return false;
    float dx = (m.x - (vp.x + vp.width * 0.5f + ui->camera_x)) / w;
    float dy = (m.y - (vp.y + vp.height * 0.47f + ui->camera_y)) / h;
    float sum = dy + (float)((MAP_W + MAP_H) / 2);
    int x = (int)floorf((sum + dx) * 0.5f + 0.5f);
    int y = (int)floorf((sum - dx) * 0.5f + 0.5f);
    if (x < 0 || y < 0 || x >= MAP_W || y >= MAP_H) return false;
    *out_x = x; *out_y = y;
    return true;
}

static void footprint_corners(const GameUI *ui, Rectangle vp, int x, int y, int fw, int fh,
                              Vector2 *a, Vector2 *b, Vector2 *c, Vector2 *d)
{
    /* Project the four true grid-boundary corners. Enlarging a single diamond
       cannot represent a 2x1 footprint: it skews the building away from its cells. */
    *a = tile_to_screen(ui, vp, (float)x - 0.5f, (float)y - 0.5f);
    *b = tile_to_screen(ui, vp, (float)(x + fw) - 0.5f, (float)y - 0.5f);
    *c = tile_to_screen(ui, vp, (float)(x + fw) - 0.5f, (float)(y + fh) - 0.5f);
    *d = tile_to_screen(ui, vp, (float)x - 0.5f, (float)(y + fh) - 0.5f);
}

static void draw_footprint_shape(const GameUI *ui, Rectangle vp, int x, int y, int fw, int fh,
                                 Color fill, Color outline, float line_width)
{
    Vector2 a, b, c, d;
    footprint_corners(ui, vp, x, y, fw, fh, &a, &b, &c, &d);
    if (fill.a > 0) {
        DrawTriangle(a, b, c, fill);
        DrawTriangle(a, c, d, fill);
    }
    if (outline.a > 0) {
        DrawLineEx(a, b, line_width, outline);
        DrawLineEx(b, c, line_width, outline);
        DrawLineEx(c, d, line_width, outline);
        DrawLineEx(d, a, line_width, outline);
    }
}

static bool render_footprint_clear(const GameState *g, BuildingType type, int x, int y, bool rotate)
{
    int fw = 1, fh = 1;
    Sim_GetDefaultFootprint(type, &fw, &fh);
    if (rotate) { int tmp = fw; fw = fh; fh = tmp; }
    if (x < 0 || y < 0 || x + fw > MAP_W || y + fh > MAP_H) return false;
    if (!render_special_requirement(g, type, NULL, 0)) return false;
    for (int ix = x; ix < x + fw; ++ix)
        for (int iy = y; iy < y + fh; ++iy)
            if (g->map[ix][iy].building != 0) return false;
    return true;
}

static void draw_ocean(Rectangle vp)
{
    DrawRectangleGradientV((int)vp.x, (int)vp.y, (int)vp.width, (int)vp.height,
                           (Color){9, 38, 57, 255}, (Color){4, 12, 26, 255});
    /* Distant shafts of filtered light and sparse particulate matter. */
    for (int i = 0; i < 7; ++i) {
        float x = vp.x + vp.width * ((float)((i * 37 + 13) % 97) / 100.0f);
        Vector2 a = {x - 70.0f, vp.y};
        Vector2 b = {x + 12.0f, vp.y};
        Vector2 c = {x + 145.0f, vp.y + vp.height};
        Vector2 d = {x + 45.0f, vp.y + vp.height};
        DrawTriangle(a, d, b, (Color){19, 72, 88, 13});
        DrawTriangle(b, d, c, (Color){19, 72, 88, 9});
    }
    for (int i = 0; i < 42; ++i) {
        int x = (int)(vp.x + (float)((i * 83 + 31) % max_int(1, (int)vp.width)));
        int y = (int)(vp.y + (float)((i * 41 + 17) % max_int(1, (int)vp.height)));
        DrawPixel(x, y, (Color){70, 131, 151, 90});
    }
}

static void draw_prism(float cx, float ground_y, float tw, float th, float height,
                       Color base, bool windows, int detail)
{
    Vector2 left_g = {cx - tw * 0.5f, ground_y};
    Vector2 bottom_g = {cx, ground_y + th * 0.5f};
    Vector2 right_g = {cx + tw * 0.5f, ground_y};
    Vector2 left_r = {left_g.x, left_g.y - height};
    Vector2 bottom_r = {bottom_g.x, bottom_g.y - height};
    Vector2 right_r = {right_g.x, right_g.y - height};
    Vector2 top_r = {cx, ground_y - th * 0.5f - height};
    Color left_color = shade(base, -26);
    Color right_color = shade(base, -43);
    Color roof_color = shade(base, 18);
    Color edge = shade(base, -55);

    DrawTriangle(left_g, bottom_g, bottom_r, left_color);
    DrawTriangle(left_g, bottom_r, left_r, left_color);
    DrawTriangle(bottom_g, right_g, right_r, right_color);
    DrawTriangle(bottom_g, right_r, bottom_r, right_color);
    DrawTriangle(top_r, left_r, bottom_r, roof_color);
    DrawTriangle(top_r, bottom_r, right_r, shade(roof_color, -5));

    if (windows && height > 15.0f) {
        /* Windows are skewed parallelograms painted on each sloped isometric wall,
           never flat front-facing rectangles. */
        Color glass = (Color){80, 183, 201, 255};
        Color lit = (Color){238, 201, 122, 255};
        int rows = clamp_int((int)(height / 11.0f), 1, 6);
        int cols = detail >= 3 ? 3 : 2;
        for (int face = 0; face < 2; ++face) {
            Vector2 ground_a = face == 0 ? left_g : bottom_g;
            Vector2 ground_b = face == 0 ? bottom_g : right_g;
            Vector2 roof_a = face == 0 ? left_r : bottom_r;
            Vector2 roof_b = face == 0 ? bottom_r : right_r;
            for (int row = 0; row < rows; ++row) {
                float vlo = 0.10f + (float)row * 0.78f / (float)rows;
                float vhi = vlo + 0.52f / (float)rows;
                for (int col = 0; col < cols; ++col) {
                    float ulo = 0.12f + (float)col * (0.68f / (float)cols);
                    float uhi = ulo + 0.22f / (float)cols;
                    Vector2 p00 = {ground_a.x + (ground_b.x-ground_a.x)*ulo, ground_a.y + (ground_b.y-ground_a.y)*ulo};
                    Vector2 p01 = {ground_a.x + (ground_b.x-ground_a.x)*uhi, ground_a.y + (ground_b.y-ground_a.y)*uhi};
                    Vector2 p10 = {roof_a.x + (roof_b.x-roof_a.x)*ulo, roof_a.y + (roof_b.y-roof_a.y)*ulo};
                    Vector2 p11 = {roof_a.x + (roof_b.x-roof_a.x)*uhi, roof_a.y + (roof_b.y-roof_a.y)*uhi};
                    Vector2 a = {p00.x + (p10.x-p00.x)*vhi, p00.y + (p10.y-p00.y)*vhi};
                    Vector2 b = {p01.x + (p11.x-p01.x)*vhi, p01.y + (p11.y-p01.y)*vhi};
                    Vector2 c = {p01.x + (p11.x-p01.x)*vlo, p01.y + (p11.y-p01.y)*vlo};
                    Vector2 d = {p00.x + (p10.x-p00.x)*vlo, p00.y + (p10.y-p00.y)*vlo};
                    Color gl = ((row + col + detail + face) % 5 == 0) ? lit : (face == 0 ? glass : shade(glass, -18));
                    DrawTriangle(a, b, c, gl);
                    DrawTriangle(a, c, d, gl);
                    DrawLineV(a, b, (Color){32, 68, 78, 255});
                    DrawLineV(d, c, (Color){32, 68, 78, 255});
                }
            }
        }
    }

    DrawLineV(left_r, top_r, edge);
    DrawLineV(top_r, right_r, shade(edge, 15));
    DrawLineV(left_g, left_r, edge);
    DrawLineV(right_g, right_r, edge);
    DrawLineV(bottom_g, bottom_r, edge);
}

static Vector2 raised(Vector2 p, float height)
{
    p.y -= height;
    return p;
}

static Vector2 interpolate_point(Vector2 a, Vector2 b, float t)
{
    return (Vector2){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

static void draw_grid_wall_windows(Vector2 ga, Vector2 gb, Vector2 ra, Vector2 rb,
                                   float height, int detail, int face)
{
    if (height <= 15.0f) return;
    Color glass = (Color){80, 183, 201, 255};
    Color lit = (Color){238, 201, 122, 255};
    int rows = clamp_int((int)(height / 11.0f), 1, 6);
    int cols = detail >= 3 ? 4 : 3;
    for (int row = 0; row < rows; ++row) {
        float vlo = 0.10f + (float)row * 0.78f / (float)rows;
        float vhi = vlo + 0.52f / (float)rows;
        for (int col = 0; col < cols; ++col) {
            float ulo = 0.10f + (float)col * 0.76f / (float)cols;
            float uhi = ulo + 0.18f / (float)cols;
            Vector2 g0 = interpolate_point(ga, gb, ulo);
            Vector2 g1 = interpolate_point(ga, gb, uhi);
            Vector2 r0 = interpolate_point(ra, rb, ulo);
            Vector2 r1 = interpolate_point(ra, rb, uhi);
            Vector2 a = interpolate_point(g0, r0, vhi);
            Vector2 b = interpolate_point(g1, r1, vhi);
            Vector2 c = interpolate_point(g1, r1, vlo);
            Vector2 d = interpolate_point(g0, r0, vlo);
            Color pane = ((row + col + detail + face) % 6 == 0) ? lit : (face == 0 ? glass : shade(glass, -18));
            DrawTriangle(a, b, c, pane);
            DrawTriangle(a, c, d, pane);
            DrawLineV(a, b, (Color){32, 68, 78, 255});
            DrawLineV(d, c, (Color){32, 68, 78, 255});
        }
    }
}

/* raylib's DrawTriangle()/DrawTriangleFan() only rasterize triangles whose
   vertices are in counter-clockwise order on screen; clockwise ones are removed
   by back-face culling. Footprint corners a,b,c,d are projected clockwise, so
   the roof and the rear-left wall used to be culled. These helpers detect the
   winding and flip it when needed, whatever order the caller passes. */
static void fill_tri_any_winding(Vector2 a, Vector2 b, Vector2 c, Color color)
{
    float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (cross > 0.0f) DrawTriangle(a, c, b, color); /* clockwise on screen: flip */
    else DrawTriangle(a, b, c, color);
}

static void fill_grid_quad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color color)
{
    fill_tri_any_winding(a, b, c, color);
    fill_tri_any_winding(a, c, d, color);
}

static void draw_grid_prism(const GameUI *ui, Rectangle vp, int gx, int gy, int fw, int fh,
                            float height, Color base, bool windows, int detail)
{
    Vector2 a, b, c, d;
    footprint_corners(ui, vp, gx, gy, fw, fh, &a, &b, &c, &d);
    Vector2 ar = raised(a, height), br = raised(b, height);
    Vector2 cr = raised(c, height), dr = raised(d, height);
    Color left_color = shade(base, -26);
    Color right_color = shade(base, -43);
    Color roof_color = shade(base, 18);
    Color edge = shade(base, -55);

    /* Paint the complete prism as four explicit wall polygons and a complete
       roof polygon. Large footprints used to be triangulated in separate passes;
       on some rasterizers the left side could disappear visually where the roof
       diagonal and the wall triangles met. */
    const Color rear_left_color = shade(base, -33);
    const Color rear_right_color = shade(base, -37);

    /* Rear faces first, behind the roof. */
    fill_grid_quad(a, d, dr, ar, rear_left_color);
    fill_grid_quad(a, b, br, ar, rear_right_color);

    /* Fill the entire roof once, then add its subtle two-tone split. */
    fill_grid_quad(ar, br, cr, dr, roof_color);
    fill_tri_any_winding(ar, cr, dr, shade(roof_color, -5));

    /* Repaint BOTH camera-facing walls after the roof. This deliberately avoids
       relying on a single face's split-triangle order, which left the left/back
       surface looking transparent on 2x2 and rotated 2x1 footprints. */
    fill_grid_quad(d, c, cr, dr, left_color);
    fill_grid_quad(c, b, br, cr, right_color);

    if (windows) draw_grid_wall_windows(c, b, cr, br, height, detail, 1);
    if (windows) draw_grid_wall_windows(d, c, dr, cr, height, detail, 0);
    DrawLineV(d, c, shade(edge, 12));
    DrawLineV(c, b, shade(edge, 8));
    DrawLineV(d, dr, edge);
    DrawLineV(c, cr, edge);
    DrawLineV(b, br, shade(edge, 15));
    DrawLineV(dr, ar, edge);
    DrawLineV(ar, br, shade(edge, 15));
}

static void draw_construction_overlay(const Building *b, Vector2 p, float tw, float th,
                                      float structure_height, float zoom, float progress)
{
    if (!b || b->construction_days_remaining <= 0 || b->construction_days_total <= 0) return;
    Color scaffold = (Color){207, 151, 77, 230};
    float pole_h = max_float(th * 0.82f, structure_height + 8.0f * zoom);
    float lx = p.x - tw * 0.42f, rx = p.x + tw * 0.42f;
    DrawLineEx((Vector2){lx, p.y + th * 0.08f}, (Vector2){lx, p.y - pole_h}, 1.8f * zoom, scaffold);
    DrawLineEx((Vector2){rx, p.y + th * 0.08f}, (Vector2){rx, p.y - pole_h}, 1.8f * zoom, scaffold);
    for (float yy = p.y - 5.0f * zoom; yy > p.y - pole_h; yy -= 10.0f * zoom) {
        DrawLineEx((Vector2){lx, yy}, (Vector2){rx, yy - 2.0f * zoom}, 1.0f * zoom, scaffold);
        DrawLineEx((Vector2){lx, yy}, (Vector2){rx, yy - 10.0f * zoom}, 0.8f * zoom, color_alpha(scaffold, 160));
    }
    float bar_w = tw * 0.72f;
    DrawRectangle((int)(p.x - bar_w * 0.5f), (int)(p.y - pole_h - 7.0f * zoom), (int)bar_w, max_int(3, (int)(3.0f*zoom)), (Color){24, 42, 49, 255});
    DrawRectangle((int)(p.x - bar_w * 0.5f), (int)(p.y - pole_h - 7.0f * zoom), (int)(bar_w * clamp_float(progress, 0.0f, 1.0f)), max_int(3, (int)(3.0f*zoom)), C_TEAL);
    float crane_x = p.x + sinf((float)GetTime() * 1.2f) * tw * 0.25f;
    DrawLineEx((Vector2){p.x, p.y - pole_h - 3.0f*zoom}, (Vector2){crane_x, p.y - pole_h + 4.0f*zoom}, 1.0f*zoom, C_GOLD);
    DrawText(TextFormat("CHANTIER %d j", b->construction_days_remaining), (int)(p.x - 35.0f*zoom), (int)(p.y + th*0.42f), max_int(7, (int)(8.0f*zoom)), C_GOLD);
}

/* Decorations that follow the isometric walls. g0->g1 is the ground edge of a
   wall; u runs along that edge (0..1) and v is a fraction of the wall height.
   Flat screen-horizontal bars used to cut across the buildings; these bands are
   sheared like the wall they sit on, so they read as trim, awnings or signs. */
static Vector2 wall_point(Vector2 g0, Vector2 g1, float u, float lift)
{
    Vector2 p = interpolate_point(g0, g1, u);
    p.y -= lift;
    return p;
}

static void wall_band(Vector2 g0, Vector2 g1, float height, float u0, float u1,
                      float v0, float v1, Color color)
{
    float lo = height * v0, hi = height * v1;
    if (hi - lo < 2.0f) hi = lo + 2.0f;
    fill_grid_quad(wall_point(g0, g1, u0, lo), wall_point(g0, g1, u1, lo),
                   wall_point(g0, g1, u1, hi), wall_point(g0, g1, u0, hi), color);
}

/* Same band on the front-left wall (fd->fc) and front-right wall (fc->fb), so it
   wraps around the corner like a belt. The right face is slightly darker. */
static void wall_band_both(Vector2 fd, Vector2 fc, Vector2 fb, float height, float u0, float u1,
                           float v0, float v1, Color color)
{
    wall_band(fd, fc, height, u0, u1, v0, v1, color);
    wall_band(fc, fb, height, u0, u1, v0, v1, shade(color, -14));
}

/* Glass dome shell over a roof: upper cap, three ribbed meridians and the footing rim.
   The caller draws any interior content (crops, trees...) and the soil ellipse first, since
   the dome's front glass panels are meant to sit over them. */
static void draw_glass_dome_shell(Vector2 p, float roof_y, float rx, float ry, float dome_h, float detail_zoom,
                                  Color glass, Color rim, Color rib)
{
    float cap_h = sqrtf(ry * ry + dome_h * dome_h);
    const int seg = 24;
    Vector2 center = {p.x, roof_y};
    Vector2 prev_up = {p.x + rx, roof_y};
    Vector2 prev_low = prev_up;
    for (int i = 1; i <= seg; ++i) {
        float ang = 3.14159265f * (float)i / (float)seg;
        Vector2 up = {p.x + rx * cosf(ang), roof_y - cap_h * sinf(ang)};
        Vector2 low = {p.x + rx * cosf(ang), roof_y + ry * sinf(ang)};
        fill_tri_any_winding(center, prev_up, up, glass);
        fill_tri_any_winding(center, prev_low, low, glass);
        DrawLineEx(prev_up, up, max_float(1.0f, detail_zoom), rim);
        prev_up = up;
        prev_low = low;
    }
    for (int m = 0; m < 3; ++m) {
        float phi = 3.14159265f * (float)m / 3.0f;
        float cphi = cosf(phi), sphi = sinf(phi);
        Vector2 last = {p.x + rx * cphi, roof_y + ry * sphi};
        for (int i = 1; i <= 16; ++i) {
            float alpha = 3.14159265f * (float)i / 16.0f;
            Vector2 pt = {p.x + rx * cosf(alpha) * cphi, roof_y + ry * cosf(alpha) * sphi - dome_h * sinf(alpha)};
            DrawLineEx(last, pt, max_float(1.0f, detail_zoom), rib);
            last = pt;
        }
    }
    DrawEllipseLines((int)p.x, (int)roof_y, rx, ry, rim);
}

static void draw_building(const GameState *g, const GameUI *ui, Rectangle vp, int bi)
{
    const Building *b = &g->buildings[bi];
    if (!b->active) return;
    int fw = b->footprint_w > 0 ? b->footprint_w : 1;
    int fh = b->footprint_h > 0 ? b->footprint_h : 1;
    float footprint_scale = ((float)fw + (float)fh) * 0.5f;
    Vector2 p = tile_to_screen(ui, vp, (float)b->x + ((float)fw - 1.0f) * 0.5f, (float)b->y + ((float)fh - 1.0f) * 0.5f);
    float tw = TILE_W * ui->zoom * footprint_scale;
    float th = TILE_H * ui->zoom * footprint_scale;
    bool under_construction = b->construction_days_remaining > 0 && b->construction_days_total > 0;
    float progress = under_construction ? 1.0f - (float)b->construction_days_remaining / (float)b->construction_days_total : 1.0f;
    float detail_zoom = ui->zoom * (under_construction ? 0.38f + 0.62f * clamp_float(progress, 0.0f, 1.0f) : 1.0f);
    draw_footprint_shape(ui, vp, b->x, b->y, fw, fh,
                         (Color){3, 13, 23, 80}, (Color){3, 13, 23, 0}, 0.0f);

    if (b->type == BT_CORRIDOR) {
        /* Walkway geometry recalculates automatically from all four neighbours:
           straight segments, elbows, three-way junctions and four-way junctions. */
        draw_diamond(p, tw, th, (Color){36, 78, 84, 255}, (Color){29, 62, 68, 255}, (Color){19, 45, 57, 255}, g->show_grid);
        const int nx[4] = {b->x - 1, b->x + 1, b->x, b->x};
        const int ny[4] = {b->y, b->y, b->y - 1, b->y + 1};
        /* Signed direction of each neighbour on screen, used to place the connector's end:
           tile edge (0.5) for a building wall, a touch past it (0.61) for another corridor
           so the two tiles' lines overlap into one seamless path with no visible seam. */
        const float dirx[4] = {-1.0f, 1.0f, 1.0f, -1.0f};
        const float diry[4] = {-1.0f, 1.0f, -1.0f, 1.0f};
        bool any_route = false;
        for (int k = 0; k < 4; ++k) {
            int ni = Sim_GetBuildingAt(g, nx[k], ny[k]);
            if (ni < 0) continue;
            any_route = true;
            bool same_network = g->buildings[ni].type == BT_CORRIDOR;
            float frac = same_network ? 0.61f : 0.50f;
            Vector2 end = {p.x + dirx[k] * tw * frac, p.y + diry[k] * th * frac};
            DrawLineEx((Vector2){p.x, p.y}, end, 3.0f * ui->zoom, (Color){76, 105, 108, 255});
            DrawLineEx((Vector2){p.x, p.y}, end, 1.25f * ui->zoom, (Color){201, 151, 83, 255});
            if (same_network) DrawCircleV((Vector2){p.x + (end.x-p.x)*0.78f, p.y + (end.y-p.y)*0.78f}, (float)max_int(1, (int)(1.8f*ui->zoom)), (Color){226, 188, 120, 255});
        }
        if (!any_route) {
            DrawLineEx((Vector2){p.x - tw * 0.34f, p.y - th * 0.34f}, (Vector2){p.x + tw * 0.34f, p.y + th * 0.34f}, 2.2f * ui->zoom, (Color){198, 148, 77, 255});
        }
        DrawCircleV((Vector2){p.x, p.y}, (float)max_int(2, (int)(2.7f*ui->zoom)), (Color){230, 195, 128, 255});
        if (((b->x + b->y) % 3) == 0) DrawLine((int)(p.x - tw * 0.10f), (int)(p.y - th * 0.10f), (int)(p.x + tw * 0.12f), (int)(p.y + th * 0.10f), (Color){82, 129, 136, 255});
        if (under_construction) draw_construction_overlay(b, p, tw, th, th * 0.25f, ui->zoom, progress);
        return;
    }
    if (b->type == BT_PIPE) {
        /* A pipe tile derives its shape from the four neighbouring pipe tiles:
           straight, elbow, T-junction, cross, or capped end are all automatic. */
        /* A pipe docks against ANY adjacent building, not just other pipes: same connector
           system as the corridor (tile edge for a wall, a touch past it for another pipe so
           the two tiles' runs overlap seamlessly). */
        const int pnx[4] = {b->x - 1, b->x + 1, b->x, b->x};
        const int pny[4] = {b->y, b->y, b->y - 1, b->y + 1};
        const float pdirx[4] = {-1.0f, 1.0f, 1.0f, -1.0f};
        const float pdiry[4] = {-1.0f, 1.0f, -1.0f, 1.0f};
        draw_diamond(p, tw, th, (Color){28, 48, 55, 255}, (Color){20, 38, 45, 255}, (Color){15, 28, 39, 255}, g->show_grid);
        const Vector2 center = {p.x, p.y};
        bool any_route = false;
        for (int k = 0; k < 4; ++k) {
            int ni = Sim_GetBuildingAt(g, pnx[k], pny[k]);
            if (ni < 0) continue;
            any_route = true;
            bool same_network = g->buildings[ni].type == BT_PIPE;
            float frac = same_network ? 0.61f : 0.50f;
            Vector2 end = {p.x + pdirx[k] * tw * frac, p.y + pdiry[k] * th * frac};
            DrawLineEx(center, end, (float)max_int(4, (int)(5.5f * ui->zoom)), (Color){72, 43, 28, 255});
            DrawLineEx(center, end, (float)max_int(2, (int)(2.2f * ui->zoom)), (Color){211, 146, 78, 255});
            if (!same_network) DrawCircleV((Vector2){center.x + (end.x-center.x)*0.9f, center.y + (end.y-center.y)*0.9f}, (float)max_int(1, (int)(1.6f * ui->zoom)), (Color){224, 188, 131, 255});
        }
        /* Fully isolated tiles are capped, but remain clearly pipe-like. */
        if (!any_route) {
            DrawLineEx((Vector2){p.x - tw * 0.61f * 0.44f, p.y - th * 0.61f * 0.44f}, (Vector2){p.x + tw * 0.61f * 0.44f, p.y + th * 0.61f * 0.44f}, 2.2f * ui->zoom, (Color){211, 146, 78, 255});
        }
        DrawCircleV(center, (float)max_int(3, (int)(4.2f * ui->zoom)), (Color){200, 139, 73, 255});
        DrawCircleV(center, (float)max_int(1, (int)(1.8f * ui->zoom)), (Color){243, 205, 137, 255});
        if (under_construction) draw_construction_overlay(b, p, tw, th, th * 0.25f, ui->zoom, progress);
        return;
    }

    float height = 25.0f;
    switch (b->type) {
        case BT_RESIDENCE: height = 45.0f; break;
        case BT_MARKET: height = 16.0f; break;
        case BT_FACTORY: height = 19.0f; break;
        case BT_LABORATORY: height = 34.0f; break;
        case BT_POWER: height = 29.0f; break;
        case BT_WATER: height = 18.0f; break;
        case BT_OXYGEN: height = 23.0f; break;
        case BT_FOOD: height = 12.0f; break;
        case BT_HOSPITAL: height = 27.0f; break;
        case BT_LEISURE: height = 22.0f; break;
        case BT_BATHYSPHERE: height = 20.0f; break;
        case BT_METRO: height = 14.0f; break;
        case BT_AIRLOCK: height = 19.0f; break;
        case BT_RECYCLING: height = 21.0f; break;
        case BT_GENETICS: height = 38.0f; break;
        case BT_SECURITY: height = 18.0f; break;
        case BT_CINEMA: height = 20.0f; break;
        case BT_RESTAURANT: height = 14.0f; break;
        case BT_BOUTIQUE: height = 12.0f; break;
        case BT_FISHERY: height = 24.0f; break;
        case BT_ARCADIA: height = 36.0f; break;
        case BT_HEPHAESTUS: height = 38.0f; break;
        case BT_FORTRESS_FOLATRE: height = 34.0f; break;
        case BT_GREENHOUSE_DOME: height = 23.0f; break;
        case BT_RICH_RESIDENCE: height = 40.0f; break;
        default: break;
    }
    height *= ui->zoom;
    if (under_construction) height *= 0.18f + 0.82f * clamp_float(progress, 0.0f, 1.0f);
    Color base = building_color(b->type);
    if (b->closed) base = shade(base, -35);
    Vector2 fa, fb, fc, fd; /* footprint corners: top, right, bottom, left */
    footprint_corners(ui, vp, b->x, b->y, fw, fh, &fa, &fb, &fc, &fd);
    bool windows = b->type == BT_RESIDENCE || b->type == BT_LABORATORY || b->type == BT_HOSPITAL || b->type == BT_GENETICS || b->type == BT_CINEMA || b->type == BT_RESTAURANT || b->type == BT_FORTRESS_FOLATRE;
    if (fw > 1 || fh > 1)
        draw_grid_prism(ui, vp, b->x, b->y, fw, fh, height, base, windows, BUILDING_DEFS[b->type].visual);
    else
        draw_prism(p.x, p.y, tw, th, height, base, windows, BUILDING_DEFS[b->type].visual);

    /* Stepped Art Deco crowns and illuminated finials. */
    if (b->type == BT_RESIDENCE || b->type == BT_LABORATORY || b->type == BT_HOSPITAL || b->type == BT_GENETICS || b->type == BT_POWER) {
        float tier_height = height * 0.22f;
        float tier_base = p.y - height;
        draw_prism(p.x, tier_base, tw * 0.62f, th * 0.62f, tier_height, shade(base, 16), false, 1);
        if (b->type == BT_LABORATORY || b->type == BT_GENETICS || b->type == BT_POWER) {
            draw_prism(p.x, tier_base - tier_height, tw * 0.31f, th * 0.31f, tier_height * 0.55f, shade(base, 24), false, 1);
            DrawRectangle((int)(p.x - 2.0f * detail_zoom), (int)(tier_base - tier_height * 1.55f - 3.0f), max_int(2, (int)(4.0f * detail_zoom)), max_int(3, (int)(6.0f * detail_zoom)), C_GOLD);
        }
    }
    /* Individually recognisable silhouettes: brass trim, glass, neon and machinery. */
    if (b->type == BT_RESIDENCE) {
        wall_band_both(fd, fc, fb, height, 0.0f, 1.0f, 0.90f, 0.955f, (Color){210, 177, 117, 255});
        DrawRectangle((int)(p.x - tw * 0.10f), (int)(p.y - height - 4.0f * detail_zoom), max_int(4, (int)(tw * 0.20f)), max_int(3, (int)(4.0f * detail_zoom)), (Color){211, 178, 117, 255});
        DrawCircle((int)p.x, (int)(p.y - height * 0.72f), (float)max_int(2, (int)(3.0f * detail_zoom)), (Color){235, 199, 126, 255});
    } else if (b->type == BT_MARKET) {
        const Color awning = (Color){228, 192, 133, 255};
        const Color stripe = (Color){132, 54, 48, 255};
        wall_band(fd, fc, height, 0.04f, 0.96f, 0.40f, 0.56f, awning);
        wall_band(fc, fb, height, 0.04f, 0.96f, 0.40f, 0.56f, shade(awning, -14));
        for (int st = 0; st < 4; ++st) {
            float u0 = 0.12f + 0.22f * (float)st;
            wall_band(fd, fc, height, u0, u0 + 0.09f, 0.40f, 0.56f, stripe);
            wall_band(fc, fb, height, u0, u0 + 0.09f, 0.40f, 0.56f, shade(stripe, -12));
        }
        wall_band(fc, fb, height, 0.20f, 0.62f, 0.68f, 0.84f, C_GOLD);
    } else if (b->type == BT_FACTORY) {
        /* Wide foundry shed with riveted roof, tall chimneys and animated exhaust. */
        wall_band_both(fd, fc, fb, height, 0.0f, 1.0f, 0.32f, 0.40f, (Color){211, 160, 89, 255});
        wall_band_both(fd, fc, fb, height, 0.0f, 1.0f, 0.62f, 0.66f, (Color){181, 87, 49, 255});
        DrawRectangle((int)(p.x - tw * 0.29f), (int)(p.y - height - 12.0f * detail_zoom), max_int(5, (int)(tw * 0.13f)), max_int(9, (int)(13.0f * detail_zoom)), (Color){48, 48, 47, 255});
        DrawRectangle((int)(p.x + tw * 0.12f), (int)(p.y - height - 19.0f * detail_zoom), max_int(6, (int)(tw * 0.14f)), max_int(13, (int)(20.0f * detail_zoom)), (Color){59, 56, 52, 255});
        DrawRectangle((int)(p.x - tw * 0.31f), (int)(p.y - height - 13.0f * detail_zoom), max_int(6, (int)(tw * 0.17f)), max_int(3, (int)(4.0f * detail_zoom)), (Color){181, 87, 49, 255});
        DrawRectangle((int)(p.x + tw * 0.10f), (int)(p.y - height - 20.0f * detail_zoom), max_int(7, (int)(tw * 0.18f)), max_int(3, (int)(4.0f * detail_zoom)), (Color){181, 87, 49, 255});
        for (int k = 0; k < 4; ++k) {
            float t = (float)fmod(GetTime() * 0.65 + (double)k * 0.24, 1.0);
            float sx = p.x + tw * 0.17f + sinf((float)GetTime() + (float)k) * tw * 0.035f;
            float sy = p.y - height - 22.0f * detail_zoom - t * 13.0f * detail_zoom;
            DrawCircle((int)sx, (int)sy, (float)(2.0 + t * 2.4) * detail_zoom, (Color){145, 163, 165, (unsigned char)(125.0f * (1.0f-t))});
        }
    } else if (b->type == BT_LABORATORY) {
        {
            /* Cross painted flat on the longest front wall: both bars follow the wall's
               isometric slant, so it keeps the correct perspective at any footprint. */
            Vector2 w0 = fd, w1 = fc;
            if (hypotf(fb.x - fc.x, fb.y - fc.y) > hypotf(fc.x - fd.x, fc.y - fd.y)) { w0 = fc; w1 = fb; }
            float wall_len = hypotf(w1.x - w0.x, w1.y - w0.y);
            float tile_edge = hypotf(TILE_W * ui->zoom * 0.5f, TILE_H * ui->zoom * 0.5f);
            float arm = min_float(wall_len * 0.24f, tile_edge * 0.30f); /* half length, in pixels */
            float hu = arm / wall_len;
            float vv = min_float(0.40f, arm * 0.95f / height);
            float tu = hu * 0.36f, tv = vv * 0.36f;
            const float uc = 0.5f, vc = 0.56f;
            const Color cross = (Color){73, 156, 176, 255};
            const Color core = (Color){153, 223, 222, 255};
            wall_band(w0, w1, height, uc - hu, uc + hu, vc - tv, vc + tv, cross);
            wall_band(w0, w1, height, uc - tu, uc + tu, vc - vv, vc + vv, cross);
            wall_band(w0, w1, height, uc - hu * 0.55f, uc + hu * 0.55f, vc - tv * 0.45f, vc + tv * 0.45f, core);
            wall_band(w0, w1, height, uc - tu * 0.45f, uc + tu * 0.45f, vc - vv * 0.55f, vc + vv * 0.55f, core);
        }
        wall_band_both(fd, fc, fb, height, 0.0f, 1.0f, 0.90f, 0.96f, C_GOLD);
        DrawRectangle((int)(p.x - 2.0f * detail_zoom), (int)(p.y - height - 10.0f * detail_zoom), max_int(2, (int)(4.0f * detail_zoom)), max_int(4, (int)(7.0f * detail_zoom)), (Color){136, 207, 214, 255});
    } else if (b->type == BT_POWER) {
        DrawRectangle((int)(p.x - tw * 0.26f), (int)(p.y - height - 8.0f * detail_zoom), max_int(3, (int)(tw * 0.13f)), max_int(5, (int)(10.0f * detail_zoom)), shade(base, 24));
        DrawRectangle((int)(p.x + tw * 0.08f), (int)(p.y - height - 12.0f * detail_zoom), max_int(3, (int)(tw * 0.12f)), max_int(6, (int)(14.0f * detail_zoom)), shade(base, 12));
        wall_band_both(fd, fc, fb, height, 0.0f, 1.0f, 0.86f, 0.94f, C_GOLD);
    } else if (b->type == BT_WATER || b->type == BT_OXYGEN) {
        Color pipe_tint = b->type == BT_WATER ? (Color){111, 198, 216, 255} : (Color){100, 211, 194, 255};
        wall_band_both(fd, fc, fb, height, 0.0f, 1.0f, 0.88f, 0.95f, C_GOLD);
        wall_band(fc, fb, height, 0.10f, 0.90f, 0.20f, 0.78f, shade(base, -16));
        for (int k = 0; k < 3; ++k) {
            float u0 = 0.18f + (float)k * 0.26f;
            wall_band(fc, fb, height, u0, u0 + 0.05f, 0.22f, 0.76f, pipe_tint);
        }
        if (b->type == BT_WATER) {
            DrawEllipse((int)(p.x - tw * 0.13f), (int)(p.y - height - 3.0f * detail_zoom), (float)max_int(4, (int)(tw * 0.13f)), (float)max_int(3, (int)(4.0f * detail_zoom)), (Color){111, 198, 216, 255});
            DrawEllipse((int)(p.x + tw * 0.15f), (int)(p.y - height - 3.0f * detail_zoom), (float)max_int(4, (int)(tw * 0.11f)), (float)max_int(3, (int)(4.0f * detail_zoom)), (Color){75, 152, 174, 255});
        } else {
            DrawRectangle((int)(p.x - tw * 0.23f), (int)(p.y - height - 6.0f * detail_zoom), max_int(4, (int)(tw * 0.16f)), max_int(7, (int)(11.0f * detail_zoom)), (Color){87, 177, 162, 255});
            DrawRectangle((int)(p.x + tw * 0.08f), (int)(p.y - height - 9.0f * detail_zoom), max_int(4, (int)(tw * 0.16f)), max_int(9, (int)(14.0f * detail_zoom)), (Color){100, 211, 194, 255});
            DrawCircle((int)(p.x + tw * 0.25f), (int)(p.y - height * 0.60f), (float)max_int(2, (int)(3.0f * detail_zoom)), (Color){170, 237, 217, 255});
        }
    } else if (b->type == BT_BATHYSPHERE) {
        /* A riveted docking collar wraps the hull, with the glazed porthole at its centre. */
        wall_band_both(fd, fc, fb, height, 0.0f, 1.0f, 0.06f, 0.16f, C_GOLD);
        wall_band(fc, fb, height, 0.22f, 0.78f, 0.30f, 0.80f, shade(base, -22));
        DrawCircle((int)p.x, (int)(p.y - height * 0.56f), (float)max_int(5, (int)(7.0f * detail_zoom)), shade(base, -30));
        DrawCircle((int)p.x, (int)(p.y - height * 0.56f), (float)max_int(4, (int)(6.0f * detail_zoom)), (Color){62, 158, 186, 255});
        DrawCircle((int)p.x, (int)(p.y - height * 0.56f), (float)max_int(2, (int)(3.0f * detail_zoom)), C_GOLD);
        for (int k = 0; k < 8; ++k) {
            float ang = 6.2831853f * (float)k / 8.0f;
            DrawCircle((int)(p.x + cosf(ang) * 8.0f * detail_zoom), (int)(p.y - height * 0.56f + sinf(ang) * 8.0f * detail_zoom), max_float(1.0f, detail_zoom), (Color){226, 188, 120, 220});
        }
    } else if (b->type == BT_METRO) {
        /* An elevated rail runs the length of the platform, with a lit sign above the doors. */
        wall_band_both(fd, fc, fb, height, 0.0f, 1.0f, 0.14f, 0.22f, C_TEAL);
        wall_band(fd, fc, height, 0.08f, 0.92f, 0.56f, 0.70f, shade(base, -18));
        wall_band(fd, fc, height, 0.08f, 0.92f, 0.60f, 0.64f, (Color){226, 188, 120, 220});
        DrawRectangle((int)(p.x - tw * 0.08f), (int)(p.y - height * 0.79f), max_int(4, (int)(tw * 0.22f)), max_int(3, (int)(6.0f * detail_zoom)), C_GOLD);
    } else if (b->type == BT_AIRLOCK) {
        /* A pressure hatch, bolted ring and gauge needle set into the front wall. */
        wall_band_both(fd, fc, fb, height, 0.0f, 1.0f, 0.04f, 0.10f, shade(C_GOLD, -24));
        DrawCircle((int)p.x, (int)(p.y - height * 0.35f), (float)max_int(5, (int)(tw * 0.18f)), shade(base, -26));
        DrawCircle((int)p.x, (int)(p.y - height * 0.35f), (float)max_int(4, (int)(tw * 0.15f)), (Color){59, 92, 108, 255});
        DrawCircle((int)p.x, (int)(p.y - height * 0.35f), (float)max_int(2, (int)(tw * 0.10f)), C_GOLD);
        for (int k = 0; k < 6; ++k) {
            float ang = 6.2831853f * (float)k / 6.0f;
            DrawCircle((int)(p.x + cosf(ang) * tw * 0.17f), (int)(p.y - height * 0.35f + sinf(ang) * tw * 0.17f), max_float(1.0f, detail_zoom), (Color){226, 188, 120, 220});
        }
    } else if (b->type == BT_GENETICS) {
        for (int k = 0; k < 4; ++k) {
            float yy = p.y - height * 0.72f + (float)k * 4.0f * detail_zoom;
            float xx = (k % 2 == 0) ? -tw * 0.13f : tw * 0.13f;
            DrawLine((int)(p.x + xx), (int)yy, (int)(p.x - xx), (int)(yy + 3.0f * detail_zoom), (Color){206, 118, 196, 255});
        }
        DrawLine((int)(p.x - tw * 0.13f), (int)(p.y - height * 0.73f), (int)(p.x - tw * 0.13f), (int)(p.y - height * 0.44f), (Color){114, 183, 214, 255});
        DrawLine((int)(p.x + tw * 0.13f), (int)(p.y - height * 0.73f), (int)(p.x + tw * 0.13f), (int)(p.y - height * 0.44f), (Color){206, 118, 196, 255});
    } else if (b->type == BT_FOOD) {
        /* Glass biodome resting on the roof. Its footing is an ellipse in the
           isometric ground ratio, centred on the roof, and the cap rises from it. */
        float roof_y = p.y - height;
        float rx = tw * 0.27f;
        float ry = rx * (th / tw);
        float dome_h = rx * 0.85f;
        const Color glass = (Color){48, 106, 111, 150};
        const Color rim = (Color){155, 219, 205, 255};
        const Color rib = (Color){123, 192, 179, 255};
        /* Soil and crops inside the dome. */
        DrawEllipse((int)p.x, (int)roof_y, rx, ry, (Color){29, 66, 62, 255});
        for (int k = -2; k <= 2; ++k) {
            float fx = (float)k * rx * 0.34f;
            float ratio = fx / rx;
            float room = sqrtf(max_float(0.0f, 1.0f - ratio * ratio));
            float base_y = roof_y + ((k % 2 == 0) ? -1.0f : 1.0f) * ry * 0.25f * room;
            DrawLineEx((Vector2){p.x + fx, base_y}, (Vector2){p.x + fx, base_y - dome_h * 0.42f * room - 2.0f * detail_zoom},
                       1.4f * detail_zoom, (Color){145, 207, 107, 255});
        }
        /* Glass: upper cap plus the front half of the footing. */
        draw_glass_dome_shell(p, roof_y, rx, ry, dome_h, detail_zoom, glass, rim, rib);
    } else if (b->type == BT_GREENHOUSE_DOME) {
        /* A taller, denser biodome: a cultivated forest instead of a single bed of crops,
           with trunks of varying height and triangular canopies, like the farm's crops but
           grown into trees. The footing/glass geometry comes from the same shell helper. */
        float roof_y = p.y - height;
        float rx = tw * 0.29f;
        float ry = rx * (th / tw);
        float dome_h = rx * 1.02f;
        const Color glass = (Color){38, 84, 64, 150};
        const Color rim = (Color){150, 214, 175, 255};
        const Color rib = (Color){112, 182, 140, 255};
        DrawEllipse((int)p.x, (int)roof_y, rx, ry, (Color){33, 58, 40, 255});
        for (int k = -3; k <= 3; ++k) {
            float fx = (float)k * rx * 0.26f;
            float ratio = fx / rx;
            float room = sqrtf(max_float(0.0f, 1.0f - ratio * ratio));
            float base_y = roof_y + ((k % 2 == 0) ? -1.0f : 1.0f) * ry * 0.30f * room;
            float trunk_h = dome_h * (0.34f + 0.22f * room) * (1.0f + 0.12f * (float)(k % 2));
            Vector2 trunk_top = {p.x + fx, base_y - trunk_h};
            DrawLineEx((Vector2){p.x + fx, base_y}, trunk_top, 1.6f * detail_zoom, (Color){92, 68, 48, 255});
            Color canopy = (k % 3 == 0) ? (Color){74, 150, 92, 255} : (Color){94, 173, 107, 255};
            fill_tri_any_winding((Vector2){trunk_top.x - rx * 0.09f, trunk_top.y + rx * 0.05f},
                                 (Vector2){trunk_top.x + rx * 0.09f, trunk_top.y + rx * 0.05f},
                                 (Vector2){trunk_top.x, trunk_top.y - rx * 0.20f}, canopy);
            fill_tri_any_winding((Vector2){trunk_top.x - rx * 0.065f, trunk_top.y - rx * 0.03f},
                                 (Vector2){trunk_top.x + rx * 0.065f, trunk_top.y - rx * 0.03f},
                                 (Vector2){trunk_top.x, trunk_top.y - rx * 0.24f}, shade(canopy, 12));
        }
        draw_glass_dome_shell(p, roof_y, rx, ry, dome_h, detail_zoom, glass, rim, rib);
    } else if (b->type == BT_RICH_RESIDENCE) {
        /* A prestige residence: a stepped marble-and-gilt facade, round-arched windows and a
           roof balustrade, built from the same wall-slanted bands used elsewhere so it stays
           correct in perspective on any footprint. */
        const Color marble = (Color){224, 212, 188, 255};
        const Color gilt = C_GOLD;
        wall_band_both(fd, fc, fb, height, 0.0f, 1.0f, 0.0f, 0.10f, shade(marble, -22));
        wall_band_both(fd, fc, fb, height, 0.0f, 1.0f, 0.50f, 0.54f, gilt);
        wall_band_both(fd, fc, fb, height, 0.0f, 1.0f, 0.93f, 0.98f, gilt);
        for (int k = 0; k < 2; ++k) {
            float u0 = 0.16f + (float)k * 0.40f;
            wall_band(fd, fc, height, u0, u0 + 0.22f, 0.58f, 0.86f, shade(marble, -45));
            wall_band(fd, fc, height, u0 + 0.03f, u0 + 0.19f, 0.62f, 0.82f, (Color){120, 168, 186, 200});
            Vector2 arch_c = wall_point(fd, fc, u0 + 0.11f, height * 0.82f);
            fill_tri_any_winding(wall_point(fd, fc, u0 + 0.03f, height * 0.82f), wall_point(fd, fc, u0 + 0.19f, height * 0.82f), arch_c, shade(marble, -45));
        }
        float roof_y = p.y - height;
        DrawRectangle((int)(p.x - tw * 0.34f), (int)(roof_y - height * 0.03f), max_int(8, (int)(tw * 0.68f)), max_int(3, (int)(height * 0.06f)), shade(marble, -30));
        for (int k = -3; k <= 3; ++k) {
            float bx = p.x + (float)k * tw * 0.095f;
            DrawRectangle((int)(bx - 1.3f * detail_zoom), (int)(roof_y - height * 0.16f), max_int(2, (int)(2.6f * detail_zoom)), max_int(4, (int)(height * 0.13f)), marble);
        }
        DrawRectangle((int)(p.x - tw * 0.10f), (int)(roof_y - height * 0.30f), max_int(6, (int)(tw * 0.20f)), max_int(4, (int)(height * 0.14f)), (Color){176, 150, 104, 255});
        DrawLineEx((Vector2){p.x - tw * 0.10f, roof_y - height * 0.30f}, (Vector2){p.x + tw * 0.10f, roof_y - height * 0.30f}, max_float(2.0f, 2.0f * detail_zoom), gilt);
    } else if (b->type == BT_HOSPITAL) {
        /* Clean clinical facade with a lit cross sign over the entrance. */
        wall_band_both(fd, fc, fb, height, 0.0f, 1.0f, 0.90f, 0.96f, (Color){230, 228, 209, 255});
        wall_band(fd, fc, height, 0.30f, 0.70f, 0.30f, 0.80f, shade((Color){230, 228, 209, 255}, -10));
        wall_band(fd, fc, height, 0.42f, 0.58f, 0.40f, 0.70f, (Color){120, 188, 206, 220});
        int cx = (int)(p.x + tw * 0.12f), cy = (int)(p.y - height * 0.64f);
        DrawRectangle(cx - max_int(2, (int)(3.0f * detail_zoom)), cy - max_int(4, (int)(6.0f * detail_zoom)), max_int(4, (int)(6.0f * detail_zoom)), max_int(8, (int)(12.0f * detail_zoom)), (Color){230, 228, 209, 255});
        DrawRectangle(cx - max_int(5, (int)(7.0f * detail_zoom)), cy - max_int(2, (int)(3.0f * detail_zoom)), max_int(10, (int)(14.0f * detail_zoom)), max_int(4, (int)(6.0f * detail_zoom)), (Color){176, 56, 50, 255});
    } else if (b->type == BT_LEISURE) {
        /* Velvet-and-brass music-hall front, with a marquee strip above the doors. */
        wall_band_both(fd, fc, fb, height, 0.0f, 1.0f, 0.88f, 0.94f, C_GOLD);
        wall_band(fd, fc, height, 0.10f, 0.90f, 0.58f, 0.80f, (Color){58, 30, 55, 255});
        wall_band(fd, fc, height, 0.16f, 0.84f, 0.64f, 0.74f, (Color){227, 91, 124, 255});
        wall_band(fc, fb, height, 0.42f, 0.58f, 0.34f, 0.46f, C_GOLD);
    } else if (b->type == BT_RECYCLING) {
        const Color arrow = (Color){102, 202, 132, 255};
        wall_band_both(fd, fc, fb, height, 0.0f, 1.0f, 0.88f, 0.94f, arrow);
        wall_band(fd, fc, height, 0.14f, 0.68f, 0.32f, 0.38f, arrow);
        fill_tri_any_winding(wall_point(fd, fc, 0.86f, height * 0.35f), wall_point(fd, fc, 0.66f, height * 0.50f),
                             wall_point(fd, fc, 0.66f, height * 0.20f), arrow);
        wall_band(fd, fc, height, 0.34f, 0.84f, 0.17f, 0.23f, arrow);
    } else if (b->type == BT_SECURITY) {
        /* Reinforced checkpoint wall with a sweeping searchlight beam. */
        wall_band_both(fd, fc, fb, height, 0.0f, 1.0f, 0.04f, 0.10f, shade(base, -26));
        wall_band(fc, fb, height, 0.18f, 0.82f, 0.30f, 0.70f, shade(base, -14));
        DrawRectangle((int)(p.x - tw * 0.08f), (int)(p.y - height - 7.0f * detail_zoom), max_int(3, (int)(tw * 0.16f)), max_int(6, (int)(9.0f * detail_zoom)), (Color){48, 58, 65, 255});
        DrawTriangle((Vector2){p.x - tw * 0.22f, p.y - height * 0.76f}, (Vector2){p.x + tw * 0.22f, p.y - height * 0.76f}, (Vector2){p.x + tw * 0.07f, p.y - height * 0.48f}, (Color){222, 189, 115, 35});
        DrawRectangle((int)(p.x - 2.0f * detail_zoom), (int)(p.y - height * 0.79f), max_int(4, (int)(5.0f * detail_zoom)), max_int(3, (int)(4.0f * detail_zoom)), (Color){240, 211, 139, 255});
    }
    if (b->type == BT_CINEMA) {
        wall_band(fd, fc, height, 0.10f, 0.90f, 0.46f, 0.80f, (Color){36, 53, 67, 255});
        wall_band(fd, fc, height, 0.17f, 0.83f, 0.54f, 0.72f, (Color){214, 76, 78, 255});
    } else if (b->type == BT_RESTAURANT) {
        wall_band_both(fd, fc, fb, height, 0.0f, 1.0f, 0.90f, 0.96f, (Color){221, 177, 108, 255});
        DrawCircle((int)(p.x + tw * 0.18f), (int)(p.y - height * 0.76f), max_float(2.0f, 2.5f * detail_zoom), (Color){232, 193, 137, 255});
    } else if (b->type == BT_BOUTIQUE) {
        wall_band_both(fd, fc, fb, height, 0.0f, 1.0f, 0.36f, 0.48f, (Color){83, 189, 179, 255});
    } else if (b->type == BT_FISHERY) {
        /* Remarkable aquaculture: a turquoise fountain pool and radial water jets. */
        float roof_y = p.y - height;
        DrawEllipse((int)p.x, (int)roof_y, tw * 0.25f, max_float(3.0f, th * 0.16f), (Color){34, 125, 143, 255});
        DrawEllipseLines((int)p.x, (int)roof_y, tw * 0.25f, max_float(3.0f, th * 0.16f), (Color){150, 239, 223, 255});
        DrawEllipse((int)p.x, (int)(roof_y - 2.0f * detail_zoom), tw * 0.13f, max_float(2.0f, th * 0.09f), (Color){91, 221, 214, 255});
        for (int jet = -1; jet <= 1; ++jet) {
            DrawLineEx((Vector2){p.x + (float)jet * tw * 0.11f, roof_y + 2.0f * detail_zoom},
                      (Vector2){p.x + (float)jet * tw * 0.15f, roof_y - height * 0.20f},
                      max_float(1.0f, 1.5f * detail_zoom), (Color){180, 244, 239, 255});
        }
    } else if (b->type == BT_ARCADIA) {
        /* Green glass biodome with brass ribs: intentionally unlike standard farms. */
        float roof_y = p.y - height;
        float rx = tw * 0.25f, ry = max_float(5.0f, height * 0.30f);
        DrawEllipse((int)p.x, (int)(roof_y - ry * 0.20f), rx, ry, (Color){40, 108, 91, 238});
        DrawEllipseLines((int)p.x, (int)(roof_y - ry * 0.20f), rx, ry, (Color){193, 224, 146, 255});
        DrawLineEx((Vector2){p.x - rx * 0.72f, roof_y + ry * 0.35f}, (Vector2){p.x - rx * 0.35f, roof_y - ry * 0.78f}, 1.4f * detail_zoom, (Color){214, 194, 117, 255});
        DrawLineEx((Vector2){p.x + rx * 0.72f, roof_y + ry * 0.35f}, (Vector2){p.x + rx * 0.35f, roof_y - ry * 0.78f}, 1.4f * detail_zoom, (Color){214, 194, 117, 255});
        for (int leaf = -2; leaf <= 2; ++leaf) {
            DrawLineEx((Vector2){p.x + (float)leaf * tw * 0.055f, roof_y + 1.0f * detail_zoom},
                      (Vector2){p.x + (float)leaf * tw * 0.075f, roof_y - height * 0.19f},
                      1.5f * detail_zoom, (Color){141, 221, 132, 255});
        }
    } else if (b->type == BT_HEPHAESTUS) {
        /* Twin smelter stacks, glowing furnace door and copper gantry. */
        float roof_y = p.y - height;
        for (int stack = -1; stack <= 1; stack += 2) {
            float sx = p.x + (float)stack * tw * 0.17f;
            DrawRectangle((int)(sx - tw * 0.045f), (int)(roof_y - height * 0.43f), max_int(4, (int)(tw * 0.09f)), max_int(6, (int)(height * 0.43f)), (Color){89, 49, 43, 255});
            DrawRectangle((int)(sx - tw * 0.06f), (int)(roof_y - height * 0.43f), max_int(5, (int)(tw * 0.12f)), max_int(2, (int)(3.0f * detail_zoom)), (Color){232, 155, 79, 255});
        }
        DrawRectangle((int)(p.x - tw * 0.13f), (int)(roof_y - height * 0.14f), max_int(7, (int)(tw * 0.26f)), max_int(5, (int)(height * 0.14f)), (Color){247, 143, 62, 255});
        DrawRectangleLines((int)(p.x - tw * 0.13f), (int)(roof_y - height * 0.14f), max_int(7, (int)(tw * 0.26f)), max_int(5, (int)(height * 0.14f)), (Color){255, 206, 126, 255});
        Vector2 gantry = {(fb.x - fa.x) * 0.28f, (fb.y - fa.y) * 0.28f};
        DrawLineEx((Vector2){p.x - gantry.x, roof_y - height * 0.08f - gantry.y}, (Vector2){p.x + gantry.x, roof_y - height * 0.08f + gantry.y},
                   max_float(2.0f, 3.0f * detail_zoom), (Color){239, 171, 92, 255});
    } else if (b->type == BT_FORTRESS_FOLATRE) {
        /* Violet pleasure palace, four gold turrets, central marquee. */
        float roof_y = p.y - height;
        for (int sx = -1; sx <= 1; sx += 2) for (int sy = -1; sy <= 1; sy += 2) {
            float tx = p.x + (float)sx * tw * 0.22f;
            float ty = roof_y + (float)sy * th * 0.14f;
            DrawRectangle((int)(tx - tw * 0.035f), (int)(ty - height * 0.24f), max_int(4, (int)(tw * 0.07f)), max_int(5, (int)(height * 0.24f)), (Color){69, 46, 91, 255});
            DrawTriangle((Vector2){tx - tw * 0.055f, ty - height * 0.24f}, (Vector2){tx + tw * 0.055f, ty - height * 0.24f}, (Vector2){tx, ty - height * 0.37f}, (Color){231, 178, 113, 255});
        }
        DrawRectangle((int)(p.x - tw * 0.18f), (int)(roof_y - height * 0.25f), max_int(8, (int)(tw * 0.36f)), max_int(4, (int)(height * 0.12f)), (Color){240, 180, 113, 255});
        Vector2 rail = {(fc.x - fd.x) * 0.22f, (fc.y - fd.y) * 0.22f};
        DrawLineEx((Vector2){p.x - rail.x, roof_y - height * 0.05f - rail.y}, (Vector2){p.x + rail.x, roof_y - height * 0.05f + rail.y},
                   max_float(2.0f, 3.0f * detail_zoom), (Color){248, 207, 150, 255});
    }
    if (under_construction) draw_construction_overlay(b, p, tw, th, height, ui->zoom, progress);
    if (b->condition < 45) {
        DrawLine((int)(p.x - tw * 0.12f), (int)(p.y - height * 0.55f), (int)(p.x + tw * 0.08f), (int)(p.y - height * 0.30f), C_RED);
    }
}

/* Depth of the closest (front-most) ground corner of an object's footprint.
   Unlike pairwise rectangle comparisons, this gives every building one stable
   total ordering, avoiding intermittent cycles where a large footprint could be
   drawn in front of a building placed behind it. */
static int building_depth_key(const Building *b)
{
    int fw = b->footprint_w > 0 ? b->footprint_w : 1;
    int fh = b->footprint_h > 0 ? b->footprint_h : 1;
    return b->x + b->y + fw + fh - 2;
}

static bool building_draw_before(const Building *a, int ai, const Building *b, int bi)
{
    int ad = building_depth_key(a), bd = building_depth_key(b);
    if (ad != bd) return ad < bd;
    /* At equal depth, follow the map's left-to-right diagonal traversal. */
    if (a->x != b->x) return a->x < b->x;
    if (a->y != b->y) return a->y < b->y;
    return ai < bi;
}

/* Rock formation or fissure blocking construction until a crew clears it (see
   Sim_StartObstacleClearing). Drawn flat over the seabed diamond. conn[0..3] = west/east/
   north/south neighbour tiles that share the same obstacle, in the corridor/pipe convention
   (dirx/diry below), so a multi-tile formation reads as one connected mass instead of four
   separate icons repeating edge to edge. */
static void draw_obstacle(Vector2 p, float zoom, uint8_t kind, bool highlight, const bool conn[4])
{
    float s = TILE_W * zoom;
    const float dirx[4] = {-1.0f, 1.0f, 1.0f, -1.0f};
    const float diry[4] = {-1.0f, 1.0f, -1.0f, 1.0f};
    if (kind == TERRAIN_OBSTACLE_FISSURE) {
        /* A jagged crack. Where the fissure continues into a neighbour, it runs flush to
           that tile edge so consecutive tiles form one unbroken rift; where it dead-ends,
           it tapers to a point partway across the tile instead of stopping on the seam. */
        const Color crack = (Color){34, 16, 13, 255};
        const Color mid = (Color){96, 42, 30, 255};
        const Color glow = (Color){232, 120, 68, 230};
        const Color glow_hot = (Color){255, 189, 110, 255};
        float jitter[2] = {0.10f, -0.07f};
        Vector2 west_end = conn[0] ? (Vector2){p.x - s * 0.5f, p.y + jitter[0] * s - s * 0.5f * 0.0f}
                                    : (Vector2){p.x - s * 0.30f, p.y - s * 0.02f};
        Vector2 east_end = conn[1] ? (Vector2){p.x + s * 0.5f, p.y + jitter[1] * s}
                                    : (Vector2){p.x + s * 0.30f, p.y + s * 0.03f};
        west_end.y = p.y + (conn[0] ? -s * 0.03f : -s * 0.05f);
        east_end.y = p.y + (conn[1] ? s * 0.03f : s * 0.04f);
        Vector2 mid1 = {p.x - s * 0.08f, p.y + s * 0.08f};
        Vector2 mid2 = {p.x + s * 0.06f, p.y - s * 0.05f};
        Vector2 chain[4] = {west_end, mid1, mid2, east_end};
        for (int i = 0; i < 3; ++i) {
            DrawLineEx(chain[i], chain[i + 1], 5.0f * zoom, crack);
            DrawLineEx(chain[i], chain[i + 1], 3.0f * zoom, mid);
            DrawLineEx(chain[i], chain[i + 1], 1.4f * zoom, i == 1 ? glow_hot : glow);
        }
        /* A second, shorter crack along the other axis when the formation is isolated, so a
           single fissure tile doesn't read as just a straight scratch. */
        if (!conn[0] && !conn[1] && !conn[2] && !conn[3]) {
            Vector2 n1 = {p.x - s * 0.04f, p.y - s * 0.22f};
            Vector2 n2 = {p.x + s * 0.03f, p.y + s * 0.18f};
            DrawLineEx(n1, mid1, 3.5f * zoom, crack);
            DrawLineEx(n1, mid1, 1.0f * zoom, glow);
            DrawLineEx(mid2, n2, 3.5f * zoom, crack);
            DrawLineEx(mid2, n2, 1.0f * zoom, glow);
        }
        (void)dirx; (void)diry;
    } else {
        /* An irregular boulder pile: several overlapping facets of varying size and a sharp
           highlight edge, plus scattered pebbles and silt for texture. A connected neighbour
           gets a bridging slab so a 2x2 field reads as one outcrop. */
        const Color rock_dark = (Color){46, 50, 54, 255};
        const Color rock_mid = (Color){68, 74, 79, 255};
        const Color rock_lit = (Color){98, 105, 111, 255};
        const Color rock_edge = (Color){132, 141, 148, 255};
        Vector2 verts[4][3] = {
            {{p.x - s * 0.30f, p.y + s * 0.10f}, {p.x - s * 0.08f, p.y - s * 0.20f}, {p.x + s * 0.06f, p.y + s * 0.04f}},
            {{p.x - s * 0.04f, p.y + s * 0.12f}, {p.x + s * 0.16f, p.y - s * 0.14f}, {p.x + s * 0.32f, p.y + s * 0.10f}},
            {{p.x - s * 0.18f, p.y + s * 0.12f}, {p.x + s * 0.02f, p.y - s * 0.02f}, {p.x + s * 0.18f, p.y + s * 0.12f}},
            {{p.x - s * 0.10f, p.y - s * 0.06f}, {p.x + s * 0.08f, p.y - s * 0.20f}, {p.x + s * 0.14f, p.y - s * 0.02f}}
        };
        Color facet_colors[4] = {rock_dark, rock_mid, rock_lit, rock_mid};
        for (int k = 0; k < 4; ++k) fill_tri_any_winding(verts[k][0], verts[k][1], verts[k][2], facet_colors[k]);
        DrawLineEx(verts[1][1], verts[1][2], max_float(1.0f, 1.2f * zoom), rock_edge);
        DrawLineEx(verts[0][1], verts[1][1], max_float(1.0f, 1.0f * zoom), shade(rock_edge, -10));
        for (int k = 0; k < 4; ++k) {
            if (!conn[k]) continue;
            Vector2 edge_mid = {p.x + dirx[k] * s * 0.42f, p.y + diry[k] * s * 0.18f};
            fill_tri_any_winding((Vector2){p.x, p.y + s * 0.06f}, edge_mid, (Vector2){edge_mid.x, edge_mid.y - s * 0.12f}, rock_mid);
        }
        for (int d = 0; d < 5; ++d) {
            float px = p.x + ((float)((d * 37 + 11) % 23) / 23.0f - 0.5f) * s * 0.5f;
            float py = p.y + ((float)((d * 53 + 7) % 19) / 19.0f - 0.5f) * s * 0.22f + s * 0.06f;
            DrawCircle((int)px, (int)py, max_float(1.0f, (0.9f + 0.5f * (float)(d % 2)) * zoom), (Color){35, 40, 44, 200});
        }
    }
    if (highlight) DrawEllipseLines((int)p.x, (int)(p.y + s * 0.07f), s * 0.36f, s * 0.14f, (Color){255, 214, 120, 180});
}

static void draw_map(const GameState *g, const GameUI *ui, Rectangle vp)
{
    draw_ocean(vp);
    BeginScissorMode((int)vp.x, (int)vp.y, (int)vp.width, (int)vp.height);
    for (int diag = 0; diag <= MAP_W + MAP_H - 2; ++diag) {
        int x0 = max_int(0, diag - (MAP_H - 1));
        int x1 = min_int(MAP_W - 1, diag);
        for (int x = x0; x <= x1; ++x) {
            int y = diag - x;
            Vector2 p = tile_to_screen(ui, vp, (float)x, (float)y);
            if (p.x < vp.x - TILE_W * ui->zoom || p.x > vp.x + vp.width + TILE_W * ui->zoom ||
                p.y < vp.y - 120.0f * ui->zoom || p.y > vp.y + vp.height + TILE_H * ui->zoom) continue;
            int terrain = (int)g->map[x][y].terrain;
            Color a = (Color){16, 44, 59, 255};
            Color b = (Color){13, 36, 52, 255};
            if (terrain == 1) { a = (Color){17, 49, 61, 255}; b = (Color){11, 32, 47, 255}; }
            if (terrain == 2) { a = (Color){20, 51, 61, 255}; b = (Color){13, 34, 48, 255}; }
            if (terrain == 3) { a = (Color){14, 39, 54, 255}; b = (Color){9, 28, 42, 255}; }
            if (terrain == 4) { a = (Color){22, 48, 58, 255}; b = (Color){12, 31, 44, 255}; }
            draw_diamond(p, TILE_W * ui->zoom, TILE_H * ui->zoom, a, b, shade(b, 3), g->show_grid);
            if (((x * 13 + y * 7) % 83) == 0) {
                DrawCircle((int)(p.x + (float)((x + y) % 7) - 3.0f), (int)(p.y + 3), (float)max_int(1, (int)ui->zoom), (Color){64, 133, 145, 130});
            }
            if (IS_OBSTACLE_TERRAIN(g->map[x][y].terrain)) {
                uint8_t kind = g->map[x][y].terrain;
                bool conn[4] = {
                    x > 0 && g->map[x - 1][y].terrain == kind,
                    x + 1 < MAP_W && g->map[x + 1][y].terrain == kind,
                    y > 0 && g->map[x][y - 1].terrain == kind,
                    y + 1 < MAP_H && g->map[x][y + 1].terrain == kind
                };
                draw_obstacle(p, ui->zoom, kind, ui->clearing_select_mode, conn);
            }
        }
    }

    /* Sort all visible buildings by the front-most corner of their full footprint.
       Sorting the whole scene (rather than inferring pairwise overlap) prevents a
       large laboratory, factory or market from jumping in front of a residence. */
    int visible[MAX_BUILDINGS];
    int visible_count = 0;
    for (int bi = 0; bi < g->building_slots && visible_count < MAX_BUILDINGS; ++bi) {
        const Building *placed = &g->buildings[bi];
        if (!placed->active) continue;
        int fw = placed->footprint_w > 0 ? placed->footprint_w : 1;
        int fh = placed->footprint_h > 0 ? placed->footprint_h : 1;
        Vector2 center = tile_to_screen(ui, vp, (float)placed->x + ((float)fw - 1.0f) * 0.5f,
                                       (float)placed->y + ((float)fh - 1.0f) * 0.5f);
        float extent_x = (float)(fw + fh) * TILE_W * ui->zoom * 0.5f;
        float extent_y = (float)(fw + fh) * TILE_H * ui->zoom * 0.5f;
        if (center.x + extent_x < vp.x || center.x - extent_x > vp.x + vp.width ||
            center.y + extent_y < vp.y || center.y - (120.0f * ui->zoom) > vp.y + vp.height) continue;
        visible[visible_count++] = bi;
    }
    for (int i = 1; i < visible_count; ++i) {
        int key = visible[i];
        int j = i - 1;
        while (j >= 0 && building_draw_before(&g->buildings[key], key,
                                               &g->buildings[visible[j]], visible[j])) {
            visible[j + 1] = visible[j];
            --j;
        }
        visible[j + 1] = key;
    }
    for (int i = 0; i < visible_count; ++i) draw_building(g, ui, vp, visible[i]);

    /* Current tool ghost and selected-property footprint. */
    Vector2 mouse = GetMousePosition();
    int mx, my;
    if (screen_to_tile(ui, vp, mouse, &mx, &my)) {
        if (ui->selected_building_type >= 0 || ui->demolish_mode) {
            int fw = 1, fh = 1;
            if (ui->selected_building_type >= 0) {
                Sim_GetDefaultFootprint((BuildingType)ui->selected_building_type, &fw, &fh);
                if (ui->rotate_building) { int tmp = fw; fw = fh; fh = tmp; }
            }
            Color c = ui->selected_building_type >= 0 ? (render_footprint_clear(g, (BuildingType)ui->selected_building_type, mx, my, ui->rotate_building) ? C_TEAL : C_RED) : (g->map[mx][my].building == 0 ? C_TEAL : C_RED);
            if (ui->selected_building_type >= 0 && tech_locked(g, (BuildingType)ui->selected_building_type)) c = C_RED;
            draw_footprint_shape(ui, vp, mx, my, fw, fh, (Color){0, 0, 0, 0}, c, 2.0f);
        }
    }
    if (ui->selected_building_index >= 0 && ui->selected_building_index < g->building_slots && g->buildings[ui->selected_building_index].active) {
        const Building *b = &g->buildings[ui->selected_building_index];
        int fw = b->footprint_w > 0 ? b->footprint_w : 1;
        int fh = b->footprint_h > 0 ? b->footprint_h : 1;
        draw_footprint_shape(ui, vp, b->x, b->y, fw, fh, (Color){0, 0, 0, 0}, C_GOLD, 2.0f);
    }
    EndScissorMode();
}

static bool draw_button(Rectangle r, const char *label, bool active, bool enabled, int font_size)
{
    Vector2 mouse = GetMousePosition();
    bool hover = point_in(mouse, r);
    Color bg = active ? (Color){38, 102, 112, 255} : C_PANEL_2;
    if (!enabled) bg = (Color){19, 34, 45, 255};
    else if (hover) bg = shade(bg, 18);
    DrawRectangleRounded(r, 0.13f, 4, bg);
    DrawRectangleRoundedLines(r, 0.13f, 4, active ? C_TEAL : C_EDGE);
    int tw = MeasureText(label, font_size);
    DrawText(label, (int)(r.x + (r.width - (float)tw) * .5f), (int)(r.y + (r.height - (float)font_size) * .5f), font_size, enabled ? C_TEXT : C_MUTED);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && enabled;
}

static void draw_panel_title(const char *label, float x, float y)
{
    DrawText(label, (int)x, (int)y, 12, C_GOLD);
    DrawLine((int)x, (int)y + 17, (int)x + 240, (int)y + 17, C_EDGE);
}

static void draw_bar(float x, float y, float w, float h, float value, Color color)
{
    DrawRectangle((int)x, (int)y, (int)w, (int)h, (Color){23, 45, 58, 255});
    DrawRectangle((int)x, (int)y, (int)(w * clamp_float(value, 0.0f, 1.0f)), (int)h, color);
    DrawRectangleLines((int)x, (int)y, (int)w, (int)h, (Color){49, 81, 92, 255});
}

static void draw_header(GameUI *ui, GameState *g)
{
    int sw = GetScreenWidth();
    DrawRectangleGradientV(0, 0, sw, TOP_H, (Color){17, 40, 57, 255}, (Color){8, 23, 39, 255});
    DrawLine(0, TOP_H - 1, sw, TOP_H - 1, C_EDGE);
    DrawText("RAPTURE", 16, 9, 24, C_GOLD);
    DrawText("DREAM OF THE DEEP  /  ANDREW RYAN", 18, 37, 9, C_MUTED);

    DrawText(TextFormat("$ %d", g->money), 267, 15, 18, g->money < 10000 ? C_RED : C_TEXT);
    DrawText("TRESORERIE", 268, 39, 9, C_MUTED);
    DrawText(TextFormat("%d", g->population), 390, 15, 20, C_TEXT);
    DrawText("HABITANTS", 390, 39, 9, C_MUTED);
    DrawText(TextFormat("%.0f%%", g->average_happiness), 492, 15, 20, g->average_happiness < 40 ? C_RED : C_TEAL);
    DrawText("SATISFACTION", 493, 39, 9, C_MUTED);
    const char *date_text = TextFormat("%02d / %02d / %d", g->day, g->month, g->year);
    DrawText(date_text, 607, 18, 15, C_TEXT);
    DrawText(g->mode == MODE_CAMPAIGN ? "CAMPAGNE" : "BAC A SABLE", 608, 39, 9, C_MUTED);
    {
        /* News history button, right next to the scrolling date. On narrow windows it
           drops under the date instead of colliding with the speed controls. */
        float date_end = 607.0f + (float)MeasureText(date_text, 15);
        Rectangle news_btn = {date_end + 10.0f, 17.0f, 46.0f, 26.0f};
        if (news_btn.x + news_btn.width > (float)(sw - 358 - 8)) {
            float label_end = 608.0f + (float)max_int(MeasureText("CAMPAGNE", 9), MeasureText("BAC A SABLE", 9));
            news_btn = (Rectangle){label_end + 12.0f, 36.0f, 46.0f, 22.0f};
        }
        if (draw_button(news_btn, "NEWS", ui->show_news, true, 10)) open_news_history(ui, g);
        if (ui->news_unread > 0) {
            float bx = news_btn.x + news_btn.width - 3.0f, by = news_btn.y + 1.0f;
            DrawCircle((int)bx, (int)by, 7.0f, C_RED);
            const char *count = TextFormat("%d", min_int(ui->news_unread, 9));
            DrawText(count, (int)bx - MeasureText(count, 9) / 2, (int)by - 4, 9, C_TEXT);
        }
    }

    int x = sw - 358;
    draw_button((Rectangle){(float)x, 15, 32, 32}, g->paused ? ">" : "||", false, true, 14); x += 38;
    for (int i = 1; i <= 3; ++i) {
        bool active = (g->speed == i && !g->paused);
        char label[8]; (void)snprintf(label, sizeof(label), "%dx", i);
        draw_button((Rectangle){(float)x, 15, 34, 32}, label, active, true, 11); x += 37;
    }
    draw_button((Rectangle){(float)x, 15, 55, 32}, "SAVE", false, true, 10); x += 59;
    draw_button((Rectangle){(float)x, 15, 52, 32}, "LOAD", false, true, 10); x += 56;
    draw_button((Rectangle){(float)x, 15, 48, 32}, "NEW", false, true, 10);

    /* These hit tests mirror the drawn controls. */
    Vector2 m = GetMousePosition();
    int bx = sw - 358;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (point_in(m, (Rectangle){(float)bx, 15, 32, 32})) g->paused = !g->paused;
        bx += 38;
        for (int i = 1; i <= 3; ++i) {
            if (point_in(m, (Rectangle){(float)bx, 15, 34, 32})) { g->speed = i; g->paused = false; }
            bx += 37;
        }
        if (point_in(m, (Rectangle){(float)bx, 15, 55, 32})) {
            open_save_menu(ui, g, false, false, false);
        }
        bx += 59;
        if (point_in(m, (Rectangle){(float)bx, 15, 52, 32})) {
            open_save_menu(ui, g, true, false, false);
        }
        bx += 56;
        if (point_in(m, (Rectangle){(float)bx, 15, 48, 32})) {
            ui->screen = SCREEN_TITLE;
            ui->selected_building_type = -1; ui->selected_building_index = -1;
            g->paused = true;
        }
    }
}

static int visible_build_rows(int screen_h)
{
    int available = screen_h - 64 - 40 - 126 - 54;
    return max_int(5, available / 35);
}

/* Keep the remarkable buildings visible near the sea-access tools rather than
   burying the fishery below the fold on smaller screens. Enum values remain
   stable for save-file compatibility; only the presentation order changes. */
static const BuildingType BUILD_CATALOG_ORDER[BT_COUNT] = {
    /* Put the everyday access and support infrastructure first. These two
       tools are immediately available and should never be buried under locks. */
    BT_CORRIDOR, BT_PIPE, BT_SECURITY, BT_RESIDENCE, BT_RICH_RESIDENCE, BT_MARKET, BT_FACTORY,
    BT_LABORATORY, BT_POWER, BT_WATER, BT_OXYGEN, BT_FOOD, BT_GREENHOUSE_DOME, BT_HOSPITAL,
    BT_LEISURE, BT_AIRLOCK, BT_BATHYSPHERE, BT_FISHERY, BT_ARCADIA,
    BT_HEPHAESTUS, BT_FORTRESS_FOLATRE, BT_METRO, BT_RECYCLING,
    BT_GENETICS, BT_CINEMA, BT_RESTAURANT, BT_BOUTIQUE
};

static void draw_wrapped_text(Font font, const char *text, Rectangle area, float font_size, float spacing, Color color)
{
    if (!text || area.width <= 1.0f || area.height <= 1.0f) return;
    char line[256] = {0};
    const size_t line_capacity = sizeof(line) - 1;
    size_t len = strlen(text);
    float y = area.y;
    const float line_height = font_size + 2.0f;
    size_t i = 0;
    while (i < len && y + line_height <= area.y + area.height) {
        while (i < len && (text[i] == ' ' || text[i] == '\t')) i++;
        if (i >= len) break;
        if (text[i] == '\n') { y += line_height; i++; line[0] = '\0'; continue; }
        size_t word_start = i;
        while (i < len && text[i] != ' ' && text[i] != '\t' && text[i] != '\n') i++;
        size_t word_len = i - word_start;
        if (word_len > line_capacity) word_len = line_capacity;

        char candidate[256];
        size_t current_len = strlen(line);
        size_t separator = current_len > 0 ? 1u : 0u;
        size_t candidate_word_len = word_len;
        size_t candidate_room = line_capacity - current_len;
        if (separator > candidate_room) separator = candidate_room;
        candidate_room -= separator;
        if (candidate_word_len > candidate_room) candidate_word_len = candidate_room;
        memcpy(candidate, line, current_len);
        size_t candidate_at = current_len;
        if (separator > 0) candidate[candidate_at++] = ' ';
        memcpy(candidate + candidate_at, text + word_start, candidate_word_len);
        candidate[candidate_at + candidate_word_len] = '\0';

        if (line[0] != '\0' && MeasureTextEx(font, candidate, font_size, spacing).x > area.width) {
            DrawTextEx(font, line, (Vector2){area.x, y}, font_size, spacing, color);
            y += line_height;
            line[0] = '\0';
            size_t copy_len = word_len > line_capacity ? line_capacity : word_len;
            memcpy(line, text + word_start, copy_len);
            line[copy_len] = '\0';
        } else {
            current_len = strlen(line);
            size_t at = current_len;
            if (at > 0 && at < line_capacity) line[at++] = ' ';
            size_t room = line_capacity - at;
            size_t copy_len = word_len < room ? word_len : room;
            memcpy(line + at, text + word_start, copy_len);
            line[at + copy_len] = '\0';
        }
    }
    if (line[0] != '\0' && y + line_height <= area.y + area.height)
        DrawTextEx(font, line, (Vector2){area.x, y}, font_size, spacing, color);
}

static void draw_building_tooltip(const GameState *g, const GameUI *ui, BuildingType type, Vector2 mouse)
{
    if (type < 0 || type >= BT_COUNT) return;
    const BuildingDef *def = &BUILDING_DEFS[type];
    int fw = 1, fh = 1;
    Sim_GetDefaultFootprint(type, &fw, &fh);
    if (ui && ui->rotate_building) { int tmp = fw; fw = fh; fh = tmp; }
    const float x = (float)LEFT_W + 7.0f;
    float width = (float)GetScreenWidth() - (float)LEFT_W - (float)RIGHT_W - 18.0f;
    if (width > 300.0f) width = 300.0f;
    if (width < 230.0f) width = 230.0f;
    const float height = 202.0f;
    float y = mouse.y + 13.0f;
    float max_y = (float)GetScreenHeight() - FOOT_H - height - 4.0f;
    if (y > max_y) y = max_y;
    if (y < TOP_H + 4.0f) y = TOP_H + 4.0f;
    Rectangle box = {x, y, width, height};
    DrawRectangle((int)box.x + 3, (int)box.y + 4, (int)box.width, (int)box.height, (Color){0, 0, 0, 100});
    DrawRectangleRec(box, (Color){7, 23, 38, 247});
    DrawRectangleLinesEx(box, 1.0f, C_GOLD);
    DrawText(def->name, (int)x + 10, (int)y + 7, 15, C_GOLD);
    draw_wrapped_text(GetFontDefault(), TextFormat("Construction : %d jours  |  Emprise : %dx%d cases", Sim_GetConstructionDays(type), fw, fh),
                      (Rectangle){x + 10.0f, y + 29.0f, width - 20.0f, 16.0f}, 10.0f, 1.0f, C_TEAL);
    draw_wrapped_text(GetFontDefault(), TextFormat("Cout : $%d  |  Materiaux : %d  |  Entretien : $%d/mois", def->cost, def->material_cost, def->upkeep),
                      (Rectangle){x + 10.0f, y + 47.0f, width - 20.0f, 25.0f}, 10.0f, 1.0f, C_TEXT);
    Rectangle desc = {x + 10.0f, y + 75.0f, width - 20.0f, 43.0f};
    draw_wrapped_text(GetFontDefault(), def->description, desc, 11.0f, 1.0f, C_MUTED);
    char requirement[256] = "Conditions : aucune.";
    bool unique = type == BT_FISHERY || type == BT_ARCADIA || type == BT_HEPHAESTUS || type == BT_FORTRESS_FOLATRE;
    switch (type) {
        case BT_FISHERY: (void)snprintf(requirement, sizeof(requirement), "UNIQUE | Aquaculture + sas ou bathysphere operationnelle."); break;
        case BT_ARCADIA: (void)snprintf(requirement, sizeof(requirement), "UNIQUE | 3 fermes marines operationnelles minimum."); break;
        case BT_HEPHAESTUS: (void)snprintf(requirement, sizeof(requirement), "UNIQUE | 3 usines operationnelles minimum."); break;
        case BT_FORTRESS_FOLATRE: (void)snprintf(requirement, sizeof(requirement), "UNIQUE | 3 salles de fete operationnelles minimum."); break;
        default: break;
    }
    char current_reason[160];
    bool special_met = render_special_requirement(g, type, current_reason, sizeof(current_reason));
    bool research_locked = tech_locked(g, type);
    bool requirements_met = special_met && !research_locked;
    if (research_locked) {
        /* Name the technology that has to be researched first. */
        const char *tech_name = TECH_NAMES[def->unlock_tech];
        char base[160];
        (void)snprintf(base, sizeof(base), "%s", requirement);
        if (unique) (void)snprintf(requirement, sizeof(requirement), "Recherche requise : %s | %s", tech_name, base);
        else (void)snprintf(requirement, sizeof(requirement), "Recherche requise : %s", tech_name);
    } else if (unique && !special_met) {
        (void)snprintf(requirement, sizeof(requirement), "%s", current_reason);
    }
    Rectangle req_rect = {x + 10.0f, y + 122.0f, width - 20.0f, 48.0f};
    draw_wrapped_text(GetFontDefault(), requirement, req_rect, 10.0f, 1.0f, requirements_met ? C_GREEN : C_RED);
    DrawText(TextFormat("Rotation : R (%s)", (fw == 1 && fh == 1) ? "sans effet" : (fh > fw ? "verticale" : "horizontale")), (int)x + 10, (int)y + 177, 10, (Color){142, 174, 179, 255});
}

static void draw_left_panel(GameUI *ui, GameState *g)
{
    Rectangle panel = get_layout().left_panel;
    DrawRectangleRec(panel, C_PANEL);
    DrawLine((int)panel.width - 1, (int)panel.y, (int)panel.width - 1, (int)(panel.y + panel.height), C_EDGE);
    DrawText("CONSTRUCTION", 14, 78, 15, C_TEXT);
    DrawText("Molette : services et batiments remarquables", 14, 99, 10, C_MUTED);
    DrawLine(12, 115, LEFT_W - 12, 115, C_EDGE);

    int rows = visible_build_rows(GetScreenHeight());
    ui->build_scroll = clamp_int(ui->build_scroll, 0, max_int(0, BT_COUNT - rows));
    /* Buildable types stay in their catalog order at the top; locked ones drop to the bottom,
       in their own catalog order, so unlocking one simply moves it up to join the buildable
       group instead of reshuffling the whole list. */
    BuildingType sorted_catalog[BT_COUNT];
    {
        int w = 0;
        for (int pass = 0; pass < 2; ++pass) {
            for (int k = 0; k < BT_COUNT; ++k) {
                BuildingType t = BUILD_CATALOG_ORDER[k];
                /* Only the requirement status is needed here; do not truncate an unused reason string. */
                bool locked = tech_locked(g, t) || !render_special_requirement(g, t, NULL, 0);
                if ((pass == 0 && !locked) || (pass == 1 && locked)) sorted_catalog[w++] = t;
            }
        }
    }
    Rectangle list_rect = {7, 121, (float)(LEFT_W - 22), (float)(rows * 35)};
    BeginScissorMode((int)list_rect.x, (int)list_rect.y, (int)list_rect.width, (int)list_rect.height);
    Vector2 mouse = GetMousePosition();
    int hovered_type = -1;
    for (int i = 0; i < rows; ++i) {
        int catalog_idx = ui->build_scroll + i;
        if (catalog_idx >= BT_COUNT) break;
        BuildingType type = sorted_catalog[catalog_idx];
        Rectangle r = {10, 122.0f + (float)i * 35.0f, (float)(LEFT_W - 30), 31};
        char requirement[160];
        bool special_ok = render_special_requirement(g, type, requirement, sizeof(requirement));
        bool locked = tech_locked(g, type) || !special_ok;
        bool hover = point_in(mouse, r);
        if (hover) hovered_type = (int)type;
        bool active = ui->selected_building_type == (int)type;
        Color bg = active ? (Color){39, 89, 97, 255} : (hover ? (Color){24, 52, 68, 255} : (Color){15, 36, 52, 255});
        if (locked) bg = (Color){17, 30, 41, 255};
        DrawRectangleRounded(r, 0.10f, 4, bg);
        DrawRectangle((int)r.x + 3, (int)r.y + 7, 5, 17, locked ? C_MUTED : building_color(type));
        DrawText(BUILDING_DEFS[type].name, (int)r.x + 14, (int)r.y + 4, 12, locked ? C_MUTED : C_TEXT);
        if (locked) {
            DrawText("LOCK", (int)(r.x + r.width - 38), (int)r.y + 12, 10, C_RED);
        } else {
            DrawText(TextFormat("%d$", BUILDING_DEFS[type].cost), (int)(r.x + r.width - 58), (int)r.y + 15, 10, C_GOLD);
        }
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (locked) {
                if (tech_locked(g, type)) {
                    UI_ShowToast(ui, TextFormat("Recherche requise : %s", TECH_NAMES[BUILDING_DEFS[type].unlock_tech]));
                } else {
                    UI_ShowToast(ui, requirement[0] ? requirement : "Conditions de construction non satisfaites.");
                }
            } else {
                ui->selected_building_type = (int)type;
                ui->rotate_building = false;
                ui->selected_building_index = -1;
                ui->clearing_select_mode = false;
                ui->selected_household_index = -1;
                ui->demolish_mode = false;
            ui->clearing_select_mode = false;
                UI_ShowToast(ui, TextFormat("Outil actif : %s. Cliquez sur une case libre.", BUILDING_DEFS[type].name));
            }
        }
    }
    EndScissorMode();
    draw_scrollbar((float)(LEFT_W - 11), list_rect.y, 4.0f, list_rect.height, BT_COUNT, rows, ui->build_scroll);
    if (hovered_type >= 0) draw_building_tooltip(g, ui, (BuildingType)hovered_type, mouse);

    int control_y = GetScreenHeight() - 76;
    draw_button((Rectangle){10, (float)control_y, 112, 28}, ui->demolish_mode ? "ANNULER X" : "DEMOLIR [X]", ui->demolish_mode, true, 10);
    draw_button((Rectangle){128, (float)control_y, 120, 28}, ui->selected_building_type >= 0 ? "ECHAP : ANNULER" : "OUTIL : AUCUN", false, true, 9);
    DrawText(TextFormat("Materiaux : %d", g->materials), 13, GetScreenHeight() - 106, 10, C_MUTED);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 m = GetMousePosition();
        if (point_in(m, (Rectangle){10, (float)control_y, 112, 28})) {
            ui->demolish_mode = !ui->demolish_mode;
            if (ui->demolish_mode) { ui->selected_building_type = -1; ui->clearing_select_mode = false; }
            UI_ShowToast(ui, ui->demolish_mode ? "Mode demolition : selectionnez un batiment." : "Mode demolition desactive.");
        }
        if (point_in(m, (Rectangle){128, (float)control_y, 120, 28})) {
            ui->selected_building_type = -1; ui->demolish_mode = false;
            ui->clearing_select_mode = false;
            UI_ShowToast(ui, "Outil de construction annule.");
        }
    }
    if (point_in(mouse, list_rect)) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) ui->build_scroll = clamp_int(ui->build_scroll - (wheel > 0 ? 2 : -2), 0, max_int(0, BT_COUNT - rows));
    }
}

static void draw_policy_row(GameUI *ui, Rectangle panel, float y, const char *label,
                            int *value, int step, int minimum, int maximum, const char *suffix)
{
    float x = panel.x + 12.0f;
    DrawText(label, (int)x, (int)y, 10, C_TEXT);
    DrawText(TextFormat("%d%s", *value, suffix), (int)(panel.x + 161.0f), (int)y, 10, C_GOLD);
    Rectangle minus = {panel.x + panel.width - 57.0f, y - 3.0f, 22.0f, 18.0f};
    Rectangle plus = {panel.x + panel.width - 31.0f, y - 3.0f, 22.0f, 18.0f};
    if (draw_button(minus, "-", false, *value > minimum, 10)) {
        *value = clamp_int(*value - step, minimum, maximum);
        UI_ShowToast(ui, TextFormat("Tarif modifie : %s", label));
    }
    if (draw_button(plus, "+", false, *value < maximum, 10)) {
        *value = clamp_int(*value + step, minimum, maximum);
        UI_ShowToast(ui, TextFormat("Tarif modifie : %s", label));
    }
}

static void draw_selected_building_details(GameUI *ui, GameState *g, Rectangle panel, float x, float y)
{
    int bi = ui->selected_building_index;
    if (bi < 0 || bi >= g->building_slots || !g->buildings[bi].active) return;
    Building *b = &g->buildings[bi];
    DrawText(BUILDING_DEFS[b->type].name, (int)x, (int)y, 12, C_GOLD); y += 22.0f;
    DrawText(TextFormat("Coordonnees : %d, %d", b->x, b->y), (int)x, (int)y, 10, C_MUTED); y += 17.0f;
    DrawText(TextFormat("Etat : %d%%", b->condition), (int)x, (int)y, 10, b->condition < 45 ? C_RED : C_TEXT);
    DrawText(TextFormat("Employes : %d", b->workers), (int)(x + 105.0f), (int)y, 10, C_TEXT);
    int repair_cost = max_int(150, (100 - b->condition) * BUILDING_DEFS[b->type].cost / 200);
    bool can_repair = b->condition < 100 && g->money >= repair_cost;
    if (draw_button((Rectangle){panel.x + panel.width - 76.0f, y - 4.0f, 64.0f, 21.0f}, "REPARER", false, can_repair, 8)) {
        g->money -= repair_cost;
        b->condition = 100;
        UI_ShowToast(ui, TextFormat("Batiment repare pour $%d.", repair_cost));
        Sim_UpdateStatistics(g);
    }
    y += 20.0f;
    if (b->construction_days_remaining > 0 && b->construction_days_total > 0) {
        int elapsed = b->construction_days_total - b->construction_days_remaining;
        DrawText(TextFormat("CHANTIER : %d / %d jours", elapsed, b->construction_days_total),
                 (int)x, (int)y, 10, C_GOLD);
        y += 17.0f;
        DrawText(TextFormat("Temps restant : %d jours", b->construction_days_remaining),
                 (int)x, (int)y, 9, C_TEAL);
        y += 18.0f;
    }
    if (b->type == BT_RESIDENCE) {
        DrawText(TextFormat("Occupants : %d / %d", b->residents, BUILDING_DEFS[BT_RESIDENCE].residents), (int)x, (int)y, 10, C_TEXT); y += 19.0f;
    } else if (b->firm_index > 0 && b->firm_index <= g->firm_slots && g->firms[b->firm_index - 1].active) {
        Firm *f = &g->firms[b->firm_index - 1];
        DrawText(TextFormat("Tresorerie : $%d", f->cash), (int)x, (int)y, 10, f->cash < 0 ? C_RED : C_TEXT); y += 18.0f;
        DrawText(TextFormat("Ventes/mois : $%d", f->monthly_revenue), (int)x, (int)y, 10, C_MUTED); y += 18.0f;
    } else if (b->closed) {
        DrawText("Entreprise fermee / faillite", (int)x, (int)y, 10, C_RED); y += 18.0f;
    }
    draw_wrapped_text(GetFontDefault(), BUILDING_DEFS[b->type].description,
                      (Rectangle){x, y, panel.x + panel.width - x - 12.0f, panel.y + panel.height - y - 8.0f},
                      10.0f, 1.0f, C_MUTED);
}

static void draw_vital_tooltip(Rectangle panel, Vector2 mouse, int vital_index)
{
    static const char *titles[5] = {"ELECTRICITE", "EAU POTABLE", "OXYGENE", "NOURRITURE", "BIENS"};
    static const char *body[5] = {
        "L'energie alimente les installations. Construisez une centrale, entretenez-la et raccordez des tuyaux proches pour ameliorer la distribution.",
        "L'eau soutient les foyers et certaines installations. Construisez une station de traitement, maintenez son etat et raccordez des conduites en cuivre.",
        "L'oxygene est indispensable aux habitants. Ajoutez une unite d'oxygene, entretenez-la et reliez-la au reseau de conduites.",
        "Les reserves viennent des fermes marines et des expeditions. Construisez des fermes, surveillez le stock et developpez l'aquaculture.",
        "Les biens repondent aux besoins des habitants. Developpez les boutiques et les commerces, puis veillez a la production et aux stocks."
    };
    if (vital_index < 0 || vital_index >= 5) return;
    float width = min_float(250.0f, panel.x - (float)LEFT_W - 16.0f);
    if (width < 175.0f) width = 175.0f;
    const float height = 106.0f;
    float x = panel.x - width - 9.0f;
    if (x < (float)LEFT_W + 3.0f) x = (float)LEFT_W + 3.0f;
    float y = mouse.y + 12.0f;
    if (y + height > (float)GetScreenHeight() - FOOT_H - 3.0f) y = (float)GetScreenHeight() - FOOT_H - height - 3.0f;
    if (y < (float)TOP_H + 3.0f) y = (float)TOP_H + 3.0f;
    Rectangle r = {x, y, width, height};
    DrawRectangle((int)r.x + 3, (int)r.y + 4, (int)r.width, (int)r.height, (Color){0, 0, 0, 110});
    DrawRectangleRec(r, (Color){7, 23, 38, 249});
    DrawRectangleLinesEx(r, 1.0f, C_TEAL);
    DrawText(titles[vital_index], (int)x + 10, (int)y + 7, 13, C_TEAL);
    draw_wrapped_text(GetFontDefault(), body[vital_index],
                      (Rectangle){x + 10.0f, y + 28.0f, width - 20.0f, height - 36.0f}, 10.0f, 1.0f, C_TEXT);
}

static void draw_overview_panel(GameUI *ui, GameState *g, Rectangle panel)
{
    float x = panel.x + 12.0f;
    float y = 123.0f;
    draw_panel_title("BILAN DE LA CITE", x, y);
    int damaged_count = 0;
    for (int bi = 0; bi < g->building_slots; ++bi)
        if (g->buildings[bi].active && g->buildings[bi].condition < 100 && g->buildings[bi].construction_days_remaining <= 0) damaged_count++;
    Rectangle repair_all = {panel.x + panel.width - 119.0f, y - 3.0f, 108.0f, 23.0f};
    if (draw_button(repair_all, "TOUT REPARER", false, damaged_count > 0 && g->money > 0, 9)) {
        int repaired = 0, spent = 0;
        for (int bi = 0; bi < g->building_slots; ++bi) {
            Building *b = &g->buildings[bi];
            if (!b->active || b->condition >= 100 || b->construction_days_remaining > 0) continue;
            int cost = max_int(150, (100 - b->condition) * BUILDING_DEFS[b->type].cost / 200);
            if (g->money < cost) continue;
            g->money -= cost; spent += cost; b->condition = 100; repaired++;
        }
        Sim_UpdateStatistics(g);
        if (repaired == 0) UI_ShowToast(ui, "Fonds insuffisants pour reparer les batiments.");
        else UI_ShowToast(ui, TextFormat("Maintenance groupee : %d batiment(s) repare(s), $%d depenses.%s", repaired, spent,
                                         repaired < damaged_count ? " Certains restent a reparer faute de fonds." : ""));
    }
    y += 25.0f;
    DrawText(TextFormat("Foyers %d  |  Habitants %d", g->households_count, g->population), (int)x, (int)y, 10, C_TEXT); y += 17.0f;
    DrawText(TextFormat("Emplois %d  |  Societes %d", g->employed, g->open_firms), (int)x, (int)y, 10, C_TEXT); y += 17.0f;
    DrawText(TextFormat("Recettes $%d  /  Depenses $%d", g->monthly_income, g->monthly_expenses), (int)x, (int)y, 10, g->monthly_income >= g->monthly_expenses ? C_GREEN : C_RED); y += 17.0f;
    char goal[160];
    (void)Sim_GetCampaignGoal(g, goal, (int)sizeof(goal));
    DrawText("OBJECTIF DE CAMPAGNE", (int)x, (int)y, 10, C_GOLD);
    draw_wrapped_text(GetFontDefault(), goal,
                      (Rectangle){x, y + 13.0f, panel.width - 24.0f, 26.0f},
                      10.0f, 1.0f, g->won ? C_GREEN : C_MUTED);
    y += 41.0f;

    {
        /* Attractiveness: recomputed from the current city, so the advice follows what the
           player builds. Only the most valuable fixes are listed. */
        AttractTip tips[ATTRACT_MAX_TIPS];
        int score = Sim_GetAttractiveness(g, tips, ATTRACT_MAX_TIPS);
        Color score_color = score < 40 ? C_RED : (score < 65 ? C_GOLD : C_TEAL);
        draw_panel_title("ATTRACTIVITE", x, y); y += 23.0f;
        DrawText(TextFormat("%d / 100", score), (int)x, (int)y, 10, score_color);
        const char *arrival_state;
        if (g->average_happiness < 30.0f) arrival_state = "Arrivees stoppees : trop de mecontentement";
        else if (Sim_CountAvailableHousing(g) <= 0) arrival_state = "Arrivees bloquees : aucun logement libre";
        else if (g->available_jobs < 1 && g->employment_ratio < 0.85f) arrival_state = "Arrivees bloquees : pas d'emplois libres";
        else if (score < 28) arrival_state = "Ville trop peu attractive : aucune arrivee";
        else arrival_state = TextFormat("Arrivees : environ %d foyers par mois", (int)((float)(score - 25) * 0.15f + 0.5f));
        draw_wrapped_text(GetFontDefault(), arrival_state, (Rectangle){x + 58.0f, y, panel.width - 92.0f, 25.0f}, 10.0f, 1.0f, C_MUTED);
        draw_bar(x, y + 13.0f, panel.width - 26.0f, 5.0f, (float)score / 100.0f, score_color);
        y += 24.0f;
        int shown = panel.height >= 660.0f ? 3 : 2;
        int printed = 0;
        for (int i = 0; i < shown && tips[i].text[0] != '\0'; ++i) {
            DrawText(TextFormat("+%d", tips[i].gain), (int)x, (int)y + 1, 10, C_GOLD);
            draw_wrapped_text(GetFontDefault(), tips[i].text, (Rectangle){x + 24.0f, y, panel.width - 50.0f, 26.0f}, 10.0f, 1.0f, C_TEXT);
            y += 27.0f;
            printed++;
        }
        if (printed == 0) { draw_wrapped_text(GetFontDefault(), "Rapture attire les foules : rien ne manque.", (Rectangle){x, y, panel.width - 24.0f, 22.0f}, 10.0f, 1.0f, C_GREEN); y += 22.0f; }
        y += 4.0f;
    }

    draw_panel_title("SYSTEMES VITAUX", x, y); y += 23.0f;
    const char *names[] = {"Electricite", "Eau potable", "Oxygene", "Nourriture", "Biens"};
    float vals[] = {g->power_ratio, g->water_ratio, g->oxygen_ratio, g->food_ratio, g->goods_ratio};
    Vector2 vital_mouse = GetMousePosition();
    int hovered_vital = -1;
    for (int i = 0; i < 5; ++i) {
        Rectangle vital_row = {x - 3.0f, y - 2.0f, panel.width - 18.0f, 21.0f};
        bool hover = point_in(vital_mouse, vital_row);
        if (hover) hovered_vital = i;
        if (hover) DrawRectangleRounded(vital_row, 0.12f, 3, (Color){24, 51, 65, 190});
        DrawText(names[i], (int)x, (int)y, 10, hover ? C_TEXT : C_MUTED);
        DrawText(TextFormat("%d%%", (int)(vals[i] * 100.0f)), (int)(panel.x + panel.width - 43.0f), (int)y, 10, vals[i] < .55f ? C_RED : C_TEXT);
        draw_bar(x, y + 13.0f, panel.width - 26.0f, 5.0f, vals[i], vals[i] < .55f ? C_RED : C_TEAL);
        y += 23.0f;
    }
    if (hovered_vital >= 0) draw_vital_tooltip(panel, vital_mouse, hovered_vital);

    y += 1.0f;
    draw_panel_title("TARIFS ET FISCALITE", x, y); y += 23.0f;
    draw_policy_row(ui, panel, y, "Impot sur revenus", &g->tax_rate, 1, 0, 35, "%"); y += 22.0f;
    draw_policy_row(ui, panel, y, "Taxe sur societes", &g->company_tax_rate, 1, 0, 20, "%"); y += 22.0f;
    draw_policy_row(ui, panel, y, "Taxe par foyer / mois", &g->housing_tax, 5, 0, 120, " $"); y += 22.0f;
    draw_policy_row(ui, panel, y, "Electricite / habitant", &g->electricity_price, 1, 0, 12, " $"); y += 22.0f;
    draw_policy_row(ui, panel, y, "Eau / habitant", &g->water_price, 1, 0, 12, " $"); y += 24.0f;

    if (ui->selected_building_index >= 0 && ui->selected_building_index < g->building_slots && g->buildings[ui->selected_building_index].active) {
        draw_panel_title("BATIMENT SELECTIONNE", x, y); y += 23.0f;
        draw_selected_building_details(ui, g, panel, x, y);
    } else {
        draw_panel_title("INVENTAIRE DES BATIMENTS", x, y); y += 20.0f;
        int list_top = (int)y;
        int list_bottom = (int)(panel.y + panel.height - 9.0f);
        int rows = max_int(1, (list_bottom - list_top) / 15);
        ui->inventory_scroll = clamp_int(ui->inventory_scroll, 0, max_int(0, BT_COUNT - rows));
        Rectangle list = {panel.x + 7.0f, (float)list_top, panel.width - 20.0f, (float)(rows * 15)};
        BeginScissorMode((int)list.x, (int)list.y, (int)list.width, (int)list.height);
        for (int row = 0; row < rows; ++row) {
            int idx = ui->inventory_scroll + row;
            if (idx >= BT_COUNT) break;
            float ry = (float)list_top + (float)row * 15.0f;
            if ((row % 2) == 0) DrawRectangle((int)list.x, (int)ry, (int)list.width, 15, (Color){15, 36, 52, 150});
            DrawRectangle((int)x, (int)ry + 4, 5, 7, building_color((BuildingType)idx));
            int quantity = Sim_CountBuildings(g, (BuildingType)idx);
            DrawText(BUILDING_DEFS[idx].name, (int)x + 10, (int)ry + 2, 10, quantity == 0 ? C_MUTED : C_TEXT);
            DrawText(TextFormat("%d", quantity), (int)(panel.x + panel.width - 31.0f), (int)ry + 2, 10, quantity == 0 ? C_RED : C_GOLD);
        }
        EndScissorMode();
        draw_scrollbar(panel.x + panel.width - 10.0f, list.y, 3.0f, list.height, BT_COUNT, rows, ui->inventory_scroll);
        if (point_in(GetMousePosition(), list)) {
            float wheel = GetMouseWheelMove();
            if (wheel != 0.0f) ui->inventory_scroll = clamp_int(ui->inventory_scroll - (wheel > 0 ? 2 : -2), 0, max_int(0, BT_COUNT - rows));
        }
    }
}

static void draw_research_panel(GameUI *ui, GameState *g, Rectangle panel)
{
    const char *short_names[TECH_COUNT] = {
        "Bathysph.", "Metro", "Sas", "Genetique", "Vie avanc.",
        "Production", "Recyclage", "Contrat", "Services", "Aquacult.",
        "Sous-mar.", "Commerces", "Ingenierie", "Marche", "Instrum.",
        "Metallur.", "Biotech.", "Spectacles", "Sylvicult.", "Urbanisme"
    };
    /* Four columns, six rows. Every prerequisite link joins two neighbouring rows, so no
       link ever runs behind another node. Branches: industry, science, society,
       commerce and ocean/transport (see the colour key under the title). */
    const int columns[TECH_COUNT] = {3, 3, 3, 1, 1, 0, 0, 2, 2, 2, 2, 3, 0, 3, 1, 0, 2, 3, 2, 1};
    const int rows[TECH_COUNT]    = {4, 5, 3, 2, 1, 1, 2, 0, 1, 2, 4, 1, 0, 0, 0, 3, 3, 2, 4, 5};
    /* 0 industry, 1 science, 2 society, 3 commerce, 4 ocean and transport. */
    const int branch[TECH_COUNT]  = {4, 4, 4, 1, 1, 0, 0, 2, 2, 1, 4, 3, 0, 3, 1, 0, 1, 2, 1, 2};
    const Color branch_colors[5] = {
        {214, 132, 72, 255}, {96, 196, 210, 255}, {168, 120, 214, 255}, {226, 190, 96, 255}, {86, 140, 226, 255}
    };
    const char *branch_names[5] = {"Industrie", "Science", "Societe", "Commerce", "Ocean"};
    const float x0 = panel.x + 8.0f;
    /* Use the panel origin so the title and first tree row stay below the tabs
       at every supported window size. */
    const float y0 = panel.y + 121.0f;
    const float gap_x = 4.0f;
    const float gap_y = 6.0f;
    const float node_w = (panel.width - 16.0f - 3.0f * gap_x) / 4.0f;
    const float node_h = 44.0f;
    const int tree_rows = 6;
    DrawText("ARBRE DES CONNAISSANCES", (int)(panel.x + 12), (int)(panel.y + 52.0f), 13, C_TEXT);
    DrawText(TextFormat("Points : %d RP  |  Tresorerie : $%d", g->research, g->money), (int)(panel.x + 12), (int)(panel.y + 72.0f), 10, C_TEAL);
    {
        float lx = panel.x + 12.0f;
        for (int k = 0; k < 5; ++k) {
            DrawRectangle((int)lx, (int)(panel.y + 92.0f), 5, 5, branch_colors[k]);
            DrawText(branch_names[k], (int)lx + 8, (int)(panel.y + 88.0f), 9, C_MUTED);
            lx += 12.0f + (float)MeasureText(branch_names[k], 9) + 5.0f;
        }
    }

    Rectangle nodes[TECH_COUNT];
    for (int t = 0; t < TECH_COUNT; ++t) {
        nodes[t] = (Rectangle){x0 + (float)columns[t] * (node_w + gap_x),
                               y0 + (float)rows[t] * (node_h + gap_y), node_w, node_h};
    }
    /* Draw prerequisite edges behind the nodes to make research order legible. */
    for (int t = 0; t < TECH_COUNT; ++t) {
        unsigned int mask = TECH_PREREQUISITES[t];
        for (int parent = 0; parent < TECH_COUNT; ++parent) {
            if ((mask & (1u << parent)) == 0u) continue;
            Vector2 a = {nodes[parent].x + nodes[parent].width * 0.5f, nodes[parent].y + nodes[parent].height};
            Vector2 b = {nodes[t].x + nodes[t].width * 0.5f, nodes[t].y};
            DrawLineEx(a, b, 1.8f, g->tech[parent] ? C_TEAL : C_EDGE);
        }
    }
    int hovered = -1;
    for (int t = 0; t < TECH_COUNT; ++t) {
        Rectangle r = nodes[t];
        bool done = g->tech[t];
        bool prereqs_met = true;
        for (int parent = 0; parent < TECH_COUNT; ++parent)
            if ((TECH_PREREQUISITES[t] & (1u << parent)) != 0u && !g->tech[parent]) prereqs_met = false;
        bool affordable = g->research >= TECH_COST_POINTS[t] && g->money >= TECH_COST_MONEY[t];
        bool hover = point_in(GetMousePosition(), r);
        if (hover) hovered = t;
        Color fill = done ? (Color){23, 67, 64, 255} : (!prereqs_met ? (Color){20, 27, 37, 255} : (hover ? (Color){25, 57, 70, 255} : (Color){15, 36, 52, 255}));
        DrawRectangleRounded(r, 0.10f, 4, fill);
        DrawRectangleRoundedLines(r, 0.10f, 4, done ? C_GREEN : (prereqs_met ? (hover ? C_TEAL : C_EDGE) : (Color){62, 72, 80, 255}));
        Color strip = branch_colors[branch[t]];
        if (!prereqs_met && !done) strip = shade(strip, -70);
        DrawRectangle((int)r.x + 3, (int)r.y + 6, 3, (int)r.height - 12, strip);
        DrawText(short_names[t], (int)r.x + 9, (int)r.y + 4, 10, done ? C_GREEN : (prereqs_met ? C_TEXT : C_MUTED));
        DrawText(TextFormat("%d RP", TECH_COST_POINTS[t]), (int)r.x + 9, (int)r.y + 18, 10, affordable ? C_GOLD : C_MUTED);
        DrawText(done ? "ACQUIS" : (prereqs_met ? (affordable ? "ETUDIER" : "BLOQUE $") : "PREREQUIS"),
                 (int)r.x + 9, (int)r.y + 30, 9, done ? C_GREEN : (prereqs_met && affordable ? C_TEAL : C_MUTED));
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !done) {
            char reason[160];
            if (Sim_Research(g, t, reason, (int)sizeof(reason))) UI_ShowToast(ui, reason);
            else UI_ShowToast(ui, reason);
        }
    }

    float details_y = y0 + (float)tree_rows * (node_h + gap_y) + 1.0f;
    DrawLine((int)(panel.x + 10), (int)details_y, (int)(panel.x + panel.width - 10), (int)details_y, C_EDGE);
    details_y += 8.0f;
    if (hovered >= 0) {
        int t = hovered;
        DrawText(TECH_NAMES[t], (int)(panel.x + 12), (int)details_y, 11, g->tech[t] ? C_GREEN : C_GOLD);
        details_y += 16.0f;
        DrawText(TextFormat("Investissement : $%d et %d RP", TECH_COST_MONEY[t], TECH_COST_POINTS[t]), (int)(panel.x + 12), (int)details_y, 10, C_TEXT);
        details_y += 15.0f;
        bool any_parent = false;
        for (int i = 0; i < TECH_COUNT; ++i) {
            if ((TECH_PREREQUISITES[t] & (1u << i)) == 0u) continue;
            DrawText(TextFormat("Prerequis : %s%s", TECH_NAMES[i], g->tech[i] ? " (acquis)" : " (a etudier)"),
                     (int)(panel.x + 12), (int)details_y, 9, g->tech[i] ? C_TEAL : C_RED);
            details_y += 14.0f;
            any_parent = true;
        }
        if (!any_parent) { DrawText("Prerequis : aucun", (int)(panel.x + 12), (int)details_y, 10, C_TEAL); details_y += 14.0f; }
        char unlocks[160] = "";
        size_t used = 0;
        for (int i = 0; i < BT_COUNT; ++i) {
            if (BUILDING_DEFS[i].unlock_tech != t) continue;
            int written = snprintf(unlocks + used, sizeof(unlocks) - used, "%s%s", used ? ", " : "", BUILDING_DEFS[i].name);
            if (written < 0 || (size_t)written >= sizeof(unlocks) - used) break;
            used += (size_t)written;
        }
        if (used > 0) {
            draw_wrapped_text(GetFontDefault(), TextFormat("Debloque : %s", unlocks),
                              (Rectangle){panel.x + 12.0f, details_y, panel.width - 24.0f, 26.0f},
                              9.0f, 1.0f, C_GOLD);
            details_y += 27.0f;
        }
        details_y += 3.0f;
        Rectangle desc = {panel.x + 12.0f, details_y, panel.width - 24.0f, panel.y + panel.height - details_y - 8.0f};
        draw_wrapped_text(GetFontDefault(), TECH_DESCRIPTIONS[t], desc, 10.0f, 1.0f, C_MUTED);
    } else {
        DrawText("Le savoir se debloque par branches.", (int)(panel.x + 12), (int)details_y, 10, C_TEXT);
        DrawText("Survolez un noeud pour ses prerequis et ses effets.", (int)(panel.x + 12), (int)details_y + 16, 10, C_MUTED);
        DrawText("Les prerequis sont obligatoires : suivez les liens.", (int)(panel.x + 12), (int)details_y + 30, 10, C_MUTED);
        DrawText("Laboratoires et ADAM generent de la recherche.", (int)(panel.x + 12), (int)details_y + 46, 10, C_TEAL);
    }
    if (point_in(GetMousePosition(), panel)) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) ui->right_scroll = 0;
    }
}

static void draw_companies_panel(GameUI *ui, GameState *g, Rectangle panel)
{
    float x = panel.x + 12, y = 123;
    draw_panel_title("ENTREPRISES PRIVEES", x, y); y += 25;
    DrawText("Les proprietaires gerent leur tresorerie.", (int)x, (int)y, 10, C_MUTED); y += 18;
    int rows = max_int(3, (int)((panel.y + panel.height - y - 8) / 68.0f));
    int active_total = Sim_GetFirmCount(g);
    ui->right_scroll = clamp_int(ui->right_scroll, 0, max_int(0, active_total - rows));
    Rectangle list = {panel.x + 7, y, panel.width - 14, (float)(rows * 68)};
    BeginScissorMode((int)list.x, (int)list.y, (int)list.width, (int)list.height);
    int visible = 0;
    int skipped = 0;
    for (int i = 0; i < g->firm_slots; ++i) {
        Firm *f = &g->firms[i];
        if (!f->active) continue;
        if (skipped++ < ui->right_scroll) continue;
        if (visible >= rows) break;
        int bi = f->building_index - 1;
        if (bi < 0 || bi >= g->building_slots || !g->buildings[bi].active) continue;
        Building *b = &g->buildings[bi];
        Rectangle r = {list.x + 2, y + (float)visible * 68 + 2, list.width - 4, 63};
        bool hover = point_in(GetMousePosition(), r);
        bool selected = ui->selected_building_index == bi;
        DrawRectangleRounded(r, 0.08f, 4, selected ? (Color){39, 79, 83, 255} : (hover ? (Color){25, 51, 67, 255} : (Color){15, 36, 52, 255}));
        DrawRectangle((int)r.x + 6, (int)r.y + 8, 5, 47, building_color(b->type));
        DrawText(BUILDING_DEFS[b->type].name, (int)r.x + 16, (int)r.y + 5, 11, C_TEXT);
        DrawText(TextFormat("Employes : %d   Paie : $%d", f->employees, f->employees * f->wage), (int)r.x + 16, (int)r.y + 22, 10, C_MUTED);
        DrawText(TextFormat("CA : $%d   Fonds : $%d", f->monthly_revenue, f->cash), (int)r.x + 16, (int)r.y + 38, 10, f->cash < 0 ? C_RED : C_GOLD);
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            ui->selected_building_index = bi;
            ui->selected_building_type = -1;
            ui->demolish_mode = false;
            ui->clearing_select_mode = false;
        }
        visible++;
    }
    EndScissorMode();
    draw_scrollbar(panel.x + panel.width - 10.0f, list.y, 3.0f, list.height, active_total, rows, ui->right_scroll);
    if (active_total == 0) DrawText("Aucune entreprise active.", (int)x, (int)y, 11, C_MUTED);
    DrawText(TextFormat("Foyers : %d  |  Materiaux : %d", g->households_count, g->materials), (int)x, (int)(panel.y + panel.height - 25), 10, C_MUTED);
    if (point_in(GetMousePosition(), list)) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) ui->right_scroll = clamp_int(ui->right_scroll - (wheel > 0 ? 1 : -1), 0, max_int(0, active_total - rows));
    }
}

static void draw_ocean_panel(GameUI *ui, GameState *g, Rectangle panel)
{
    float x = panel.x + 12.0f, y = 123.0f;
    draw_panel_title("EXPLORATION OCEANIQUE", x, y); y += 26.0f;
    DrawText("Envoyez des ouvriers recuperer", (int)x, (int)y, 10, C_TEXT); y += 14.0f;
    DrawText("des materiaux dans les profondeurs.", (int)x, (int)y, 10, C_TEXT); y += 24.0f;
    int access = Sim_CountBuildings(g, BT_AIRLOCK) + Sim_CountBuildings(g, BT_BATHYSPHERE);
    DrawText(TextFormat("Acces oceanique : %s", access > 0 ? "operationnel" : "indisponible"), (int)x, (int)y, 10, access > 0 ? C_GREEN : C_RED); y += 18.0f;
    if (g->tech[10]) {
        DrawText("Sous-marins : retour accelere actif", (int)x, (int)y, 10, C_TEAL);
        y += 15.0f;
    }
    int free_workers = 0;
    for (int i = 0; i < g->household_slots; ++i) {
        const Household *h = &g->households[i];
        if (h->active) free_workers += max_int(0, h->adults - h->expedition_adults);
    }
    DrawText(TextFormat("Ouvriers mobilisables : %d", free_workers), (int)x, (int)y, 10, C_MUTED); y += 24.0f;

    draw_panel_title("DEGAGEMENT D'OBSTACLES", x, y); y += 22.0f;
    int obstacle_tiles_left = Sim_CountObstacleTiles(g);
    int obstacles_cleared_tiles = max_int(0, g->obstacles_initial - obstacle_tiles_left);
    DrawText(TextFormat("Degages : %d / %d cases", obstacles_cleared_tiles, g->obstacles_initial), (int)x, (int)y, 10, C_MUTED); y += 17.0f;
    if (g->clearing_active) {
        DrawText(TextFormat("Equipe au travail : %d ouvriers", g->clearing_workers), (int)x, (int)y, 10, C_TEAL); y += 15.0f;
        DrawText(TextFormat("%d case(s), %d jour(s) restants", g->clearing_tile_count, g->clearing_days_remaining), (int)x, (int)y, 10, C_GOLD); y += 24.0f;
    } else if (obstacle_tiles_left <= 0) {
        DrawText("Tous les obstacles reperes ont ete degages.", (int)x, (int)y, 10, C_GREEN); y += 24.0f;
    } else {
        bool clicked = draw_button((Rectangle){x, y, panel.width - 24.0f, 28.0f}, ui->clearing_select_mode ? "ANNULER LE REPERAGE" : "REPERER UN OBSTACLE", ui->clearing_select_mode, true, 10);
        if (clicked) { ui->clearing_select_mode = !ui->clearing_select_mode; if (ui->clearing_select_mode) { ui->demolish_mode = false; ui->selected_building_type = -1; } }
        y += 32.0f;
        DrawText(ui->clearing_select_mode ? "Cliquez un rocher ou une faille sur la carte." : "Il reste des rochers et failles a degager.", (int)x, (int)y, 9, C_MUTED); y += 20.0f;
    }

    draw_panel_title("ETAT DE L'EXPEDITION", x, y); y += 25.0f;
    if (g->expedition_active) {
        DrawText("EQUIPE EN MER", (int)x, (int)y, 12, C_TEAL); y += 22.0f;
        DrawText(TextFormat("Equipe : %d ouvriers", g->expedition_crew), (int)x, (int)y, 10, C_TEXT); y += 17.0f;
        DrawText(TextFormat("Retour dans : %d jours", g->expedition_days_remaining), (int)x, (int)y, 10, C_GOLD); y += 17.0f;
        DrawText(TextFormat("Risque estime : %d%%", g->expedition_risk), (int)x, (int)y, 10, g->expedition_risk > 20 ? C_RED : C_MUTED); y += 23.0f;
        DrawText("L'equipe peut rapporter 30 a 104", (int)x, (int)y, 10, C_MUTED); y += 15.0f;
        DrawText("materiaux. Un incident reste possible.", (int)x, (int)y, 10, C_MUTED);
    } else {
        DrawText("Aucune equipe actuellement en mer.", (int)x, (int)y, 10, C_MUTED); y += 25.0f;
        draw_panel_title("CHOISIR LA TAILLE DE L'EQUIPE", x, y); y += 24.0f;
        const int crews[] = {2, 4, 6, 8};
        for (int i = 0; i < 4; ++i) {
            int crew = crews[i];
            int cost = 450 + crew * 115;
            Rectangle r = {x, y, panel.width - 24.0f, 34.0f};
            bool can = access > 0 && free_workers >= crew && g->money >= cost;
            bool clicked = draw_button(r, TextFormat("%d ouvriers  |  equipement : $%d", crew, cost), false, can, 10);
            if (clicked) {
                char reason[160];
                if (Sim_SendOceanExpedition(g, crew, reason, (int)sizeof(reason))) UI_ShowToast(ui, reason);
                else UI_ShowToast(ui, reason);
            }
            y += 41.0f;
        }
        if (access <= 0) {
            DrawText("Construisez un sas oceanique operationnel", (int)x, (int)(y + 1.0f), 10, C_MUTED); y += 14.0f;
            DrawText("ou debloquez une bathysphere.", (int)x, (int)(y + 1.0f), 10, C_MUTED);
        } else if (free_workers < 2) {
            DrawText("Pas assez d'adultes disponibles dans la ville.", (int)x, (int)(y + 1.0f), 10, C_RED);
        }
    }
    y = panel.y + panel.height - 85.0f;
    draw_panel_title("CONSEIL DE RYAN", x, y); y += 23.0f;
    DrawText("Une securite renforcee et la", (int)x, (int)y, 10, C_MUTED); y += 15.0f;
    DrawText("recherche sur les sas reduisent le risque.", (int)x, (int)y, 10, C_MUTED);
}

static void draw_households_panel(GameUI *ui, GameState *g, Rectangle panel)
{
    float x = panel.x + 12, y = 123;
    draw_panel_title("FOYERS ET CONDITIONS DE VIE", x, y); y += 25;
    DrawText(TextFormat("Habitants : %d  |  Foyers : %d", g->population, g->households_count), (int)x, (int)y, 10, C_MUTED); y += 18;
    int cultural = Sim_CountBuildings(g, BT_LEISURE) + Sim_CountBuildings(g, BT_CINEMA);
    int food_service = Sim_CountBuildings(g, BT_RESTAURANT) + Sim_CountBuildings(g, BT_FOOD) + Sim_CountBuildings(g, BT_FISHERY);
    int shopping = Sim_CountBuildings(g, BT_MARKET) + Sim_CountBuildings(g, BT_BOUTIQUE);
    DrawText(TextFormat("Envies : culture %s  |  repas %s", cultural ? "OK" : "MANQUE", food_service ? "OK" : "MANQUE"),
             (int)x, (int)y, 10, (cultural && food_service) ? C_TEAL : C_GOLD); y += 16;
    DrawText(TextFormat("Commerce : %s  |  Sante : %s", shopping ? "OK" : "MANQUE", Sim_CountBuildings(g, BT_HOSPITAL) ? "OK" : "MANQUE"),
             (int)x, (int)y, 10, (shopping && Sim_CountBuildings(g, BT_HOSPITAL)) ? C_TEAL : C_GOLD); y += 18;
    int rows = max_int(3, (int)((panel.y + panel.height - y - 145) / 66.0f));
    ui->right_scroll = clamp_int(ui->right_scroll, 0, max_int(0, g->households_count - rows));
    Rectangle list = {panel.x + 7, y, panel.width - 14, (float)(rows * 66)};
    BeginScissorMode((int)list.x, (int)list.y, (int)list.width, (int)list.height);
    int visible = 0, skipped = 0;
    for (int hi = 0; hi < g->household_slots; ++hi) {
        Household *h = &g->households[hi];
        if (!h->active) continue;
        if (skipped++ < ui->right_scroll) continue;
        if (visible >= rows) break;
        Rectangle r = {list.x + 2, y + (float)visible * 66.0f + 2, list.width - 4, 61};
        bool hover = point_in(GetMousePosition(), r);
        bool selected = ui->selected_household_index == hi;
        DrawRectangleRounded(r, 0.08f, 4, selected ? (Color){39, 79, 83, 255} : (hover ? (Color){25, 51, 67, 255} : (Color){15, 36, 52, 255}));
        DrawRectangle((int)r.x + 6, (int)r.y + 7, 5, 47, h->happiness < 35 ? C_RED : (h->happiness > 70 ? C_GREEN : C_GOLD));
        DrawText(TextFormat("Foyer #%03d  |  %d adultes, %d enfant(s)", hi + 1, h->adults, h->children), (int)r.x + 16, (int)r.y + 5, 10, C_TEXT);
        DrawText(TextFormat("Revenu $%d  |  Emplois %d/%d", h->monthly_income, h->employed_adults, h->adults), (int)r.x + 16, (int)r.y + 22, 10, C_MUTED);
        DrawText(TextFormat("Bonheur %d%%  |  Social %d%%", h->happiness, h->social_satisfaction), (int)r.x + 16, (int)r.y + 38, 10, h->social_satisfaction < 50 ? C_GOLD : C_TEAL);
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            ui->selected_household_index = hi;
            ui->selected_building_type = -1;
            ui->demolish_mode = false;
            ui->clearing_select_mode = false;
            int home = h->home_building - 1;
            ui->selected_building_index = (home >= 0 && home < g->building_slots && g->buildings[home].active) ? home : -1;
        }
        visible++;
    }
    EndScissorMode();
    draw_scrollbar(panel.x + panel.width - 10.0f, list.y, 3.0f, list.height, g->households_count, rows, ui->right_scroll);
    if (g->households_count == 0) DrawText("Aucun foyer enregistre.", (int)x, (int)y, 11, C_MUTED);

    int hi = ui->selected_household_index;
    if (hi >= 0 && hi < g->household_slots && g->households[hi].active) {
        Household *h = &g->households[hi];
        float bottom = panel.y + panel.height - 107;
        draw_panel_title("DETAIL DU FOYER", x, bottom); bottom += 22;
        int people = h->adults + h->children;
        int bill = people * (48 + g->electricity_price + g->water_price) + 30 + g->housing_tax;
        int tax = h->monthly_income * g->tax_rate / 100;
        int sales_tax = bill * 18 / 100;
        DrawText(TextFormat("Charges + taxe de vente : $%d/mois", bill + sales_tax), (int)x, (int)bottom, 10, C_TEXT); bottom += 15;
        DrawText(TextFormat("Impot sur revenus : $%d/mois", tax), (int)x, (int)bottom, 10, C_MUTED); bottom += 15;
        DrawText(TextFormat("Besoins vitaux : %s", h->basic_needs_met ? "couverts" : "menaces"), (int)x, (int)bottom, 10, h->basic_needs_met ? C_GREEN : C_RED); bottom += 15;
        const char *want = h->social_satisfaction < 50 ? "Envie : plus de culture, de repas et de commerces." :
                           (h->employed_adults < h->adults ? "Envie : emploi stable et meilleur revenu." :
                           (h->savings < 250 ? "Envie : charges moins lourdes et economies." : "Envie : conserver le confort et les services."));
        draw_wrapped_text(GetFontDefault(), want,
                          (Rectangle){x, bottom, panel.width - 24.0f, panel.y + panel.height - bottom - 4.0f},
                          10.0f, 1.0f, C_GOLD);
    }
    if (point_in(GetMousePosition(), list)) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) ui->right_scroll = clamp_int(ui->right_scroll - (wheel > 0 ? 1 : -1), 0, max_int(0, g->households_count - rows));
    }
}

static void draw_right_panel(GameUI *ui, GameState *g)
{
    Rectangle panel = get_layout().right_panel;
    DrawRectangleRec(panel, C_PANEL);
    DrawLine((int)panel.x, (int)panel.y, (int)panel.x, (int)(panel.y + panel.height), C_EDGE);
    const char *tabs[] = {"VILLE", "SCI.", "ENT.", "FOY.", "OCEAN"};
    const float widths[] = {48.0f, 43.0f, 45.0f, 43.0f, 51.0f};
    float tx = panel.x + 7;
    for (int i = 0; i < 5; ++i) {
        float w = widths[i];
        Rectangle r = {tx, 76, w, 31};
        draw_button(r, tabs[i], ui->right_tab == i, true, 9);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && point_in(GetMousePosition(), r)) {
            ui->right_tab = i;
            ui->right_scroll = 0;
        }
        tx += w + 3;
    }
    if (ui->right_tab == 0) draw_overview_panel(ui, g, panel);
    else if (ui->right_tab == 1) draw_research_panel(ui, g, panel);
    else if (ui->right_tab == 2) draw_companies_panel(ui, g, panel);
    else if (ui->right_tab == 3) draw_households_panel(ui, g, panel);
    else draw_ocean_panel(ui, g, panel);
}

static Rectangle toast_rectangle(const GameUI *ui)
{
    int sw = GetScreenWidth();
    bool has_detail = ui->toast_detail[0] != '\0';
    int measured = max_int(MeasureText(ui->toast, 13), has_detail ? MeasureText(ui->toast_detail, 11) : 0);
    int width = min_int(max_int(260, sw - 40), max_int(has_detail ? 480 : 320, measured + 34));
    float height = has_detail ? 90.0f : 48.0f;
    return (Rectangle){(float)(sw / 2 - width / 2), 74.0f, (float)width, height};
}

static void draw_footer(GameUI *ui, GameState *g, float dt)
{
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    DrawRectangle(0, sh - FOOT_H, sw, FOOT_H, (Color){7, 17, 29, 255});
    DrawLine(0, sh - FOOT_H, sw, sh - FOOT_H, C_EDGE);
    const char *status = g->log_count > 0 ? g->logs[0].text : "Rapture attend vos ordres.";
    DrawCircle(15, sh - 20, 4, g->log_count > 0 && g->logs[0].severity == 2 ? C_RED : C_TEAL);
    DrawText(status, 27, sh - 27, 10, C_MUTED);
    if (ui->selected_building_type >= 0) {
        int fw = 1, fh = 1;
        Sim_GetDefaultFootprint((BuildingType)ui->selected_building_type, &fw, &fh);
        if (fw != fh) DrawText(TextFormat("Construction : %s  |  R : rotation %s", BUILDING_DEFS[ui->selected_building_type].name,
                 ui->rotate_building ? "verticale" : "horizontale"), 470, sh - 27, 10, C_GOLD);
        else DrawText(TextFormat("Construction : %s", BUILDING_DEFS[ui->selected_building_type].name), 470, sh - 27, 10, C_GOLD);
    } else if (ui->demolish_mode) {
        DrawText("DEMOLITION ACTIVE", 470, sh - 27, 10, C_RED);
    }
    if (g->paused) DrawText("PAUSE", sw - 74, sh - 27, 10, C_GOLD);
    else DrawText(TextFormat("VITESSE %dx", g->speed), sw - 95, sh - 27, 10, C_TEAL);

    if (ui->toast_time > 0.0f) {
        ui->toast_time -= dt;
        bool has_detail = ui->toast_detail[0] != '\0';
        Rectangle r = toast_rectangle(ui);
        DrawRectangleRounded(r, 0.12f, 5, (Color){16, 43, 55, 246});
        DrawRectangleRoundedLines(r, 0.12f, 5, has_detail ? C_GOLD : C_TEAL);
        draw_wrapped_text(GetFontDefault(), ui->toast,
                          (Rectangle){r.x + 13.0f, r.y + 7.0f, r.width - 26.0f, has_detail ? 32.0f : 36.0f},
                          13.0f, 1.0f, C_TEXT);
        if (has_detail) {
            DrawLine((int)r.x + 12, (int)r.y + 42, (int)(r.x + r.width - 12), (int)r.y + 42, (Color){71, 111, 120, 255});
            draw_wrapped_text(GetFontDefault(), ui->toast_detail,
                              (Rectangle){r.x + 13.0f, r.y + 47.0f, r.width - 26.0f, 38.0f},
                              11.0f, 1.0f, (Color){197, 218, 216, 255});
        }
    }
}

static void finish_tutorial(GameUI *ui, GameState *g)
{
    ui->show_tutorial = false;
    ui->tutorial_page = 0;
    g->paused = false;
    UI_ShowToast(ui, "Tutoriel termine. Rapture attend vos decisions, Monsieur Ryan.");
}

static void draw_tutorial_overlay(GameUI *ui, GameState *g)
{
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, (Color){2, 8, 14, 218});
    Rectangle card = {(float)(sw / 2 - 310), (float)(sh / 2 - 218), 620.0f, 436.0f};
    DrawRectangleRounded(card, 0.035f, 8, (Color){8, 24, 39, 252});
    DrawRectangleRoundedLines(card, 0.035f, 8, (Color){121, 162, 157, 255});
    DrawText("RAPTURE : GUIDE DU NOUVEAU DIRECTEUR", (int)card.x + 28, (int)card.y + 25, 18, C_GOLD);
    DrawText(TextFormat("ETAPE %d / 3", ui->tutorial_page + 1), (int)(card.x + card.width - 100), (int)card.y + 55, 10, C_TEAL);
    DrawLine((int)card.x + 27, (int)card.y + 59, (int)(card.x + card.width - 27), (int)card.y + 59, C_EDGE);

    if (ui->tutorial_page == 0) {
        DrawText("1. LIRE LA VILLE", (int)card.x + 29, (int)card.y + 83, 15, C_TEAL);
        DrawText("La carte au centre est une grille isometrique.", (int)card.x + 29, (int)card.y + 119, 12, C_TEXT);
        DrawText("Maintenez le clic droit et deplacez la souris pour", (int)card.x + 29, (int)card.y + 143, 11, C_MUTED);
        DrawText("faire glisser la camera. La molette sert au zoom ;", (int)card.x + 29, (int)card.y + 163, 11, C_MUTED);
        DrawText("WASD ou les fleches deplacent aussi la vue.", (int)card.x + 29, (int)card.y + 183, 11, C_MUTED);
        DrawText("En haut : tresorerie, population, satisfaction et date.", (int)card.x + 29, (int)card.y + 220, 11, C_TEXT);
        DrawText("A gauche : choix des batiments ; a droite : statistiques,", (int)card.x + 29, (int)card.y + 244, 11, C_TEXT);
        DrawText("tarifs, technologies, entreprises, foyers et expeditions.", (int)card.x + 29, (int)card.y + 264, 11, C_TEXT);
    } else if (ui->tutorial_page == 1) {
        DrawText("2. CONSTRUIRE ET RELIER", (int)card.x + 29, (int)card.y + 83, 15, C_TEAL);
        DrawText("Choisissez un ouvrage dans la liste a gauche, puis", (int)card.x + 29, (int)card.y + 119, 11, C_TEXT);
        DrawText("cliquez sur une case libre proche de la ville.", (int)card.x + 29, (int)card.y + 139, 11, C_TEXT);
        DrawText("Les corridors donnent forme aux quartiers. Les petits", (int)card.x + 29, (int)card.y + 171, 11, C_MUTED);
        DrawText("tuyaux en cuivre ameliorent les installations vitales raccordees.", (int)card.x + 29, (int)card.y + 191, 11, C_MUTED);
        DrawText("Chaque construction coute de l'argent et parfois des", (int)card.x + 29, (int)card.y + 223, 11, C_TEXT);
        DrawText("materiaux. La recherche debloque des infrastructures.", (int)card.x + 29, (int)card.y + 243, 11, C_TEXT);
        DrawText("Echap annule l'outil ; X ou DEMOLIR retire un ouvrage.", (int)card.x + 29, (int)card.y + 275, 11, C_MUTED);
    } else {
        DrawText("3. GOUVERNER ET PROSPERER", (int)card.x + 29, (int)card.y + 83, 15, C_TEAL);
        DrawText("L'onglet VILLE liste vos batiments et leurs quantites.", (int)card.x + 29, (int)card.y + 119, 11, C_TEXT);
        DrawText("Les tarifs et impots influencent les recettes publiques", (int)card.x + 29, (int)card.y + 139, 11, C_MUTED);
        DrawText("mais aussi le budget et le bonheur des familles.", (int)card.x + 29, (int)card.y + 159, 11, C_MUTED);
        DrawText("Les entreprises embauchent ; les habitants gagnent des", (int)card.x + 29, (int)card.y + 191, 11, C_TEXT);
        DrawText("revenus. Recherche, ressources, emploi et services vitaux", (int)card.x + 29, (int)card.y + 211, 11, C_TEXT);
        DrawText("doivent evoluer ensemble. Les expeditions demandent un", (int)card.x + 29, (int)card.y + 231, 11, C_MUTED);
        DrawText("sas ou une bathysphere et rapportent des materiaux.", (int)card.x + 29, (int)card.y + 251, 11, C_MUTED);
        DrawText("Espace = pause ; 1/2/3 = vitesse ; F5/F9 = emplacements.", (int)card.x + 29, (int)card.y + 283, 10, C_GOLD);
    }

    float button_y = card.y + card.height - 56.0f;
    Rectangle skip = {card.x + 28.0f, button_y, 126.0f, 34.0f};
    Rectangle prev = {card.x + card.width - 272.0f, button_y, 112.0f, 34.0f};
    Rectangle next = {card.x + card.width - 150.0f, button_y, 122.0f, 34.0f};
    if (draw_button(skip, "PASSER", false, true, 10)) finish_tutorial(ui, g);
    if (ui->tutorial_page > 0 && draw_button(prev, "PRECEDENT", false, true, 9)) ui->tutorial_page--;
    if (draw_button(next, ui->tutorial_page == 2 ? "COMMENCER" : "SUIVANT", true, true, 10)) {
        if (ui->tutorial_page < 2) ui->tutorial_page++;
        else finish_tutorial(ui, g);
    }
}

static void draw_title(GameUI *ui, GameState *g)
{
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    draw_map(g, ui, (Rectangle){0, 0, (float)sw, (float)sh});
    DrawRectangle(0, 0, sw, sh, (Color){1, 8, 16, 105});
    Rectangle card = {(float)(sw / 2 - 250), (float)(sh / 2 - 205), 500, 410};
    DrawRectangleRounded(card, 0.035f, 8, (Color){8, 24, 39, 242});
    DrawRectangleRoundedLines(card, 0.035f, 8, (Color){107, 157, 160, 255});
    DrawText("RAPTURE", sw / 2 - 150, (int)card.y + 35, 48, C_GOLD);
    DrawText("DREAM OF THE DEEP", sw / 2 - 132, (int)card.y + 86, 18, C_TEAL);
    DrawLine(sw / 2 - 190, (int)card.y + 122, sw / 2 + 190, (int)card.y + 122, C_EDGE);
    DrawText("Construisez la ville qui devait liberer l'humanite.", sw / 2 - 198, (int)card.y + 140, 12, C_TEXT);
    DrawText("Dirigez l'economie. Protegez ses habitants.", sw / 2 - 174, (int)card.y + 160, 12, C_MUTED);

    Rectangle campaign = {(float)(sw / 2 - 172), (float)card.y + 211, 344, 40};
    Rectangle sandbox = {(float)(sw / 2 - 172), (float)card.y + 260, 344, 40};
    Rectangle load = {(float)(sw / 2 - 172), (float)card.y + 309, 344, 32};
    draw_button(campaign, "NOUVELLE CAMPAGNE", false, true, 13);
    draw_button(sandbox, "BAC A SABLE", false, true, 13);
    draw_button(load, "CHARGER UNE PARTIE", false, true, 11);
    DrawText("C17  |  VUE ISOMETRIQUE  |  SIMULATION ECONOMIQUE", sw / 2 - 190, sh - 35, 10, C_MUTED);
    Vector2 m = GetMousePosition();
    if (!ui->save_menu_open && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (point_in(m, campaign)) {
            Sim_Init(g, MODE_CAMPAIGN); g->paused = true; ui->screen = SCREEN_PLAYING; ui->camera_x = 0; ui->camera_y = 0; ui->zoom = 0.95f;
            ui->selected_building_type = -1; ui->selected_building_index = -1; ui->selected_household_index = -1; ui->right_tab = 0; ui->build_scroll = 0; ui->right_scroll = 0; ui->inventory_scroll = 0; ui->sim_accumulator = 0;
            ui->show_tutorial = true; ui->tutorial_page = 0;
        } else if (point_in(m, sandbox)) {
            Sim_Init(g, MODE_SANDBOX); ui->screen = SCREEN_PLAYING; ui->camera_x = 0; ui->camera_y = 0; ui->zoom = 0.95f;
            ui->selected_building_type = -1; ui->selected_building_index = -1; ui->selected_household_index = -1; ui->right_tab = 0; ui->build_scroll = 0; ui->right_scroll = 0; ui->inventory_scroll = 0; ui->sim_accumulator = 0; ui->show_tutorial = false;
            UI_ShowToast(ui, "Bac a sable : construisez votre cite librement.");
        } else if (point_in(m, load)) {
            open_save_menu(ui, g, true, true, false);
        }
    }
    if (!ui->save_menu_open && (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_ENTER))) {
        Sim_Init(g, MODE_CAMPAIGN); g->paused = true; ui->screen = SCREEN_PLAYING; ui->selected_building_type = -1; ui->selected_building_index = -1; ui->selected_household_index = -1; ui->camera_x = 0; ui->camera_y = 0; ui->zoom = 0.95f; ui->show_tutorial = true; ui->tutorial_page = 0; ui->right_tab = 0; ui->inventory_scroll = 0; ui->sim_accumulator = 0;
    } else if (!ui->save_menu_open && IsKeyPressed(KEY_TWO)) {
        Sim_Init(g, MODE_SANDBOX); ui->screen = SCREEN_PLAYING; ui->selected_building_type = -1; ui->selected_building_index = -1; ui->selected_household_index = -1; ui->camera_x = 0; ui->camera_y = 0; ui->zoom = 0.95f; ui->show_tutorial = false; ui->right_tab = 0; ui->inventory_scroll = 0; ui->sim_accumulator = 0;
    }
}

void UI_Init(GameUI *ui)
{
    memset(ui, 0, sizeof(*ui));
    ui->screen = SCREEN_TITLE;
    ui->selected_building_type = -1;
    ui->selected_building_index = -1;
    ui->selected_household_index = -1;
    ui->zoom = 0.95f;
    ui->toast_building_index = -1;
    ui->show_tutorial = false;
    ui->dragging_camera = false;
    ui->mouse_dragged = false;
}

void UI_ShowToast(GameUI *ui, const char *message)
{
    if (!ui || !message) return;
    (void)snprintf(ui->toast, sizeof(ui->toast), "%s", message);
    ui->toast_detail[0] = '\0';
    ui->toast_time = 3.5f;
    ui->toast_building_index = -1;
}

static void get_save_slot_path(int slot, char *path, size_t path_size)
{
    if (slot <= 0) (void)snprintf(path, path_size, "%s", SAVE_PATH); /* preserve the legacy save */
    else (void)snprintf(path, path_size, "rapture_save_%d.dat", slot + 1);
}

static bool save_slot_exists(int slot)
{
    char path[64];
    get_save_slot_path(slot, path, sizeof(path));
    FILE *fp = fopen(path, "r");
    if (!fp) return false;
    fclose(fp);
    return true;
}

static void open_save_menu(GameUI *ui, GameState *g, bool is_load, bool from_title, bool from_pause)
{
    if (!ui || !g) return;
    ui->save_menu_open = true;
    ui->save_menu_is_load = is_load;
    ui->save_menu_return_to_title = from_title;
    ui->save_menu_return_to_pause = from_pause;
    ui->paused_before_save_menu = g->paused;
    g->paused = true;
}

static void finish_save_menu(GameUI *ui, GameState *g)
{
    bool return_to_pause = ui->save_menu_return_to_pause;
    ui->save_menu_open = false;
    ui->save_menu_return_to_title = false;
    ui->save_menu_return_to_pause = false;
    if (return_to_pause) {
        ui->show_pause_menu = true;
        g->paused = true;
    } else {
        g->paused = ui->paused_before_save_menu;
    }
}

static void draw_save_load_modal(GameUI *ui, GameState *g)
{
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, (Color){2, 8, 14, 210});
    Rectangle card = {(float)(sw / 2 - 245), (float)(sh / 2 - 225), 490.0f, 450.0f};
    DrawRectangleRounded(card, 0.045f, 7, (Color){8, 24, 39, 252});
    DrawRectangleRoundedLines(card, 0.045f, 7, C_EDGE);
    DrawText(ui->save_menu_is_load ? "CHARGER UNE PARTIE" : "SAUVEGARDER UNE PARTIE",
             (int)card.x + 28, (int)card.y + 23, 20, C_GOLD);
    DrawText(ui->save_menu_is_load ? "Choisissez une sauvegarde a reprendre." : "Choisissez un emplacement de sauvegarde.",
             (int)card.x + 28, (int)card.y + 54, 10, C_MUTED);

    for (int i = 0; i < SAVE_SLOT_COUNT; ++i) {
        bool exists = save_slot_exists(i);
        char label[96];
        if (ui->save_menu_is_load)
            (void)snprintf(label, sizeof(label), "PARTIE %d  |  %s", i + 1, exists ? "Sauvegarde disponible" : "Emplacement vide");
        else
            (void)snprintf(label, sizeof(label), "PARTIE %d  |  %s", i + 1, exists ? "Remplacer la sauvegarde" : "Nouvel emplacement");
        Rectangle row = {card.x + 28.0f, card.y + 83.0f + (float)i * 51.0f, card.width - 56.0f, 42.0f};
        bool clicked = draw_button(row, label, false, !ui->save_menu_is_load || exists, 10);
        if (!clicked) continue;
        char path[64];
        get_save_slot_path(i, path, sizeof(path));
        if (ui->save_menu_is_load) {
            if (Sim_Load(g, path)) {
                ui->screen = SCREEN_PLAYING;
                ui->show_pause_menu = false;
                ui->show_tutorial = false;
                ui->selected_building_type = -1;
                ui->selected_building_index = -1;
                ui->selected_household_index = -1;
                ui->demolish_mode = false;
            ui->clearing_select_mode = false;
                ui->rotate_building = false;
                ui->camera_x = 0.0f; ui->camera_y = 0.0f; ui->zoom = 0.95f;
                ui->right_tab = 0; ui->sim_accumulator = 0.0f; ui->last_notification_serial = 0u;
                ui->save_menu_open = false;
                ui->save_menu_return_to_title = false;
                ui->save_menu_return_to_pause = false;
                UI_ShowToast(ui, TextFormat("Partie %d chargee.", i + 1));
            } else {
                UI_ShowToast(ui, TextFormat("Impossible de charger la partie %d.", i + 1));
            }
        } else {
            if (Sim_Save(g, path)) {
                finish_save_menu(ui, g);
                UI_ShowToast(ui, TextFormat("Partie sauvegardee dans l'emplacement %d.", i + 1));
            } else {
                UI_ShowToast(ui, TextFormat("Echec de sauvegarde dans l'emplacement %d.", i + 1));
            }
        }
        return;
    }
    if (ui->toast_time > 0.0f && ui->toast[0] != '\0')
        DrawText(ui->toast, (int)card.x + 28, (int)card.y + 334, 10, C_GOLD);
    Rectangle cancel = {card.x + 140.0f, card.y + 355.0f, card.width - 280.0f, 38.0f};
    if (draw_button(cancel, "ANNULER", false, true, 11)) {
        finish_save_menu(ui, g);
        UI_ShowToast(ui, "Operation annulee.");
    }
}

static void draw_pause_menu(GameUI *ui, GameState *g)
{
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, (Color){2, 8, 14, 205});
    Rectangle card = {(float)(sw / 2 - 175), (float)(sh / 2 - 150), 350.0f, 300.0f};
    DrawRectangleRounded(card, 0.05f, 7, (Color){8, 24, 39, 252});
    DrawRectangleRoundedLines(card, 0.05f, 7, C_EDGE);
    DrawText("RAPTURE | MENU", (int)card.x + 28, (int)card.y + 24, 20, C_GOLD);
    DrawText("La ville est en pause.", (int)card.x + 28, (int)card.y + 56, 11, C_MUTED);
    Rectangle resume = {card.x + 28, card.y + 88, card.width - 56, 40};
    Rectangle save = {card.x + 28, card.y + 139, card.width - 56, 40};
    Rectangle title = {card.x + 28, card.y + 190, card.width - 56, 40};
    if (draw_button(resume, "REPRENDRE", true, true, 12)) {
        ui->show_pause_menu = false;
        g->paused = false;
    }
    if (draw_button(save, "SAUVEGARDER", false, true, 11)) {
        ui->show_pause_menu = false;
        open_save_menu(ui, g, false, false, true);
    }
    if (draw_button(title, "RETOUR AU MENU PRINCIPAL", false, true, 10)) {
        ui->show_pause_menu = false;
        ui->screen = SCREEN_TITLE;
        ui->selected_building_type = -1;
        ui->selected_building_index = -1;
        ui->selected_household_index = -1;
        ui->demolish_mode = false;
            ui->clearing_select_mode = false;
        g->paused = true;
        UI_ShowToast(ui, "Retour au menu principal.");
    }
}

static void get_notification_impact(const GameState *g, const char *message,
                                    char *impact, size_t impact_size)
{
    if (!impact || impact_size == 0) return;
    impact[0] = '\0';
    if (!message) return;

    if (strstr(message, "Expedition oceanique rentree") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : les materiaux annonces dans le titre sont ajoutes au stock et l'equipage revient travailler.");
    } else if (strstr(message, "Expedition endommagee") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : aucun materiau rapporte ; le sauvetage coute 90 $ par membre d'equipage.");
    } else if (strstr(message, "Chantier termine") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : le batiment est operationnel ; ses emplois, sa production et son entretien commencent.");
    } else if (strstr(message, "Recherche terminee") != NULL) {
        bool described = false;
        if (g) {
            for (int i = 0; i < TECH_COUNT; ++i) {
                if (strstr(message, TECH_NAMES[i]) != NULL) {
                    (void)snprintf(impact, impact_size, "Effet concret : %s", TECH_DESCRIPTIONS[i]);
                    described = true;
                    break;
                }
            }
        }
        if (!described) (void)snprintf(impact, impact_size,
            "Impact ville : une technologie est debloquee ; consultez l'onglet Science pour ses prerequis et effets.");
    } else if (strstr(message, "Objectif de campagne atteint") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : prime de developpement creditee (4 500 $ + 900 $ par etape de campagne atteinte).");
    } else if (strstr(message, "Victoire") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : objectif final atteint ; la campagne est terminee.");
    } else if (strstr(message, "crise a depasse le point de rupture") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : la campagne est perdue ; le bonheur ou la tresorerie a franchi le seuil critique.");
    } else if (strstr(message, "Incident de corridor") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : un corridor perd environ 10 a 17 points d'etat ; reparez-le avant une rupture.");
    } else if (strstr(message, "Incident technique") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : depense municipale immediate de 450 $ pour la maintenance.");
    } else if (strstr(message, "Debut de revolte ouvriere") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : moral des foyers en baisse de 2 a 5 points et perte de 350 a 849 $.");
    } else if (strstr(message, "assemblee syndicale") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : pas de degats immediats ; surveillez le moral, l'emploi et les salaires.");
    } else if (strstr(message, "investisseur") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : la tresorerie municipale augmente de 1 500 a 3 300 $.");
    } else if (strstr(message, "caisse de materiaux") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : le stock de materiaux augmente de 8 a 24 unites.");
    } else if (strstr(message, "Surtension electrique") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : perte immediate de 650 $ et une centrale perd 8 points d'etat.");
    } else if (strstr(message, "festival local") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : le bonheur de chaque foyer eligible augmente de 2 points.");
    } else if (strstr(message, "Faillite") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : l'entreprise ferme ; ses emplois et sa production sont perdus jusqu'a son remplacement.");
    } else if (strstr(message, "nouveaux residents") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : la population augmente ; les besoins en logements, eau, oxygene et nourriture suivent.");
    } else if (strstr(message, "Hephaistos manque de materiaux") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : la production electrique du Pole d'Hephaistos est reduite jusqu'au reapprovisionnement.");
    } else if (strstr(message, "reseau electrique est en sous-capacite") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : la demande depasse la production ; satisfaction et activite des batiments peuvent baisser.");
    } else if (strstr(message, "reserves d'oxygene") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : oxygene insuffisant ; les foyers perdent du bonheur et les services vitaux sont fragilises.");
    } else if (strstr(message, "distribution d'eau est limitee") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : les foyers perdent du bonheur ; augmentez la production d'eau ou reduisez la demande.");
    } else if (strstr(message, "reserves alimentaires deviennent faibles") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : risque de baisse du bonheur si le stock tombe trop bas ; construisez des fermes marines.");
    } else if (strstr(message, "Tresorerie basse") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : moins de 12 000 $ disponibles ; reduisez les couts ou augmentez les recettes avant de construire.");
    } else if (strstr(message, "Maintenance") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : le batiment approche d'un etat critique ; ouvrez sa fiche et lancez une reparation.");
    } else if (strstr(message, "mecontentement grandit") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : bonheur moyen inferieur a 35%% ; developpez emploi, services et loisirs et surveillez les taxes.");
    } else if (strstr(message, "hausse de la demande de logements") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : besoin croissant de logements et de services ; verifiez les places vacantes et la couverture sociale.");
    } else if (strstr(message, "Partie chargee") != NULL) {
        (void)snprintf(impact, impact_size,
            "Impact ville : la partie sauvegardee est chargee ; verifiez la date, la tresorerie et les besoins dans l'onglet Ville.");
    } else {
        (void)snprintf(impact, impact_size,
            "Impact ville : consultez les indicateurs et le journal de la ville pour suivre les consequences de cet evenement.");
    }
}

static void news_history_push(GameUI *ui, const GameState *g, const char *message, const char *impact,
                              int severity, int day_stamp, int building_index)
{
    memmove(&ui->news_history[1], &ui->news_history[0], sizeof(NewsEntry) * (size_t)(NEWS_HISTORY_MAX - 1));
    NewsEntry *e = &ui->news_history[0];
    (void)snprintf(e->text, sizeof(e->text), "%s", message);
    (void)snprintf(e->impact, sizeof(e->impact), "%s", impact);
    e->severity = severity;
    /* The exact building this event happened to, set by the simulation at the event's own
       origin (see Sim_AddLogFor): never re-derived here from the message text, which would
       match the first building of that type rather than the one actually involved. */
    e->building_index = (g && building_index >= 0 && building_index < g->building_slots &&
                         g->buildings[building_index].active) ? building_index : -1;
    /* day_stamp = year * 360 + month * 30 + day, with month 1-12 and day 1-30. */
    int t = day_stamp - 31;
    if (t < 0) t = 0;
    e->year = t / 360;
    e->month = (t % 360) / 30 + 1;
    e->day = (t % 30) + 1;
    if (ui->news_count < NEWS_HISTORY_MAX) ui->news_count++;
    if (ui->news_unread < NEWS_HISTORY_MAX) ui->news_unread++;
}

static void UI_ShowNewsToast(GameUI *ui, const GameState *g, const char *message, int severity, int day_stamp, int building_index)
{
    if (!ui || !message) return;
    (void)snprintf(ui->toast, sizeof(ui->toast), "%s", message);
    get_notification_impact(g, message, ui->toast_detail, sizeof(ui->toast_detail));
    ui->toast_time = 8.0f; /* more than twice the former 3.5-second duration */
    ui->toast_building_index = (g && building_index >= 0 && building_index < g->building_slots &&
                                g->buildings[building_index].active) ? building_index : -1;
    news_history_push(ui, g, message, ui->toast_detail, severity, day_stamp, building_index);
}

static void open_news_history(GameUI *ui, GameState *g)
{
    ui->show_news = true;
    ui->paused_before_news = g->paused;
    g->paused = true;
    ui->news_unread = 0;
    ui->news_scroll = 0;
    ui->selected_building_type = -1;
    ui->demolish_mode = false;
            ui->clearing_select_mode = false;
}

static void close_news_history(GameUI *ui, GameState *g)
{
    ui->show_news = false;
    g->paused = ui->paused_before_news;
}

static void draw_news_history(GameUI *ui, GameState *g)
{
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, (Color){2, 8, 14, 205});
    float card_w = (float)min_int(680, sw - 40);
    float card_h = (float)min_int(600, sh - 60);
    Rectangle card = {((float)sw - card_w) * 0.5f, ((float)sh - card_h) * 0.5f, card_w, card_h};
    DrawRectangleRounded(card, 0.03f, 7, (Color){8, 24, 39, 252});
    DrawRectangleRoundedLines(card, 0.03f, 7, C_EDGE);
    DrawText("ANNONCES DE RAPTURE", (int)card.x + 24, (int)card.y + 20, 18, C_GOLD);
    DrawText("Historique des evenements et de ce qu'ils apportent a la ville.", (int)card.x + 24, (int)card.y + 46, 10, C_MUTED);
    if (draw_button((Rectangle){card.x + card.width - 98.0f, card.y + 16.0f, 78.0f, 28.0f}, "FERMER", false, true, 10)) {
        close_news_history(ui, g);
        return;
    }
    DrawLine((int)card.x + 16, (int)card.y + 68, (int)(card.x + card.width) - 16, (int)card.y + 68, C_EDGE);

    Rectangle list = {card.x + 12.0f, card.y + 76.0f, card.width - 24.0f, card.height - 90.0f};
    const float row_h = 88.0f;
    if (ui->news_count == 0) {
        DrawText("Aucune annonce pour le moment.", (int)list.x + 12, (int)list.y + 14, 11, C_MUTED);
        DrawText("Les chantiers termines, recherches, incidents et alertes apparaitront ici.", (int)list.x + 12, (int)list.y + 34, 10, C_MUTED);
        return;
    }
    float content_h = (float)ui->news_count * row_h;
    float max_scroll = max_float(0.0f, content_h - list.height);
    if (point_in(GetMousePosition(), list)) ui->news_scroll -= (int)(GetMouseWheelMove() * 44.0f);
    ui->news_scroll = clamp_int(ui->news_scroll, 0, (int)max_scroll);

    BeginScissorMode((int)list.x, (int)list.y, (int)list.width, (int)list.height);
    int clicked_building = -1;
    for (int i = 0; i < ui->news_count; ++i) {
        const NewsEntry *e = &ui->news_history[i];
        float y = list.y + (float)i * row_h - (float)ui->news_scroll;
        if (y + row_h < list.y || y > list.y + list.height) continue;
        Rectangle r = {list.x, y, list.width - 10.0f, row_h - 6.0f};
        bool hover_news = point_in(GetMousePosition(), r);
        DrawRectangleRounded(r, 0.08f, 4, hover_news ? (Color){22, 48, 62, 255} : (Color){13, 33, 49, 255});
        if (hover_news) {
            DrawRectangleLinesEx(r, 1.0f, e->building_index >= 0 ? C_TEAL : C_EDGE);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && e->building_index >= 0 &&
                e->building_index < g->building_slots && g->buildings[e->building_index].active) {
                clicked_building = e->building_index;
                break;
            }
        }
        Color sev = e->severity >= 2 ? C_RED : (e->severity == 1 ? C_GOLD : C_TEAL);
        DrawRectangle((int)r.x, (int)r.y + 6, 4, (int)r.height - 12, sev);
        DrawText(TextFormat("%02d / %02d / %d", e->day, e->month, e->year), (int)r.x + 16, (int)r.y + 8, 10, sev);
        DrawText(e->severity >= 2 ? "ALERTE" : (e->severity == 1 ? "AVERTISSEMENT" : "INFO"),
                 (int)(r.x + r.width - 14.0f) - MeasureText(e->severity >= 2 ? "ALERTE" : (e->severity == 1 ? "AVERTISSEMENT" : "INFO"), 9),
                 (int)r.y + 8, 9, sev);
        draw_wrapped_text(GetFontDefault(), e->text, (Rectangle){r.x + 16.0f, r.y + 23.0f, r.width - 30.0f, 28.0f}, 12.0f, 1.0f, C_TEXT);
        draw_wrapped_text(GetFontDefault(), e->impact, (Rectangle){r.x + 16.0f, r.y + 52.0f, r.width - 30.0f, 28.0f}, 10.0f, 1.0f, C_MUTED);
    }
    EndScissorMode();
    if (clicked_building >= 0) {
        ui->selected_building_index = clicked_building;
        ui->selected_building_type = -1;
        ui->selected_household_index = -1;
        ui->demolish_mode = false;
            ui->clearing_select_mode = false;
        ui->right_tab = 0;
        close_news_history(ui, g);
        UI_ShowToast(ui, TextFormat("Batiment selectionne : %s", BUILDING_DEFS[g->buildings[clicked_building].type].name));
        return;
    }
    if (max_scroll > 0.0f) {
        float track_h = list.height;
        float thumb_h = max_float(24.0f, track_h * list.height / content_h);
        float thumb_y = list.y + (track_h - thumb_h) * ((float)ui->news_scroll / max_scroll);
        DrawRectangle((int)(list.x + list.width - 5.0f), (int)list.y, 3, (int)track_h, (Color){17, 42, 61, 255});
        DrawRectangle((int)(list.x + list.width - 5.0f), (int)thumb_y, 3, (int)thumb_h, C_EDGE);
    }
}

static void process_log_notifications(GameUI *ui, const GameState *g)
{
    uint32_t current = g->notification_serial;
    if (current == ui->last_notification_serial) return;
    /* A new game or loaded save restarts the runtime sequence. */
    if (current < ui->last_notification_serial) {
        ui->last_notification_serial = 0u;
        ui->news_count = 0;
        ui->news_unread = 0;
        ui->news_scroll = 0;
    }
    if (g->log_count > 0) {
        /* Logs are newest-first. Walk oldest-to-newest so the newest important
           message becomes the visible toast if several appeared in one tick. */
        for (int i = g->log_count - 1; i >= 0; --i) {
            const CityLog *entry = &g->logs[i];
            if (entry->serial <= ui->last_notification_serial || entry->serial > current) continue;
            bool important = entry->severity >= 1 ||
                strstr(entry->text, "Chantier termine") != NULL ||
                strstr(entry->text, "Expedition oceanique rentree") != NULL ||
                strstr(entry->text, "Objectif de campagne atteint") != NULL ||
                strstr(entry->text, "Recherche terminee") != NULL ||
                strstr(entry->text, "Victoire") != NULL;
            if (important) UI_ShowNewsToast(ui, g, entry->text, entry->severity, entry->day_stamp, entry->building_index);
        }
    }
    ui->last_notification_serial = current;
}

static void process_map_click(GameUI *ui, GameState *g, Rectangle viewport, Vector2 mouse)
{
    int x, y;
    if (!screen_to_tile(ui, viewport, mouse, &x, &y)) return;
    if (ui->clearing_select_mode) {
        char reason[160];
        bool started = Sim_StartObstacleClearing(g, x, y, reason, (int)sizeof(reason));
        UI_ShowToast(ui, reason);
        if (started) ui->clearing_select_mode = false;
    } else if (ui->demolish_mode) {
        char reason[160];
        if (Sim_Demolish(g, x, y, reason, (int)sizeof(reason))) {
            ui->selected_building_index = -1;
            UI_ShowToast(ui, reason);
        } else UI_ShowToast(ui, reason);
    } else if (ui->selected_building_type >= 0) {
        char reason[160];
        if (Sim_BuildOriented(g, (BuildingType)ui->selected_building_type, x, y, ui->rotate_building, reason, (int)sizeof(reason))) {
            UI_ShowToast(ui, reason);
            ui->selected_building_index = Sim_GetBuildingAt(g, x, y);
        } else UI_ShowToast(ui, reason);
    } else {
        ui->selected_building_index = Sim_GetBuildingAt(g, x, y);
        ui->selected_household_index = -1;
        /* Keep the current information tab selected. This lets the player inspect a
           company/household list, click the city, and return without losing context. */
    }
}

void UI_HandleInput(GameUI *ui, GameState *g)
{
    if (ui->save_menu_open) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            finish_save_menu(ui, g);
            UI_ShowToast(ui, "Operation annulee.");
        }
        return;
    }
    if (ui->screen == SCREEN_TITLE) return;
    process_log_notifications(ui, g);
    if (ui->show_news) {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_N)) close_news_history(ui, g);
        return;
    }
    if (ui->show_pause_menu) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            ui->show_pause_menu = false;
            g->paused = ui->paused_before_menu;
        }
        return;
    }
    if (ui->show_tutorial) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            ui->show_tutorial = false;
            ui->tutorial_page = 0;
            ui->show_pause_menu = true;
            ui->paused_before_menu = false;
            g->paused = true;
        } else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            if (ui->tutorial_page < 2) ui->tutorial_page++;
            else finish_tutorial(ui, g);
        }
        return;
    }

    Layout l = get_layout();
    Vector2 m = GetMousePosition();

    if (ui->toast_time > 0.0f && ui->toast_building_index >= 0 &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && point_in(m, toast_rectangle(ui))) {
        int bi = ui->toast_building_index;
        if (bi < g->building_slots && g->buildings[bi].active) {
            ui->selected_building_index = bi;
            ui->selected_building_type = -1;
            ui->selected_household_index = -1;
            ui->demolish_mode = false;
            ui->clearing_select_mode = false;
            ui->right_tab = 0;
            ui->show_news = false;
        }
        ui->toast_time = 0.0f;
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        if (ui->selected_building_type >= 0 || ui->demolish_mode) {
            ui->selected_building_type = -1;
            ui->demolish_mode = false;
            ui->clearing_select_mode = false;
            ui->rotate_building = false;
            UI_ShowToast(ui, "Outil annule. Echap a de nouveau pour ouvrir le menu.");
        } else if (ui->selected_building_index >= 0 || ui->selected_household_index >= 0) {
            ui->selected_building_index = -1;
            ui->selected_household_index = -1;
            UI_ShowToast(ui, "Selection annulee.");
        } else {
            ui->show_pause_menu = true;
            ui->paused_before_menu = g->paused;
            g->paused = true;
        }
        return;
    }
    if (IsKeyPressed(KEY_R) && ui->selected_building_type >= 0) {
        int fw = 1, fh = 1;
        Sim_GetDefaultFootprint((BuildingType)ui->selected_building_type, &fw, &fh);
        if (fw == fh) UI_ShowToast(ui, "Ce batiment garde la meme emprise en rotation.");
        else {
            ui->rotate_building = !ui->rotate_building;
            UI_ShowToast(ui, TextFormat("Emprise : %s", ui->rotate_building ? "1x2 cases" : "2x1 cases"));
        }
    }
    if (IsKeyPressed(KEY_SPACE)) g->paused = !g->paused;
    if (IsKeyPressed(KEY_ONE)) { g->speed = 1; g->paused = false; }
    if (IsKeyPressed(KEY_TWO)) { g->speed = 2; g->paused = false; }
    if (IsKeyPressed(KEY_THREE)) { g->speed = 3; g->paused = false; }
    if (IsKeyPressed(KEY_G)) g->show_grid = !g->show_grid;
    if (IsKeyPressed(KEY_N)) { open_news_history(ui, g); return; }
    if (IsKeyPressed(KEY_X)) { ui->demolish_mode = !ui->demolish_mode; if (ui->demolish_mode) { ui->selected_building_type = -1; ui->clearing_select_mode = false; } }
    if (IsKeyPressed(KEY_F5)) { open_save_menu(ui, g, false, false, false); return; }
    if (IsKeyPressed(KEY_F9)) { open_save_menu(ui, g, true, false, false); return; }

    float pan_speed = IsKeyDown(KEY_LEFT_SHIFT) ? 520.0f : 300.0f;
    float dt = GetFrameTime();
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) ui->camera_x += pan_speed * dt;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) ui->camera_x -= pan_speed * dt;
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) ui->camera_y += pan_speed * dt;
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) ui->camera_y -= pan_speed * dt;

    if (point_in(m, l.viewport)) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) ui->zoom = clamp_float(ui->zoom + wheel * 0.08f, 0.48f, 1.55f);
    }

    /* Hold the right mouse button to pan; left clicks are reserved for the map. */
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && point_in(m, l.viewport)) {
        ui->dragging_camera = true;
        ui->mouse_dragged = false;
        ui->drag_start_position = m;
        ui->last_mouse_position = m;
    }
    if (ui->dragging_camera && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        float dx_total = m.x - ui->drag_start_position.x;
        float dy_total = m.y - ui->drag_start_position.y;
        if (!ui->mouse_dragged && (dx_total * dx_total + dy_total * dy_total) > 4.0f) ui->mouse_dragged = true;
        if (ui->mouse_dragged) {
            ui->camera_x += m.x - ui->last_mouse_position.x;
            ui->camera_y += m.y - ui->last_mouse_position.y;
        }
        ui->last_mouse_position = m;
    }
    if (ui->dragging_camera && IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
        ui->dragging_camera = false;
        ui->mouse_dragged = false;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && point_in(m, l.viewport)) {
        ui->dragging_camera = false;
        ui->mouse_dragged = false;
        ui->drag_start_position = m;
        ui->last_mouse_position = m;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && point_in(m, l.viewport))
        process_map_click(ui, g, l.viewport, m);
}

void UI_Draw(GameUI *ui, GameState *g, float delta_time)
{
    ClearBackground(C_BG);
    if (ui->screen == SCREEN_TITLE) {
        ui->camera_x = 0; ui->camera_y = 0;
        draw_title(ui, g);
        if (ui->save_menu_open) {
            draw_save_load_modal(ui, g);
            return;
        }
        if (ui->toast_time > 0.0f) {
            ui->toast_time -= delta_time;
            DrawRectangle(GetScreenWidth() / 2 - 200, GetScreenHeight() - 68, 400, 32, (Color){16, 43, 55, 244});
            DrawText(ui->toast, GetScreenWidth() / 2 - 185, GetScreenHeight() - 58, 11, C_TEXT);
        }
        return;
    }

    Layout l = get_layout();
    draw_map(g, ui, l.viewport);
    if (ui->save_menu_open) {
        draw_save_load_modal(ui, g);
        return;
    }
    if (ui->show_tutorial) {
        draw_tutorial_overlay(ui, g);
        return;
    }
    if (ui->show_pause_menu) {
        draw_pause_menu(ui, g);
        return;
    }
    if (ui->show_news) {
        draw_news_history(ui, g);
        return;
    }
    draw_header(ui, g);
    draw_left_panel(ui, g);
    draw_right_panel(ui, g);
    draw_footer(ui, g, delta_time);

    if (g->won || g->lost) {
        int sw = GetScreenWidth();
        Rectangle card = {(float)(sw / 2 - 200), 150, 400, 92};
        DrawRectangleRounded(card, 0.08f, 5, (Color){6, 17, 28, 238});
        DrawRectangleRoundedLines(card, 0.08f, 5, g->won ? C_GREEN : C_RED);
        DrawText(g->won ? "RAPTURE A TRIOMPHE" : "RAPTURE EST EN CRISE", sw / 2 - 150, 169, 17, g->won ? C_GREEN : C_RED);
        DrawText("Ouvrez NEW pour lancer une nouvelle partie.", sw / 2 - 160, 204, 10, C_TEXT);
    }
}
