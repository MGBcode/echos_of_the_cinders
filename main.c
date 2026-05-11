#include "raylib.h"
#include "player.h"
#include "level.h"

int main(void) {
    // Ativa VSync para evitar que o FPS suba para 2000 e frite o CPU
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    
    InitWindow(800, 600, "Echos of the Cinders");

    Player cavaleiro;
    InitPlayer(&cavaleiro, GetScreenWidth(), GetScreenHeight());

    Level level;
    level_iniciar(&level);
     SetTargetFPS(60);
    // Variável para detectar mudança de resolução
    int larguraAnterior = GetScreenWidth();

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        // SÓ atualiza os cálculos do mapa se a janela mudar de tamanho
        if (GetScreenWidth() != larguraAnterior) {
            level_atualizar_tile(&level);
            larguraAnterior = GetScreenWidth();
        }

        UpdatePlayer(&cavaleiro, deltaTime, &level);

        BeginDrawing();
            ClearBackground(BLACK);
            level_desenhar(&level);
            DrawPlayer(cavaleiro);
            
            // Mostra o FPS e a placa em uso (ajuda a debugar)
            DrawFPS(10, 10);
            DrawText("Alt+Enter para Fullscreen", 10, 40, 20, GRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}