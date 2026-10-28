#ifndef RAPTURE_RENDER_H
#define RAPTURE_RENDER_H

#include "rapture.h"
#include "raylib.h"

#define NEWS_HISTORY_MAX 64

typedef struct NewsEntry {
    char text[112];
    char impact[200];
    int severity; /* 0 info, 1 warning, 2 alert */
    int day, month, year;
    int building_index; /* runtime-only target for click-to-select news */
} NewsEntry;

typedef struct GameUI {
    ScreenState screen;
    int selected_building_type;
    int selected_building_index;
    int selected_household_index;
    int right_tab; /* 0 city, 1 research, 2 companies */
    int build_scroll;
    int right_scroll;
    int inventory_scroll;
    bool demolish_mode;
    bool rotate_building;
    bool show_pause_menu;
    bool save_menu_open;
    bool save_menu_is_load;
    bool save_menu_return_to_title;
    bool save_menu_return_to_pause;
    bool paused_before_save_menu;
    bool paused_before_menu;
    uint32_t last_notification_serial;
    bool show_tutorial;
    int tutorial_page;
    bool dragging_camera;
    bool mouse_dragged;
    Vector2 last_mouse_position;
    Vector2 drag_start_position;
    float camera_x;
    float camera_y;
    float zoom;
    float sim_accumulator;
    char toast[160];
    char toast_detail[200];
    float toast_time;
    int toast_building_index; /* runtime-only target for clickable event toast */
    NewsEntry news_history[NEWS_HISTORY_MAX]; /* newest first, runtime only */
    int news_count;
    int news_unread;
    int news_scroll;
    bool show_news;
    bool paused_before_news;
    bool clearing_select_mode; /* Ocean tab: next map click on an obstacle sends a clearing crew */
} GameUI;

void UI_Init(GameUI *ui);
void UI_ShowToast(GameUI *ui, const char *message);
void UI_HandleInput(GameUI *ui, GameState *game);
void UI_Draw(GameUI *ui, GameState *game, float delta_time);

#endif
