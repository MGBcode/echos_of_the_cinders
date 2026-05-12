#include "raylib.h"
#include "player.h"
#include "level.h"
#include "enemy.h"

int main(void) {
    // Ativa VSync para evitar que o FPS suba para 2000 e frite o CPU
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    
    InitWindow(800, 600, "Echos of the Cinders");

    Player cavaleiro;
    InitPlayer(&cavaleiro, GetScreenWidth(), GetScreenHeight());

    Level level;
    level_iniciar(&level);

    Boss boss;
    Boss_Init(&boss, (Vector2){ 600, 300 });

    SetTargetFPS(60);

    // Variáveis para detectar mudança de resolução
    int lastScreenWidth = GetScreenWidth();
    int lastScreenHeight = GetScreenHeight();

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        // SÓ atualiza os cálculos do mapa se a janela mudar de tamanho
        if (GetScreenWidth() != lastScreenWidth || GetScreenHeight() != lastScreenHeight) {
            level_atualizar_tile(&level);
            lastScreenWidth = GetScreenWidth();
            lastScreenHeight = GetScreenHeight();
        }

        UpdatePlayer(&cavaleiro, deltaTime, &level);
        Boss_Update(&boss, deltaTime, cavaleiro.pos, cavaleiro.raio, &level);

        BeginDrawing();
            ClearBackground(BLACK);
            level_desenhar(&level);
            DrawPlayer(cavaleiro);
            Boss_Draw(&boss);
            
            // Mostra o FPS e a placa em uso (ajuda a debugar)
            DrawFPS(10, 10);
            DrawText("Alt+Enter para Fullscreen", 10, 40, 20, GRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}