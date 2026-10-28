#include "rapture.h"
#include "render.h"

#include "raylib.h"

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1440, 900, "Rapture: Dream of the Deep");
    /* Set this after InitWindow: raylib initializes ESC as its default exit key. */
    SetExitKey(KEY_NULL);
    SetWindowMinSize(1100, 720);
    SetTargetFPS(60);

    GameState game;
    GameUI ui;
    Sim_Init(&game, MODE_CAMPAIGN);
    game.paused = true;
    UI_Init(&ui);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.08f) dt = 0.08f;
        UI_HandleInput(&ui, &game);
        if (ui.screen == SCREEN_PLAYING && !game.paused && game.speed > 0 && !game.won && !game.lost) {
            ui.sim_accumulator += dt * (float)game.speed / 3.0f;
            while (ui.sim_accumulator >= 1.0f) {
                Sim_TickDay(&game);
                ui.sim_accumulator -= 1.0f;
            }
        }

        BeginDrawing();
        UI_Draw(&ui, &game, dt);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
