#include "raylib.h"
#include "player.h"

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Echos of the Cinders - Jogo Modular");

    Player cavaleiro;
    
    InitPlayer(&cavaleiro, screenWidth, screenHeight); 

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime(); 

        UpdatePlayer(&cavaleiro, deltaTime);

        BeginDrawing();
            ClearBackground(BLACK);

            DrawPlayer(cavaleiro);
            
            DrawText("Aperte ESPACO para Dash", 20, 20, 20, LIGHTGRAY);
            DrawText(TextFormat("Cooldown: %.2f", cavaleiro.cooldownTimeCounter), 20, 50, 20, RED);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}