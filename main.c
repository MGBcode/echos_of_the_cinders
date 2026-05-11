#include "raylib.h"
#include "player.h"
#include "level.h"

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    
    int screenWidth = 800;
    int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Echos of the Cinders");

    Player cavaleiro;
    InitPlayer(&cavaleiro, GetScreenWidth(), GetScreenHeight());

    Level level;
    level_iniciar(&level);

    SetTargetFPS(60);

    int lastScreenWidth = GetScreenWidth();
    int lastScreenHeight = GetScreenHeight();

    while (!WindowShouldClose()) {
        if (IsWindowResized() || GetScreenWidth() != lastScreenWidth) {
            float scaleX = (float)GetScreenWidth() / lastScreenWidth;
            float scaleY = (float)GetScreenHeight() / lastScreenHeight;
            
            cavaleiro.pos.x *= scaleX;
            cavaleiro.pos.y *= scaleY;

            level_atualizar_tile(&level);
            
            lastScreenWidth = GetScreenWidth();
            lastScreenHeight = GetScreenHeight();
        }

        float deltaTime = GetFrameTime();
        UpdatePlayer(&cavaleiro, deltaTime, &level);

        BeginDrawing();
            ClearBackground(BLACK);
            
            level_desenhar(&level);
            DrawPlayer(cavaleiro);
            
            DrawText("Aperte ESPACO para Dash", 20, 20, 20, LIGHTGRAY);
            DrawText(TextFormat("FPS: %i", GetFPS()), GetScreenWidth() - 100, 20, 20, GREEN);
            DrawText(TextFormat("Cooldown Dash: %.2f", cavaleiro.cooldownTimeCounter), 20, 50, 20, RED);
            
        EndDrawing();
    }

    CloseWindow();
    return 0;
}