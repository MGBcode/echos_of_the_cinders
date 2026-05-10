#include "raylib.h"
#include "player.h"
#include "level.h"

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Echos of the Cinders - Jogo Modular");

    Player cavaleiro;
    InitPlayer(&cavaleiro, screenWidth, screenHeight);

    Level level;
    level_iniciar(&level);

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        level_atualizar_tile(&level);

        UpdatePlayer(&cavaleiro, deltaTime, &level);

        BeginDrawing();
            ClearBackground(BLACK);
            level_desenhar(&level);
            DrawPlayer(cavaleiro);
            DrawText("Aperte ESPACO para Dash", 20, 20, 20, LIGHTGRAY);
            DrawText(TextFormat("Cooldown: %.2f", cavaleiro.cooldownTimeCounter), 20, 50, 20, RED);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
