#include "raylib.h"
#include "player.h"
#include "level.h"
#include "enemy.h"
#include <stdio.h>

// HUD do boss (barra larga no centro inferior)
static void DrawBossHUD(const Boss *boss) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // Barra larga ocupando ~60% da largura da tela
    int barW = (int)(screenW * 0.6f);
    int barH = 20;
    int barX = (screenW - barW) / 2;
    int barY = screenH - 60;

    // Texto centralizado acima da barra
    char txt[64];
    snprintf(txt, sizeof(txt), "BOSS HP: %d/%d", boss->hp, boss->hpMax);
    int textW = MeasureText(txt, 20);
    DrawText(txt, (screenW - textW) / 2, barY - 26, 20, LIGHTGRAY);

    // fundo
    DrawRectangle(barX, barY, barW, barH, (Color){ 40, 40, 40, 220 });
    DrawRectangleLines(barX, barY, barW, barH, (Color){ 110, 110, 110, 255 });

    float ratio = 0.0f;
    if (boss->hpMax > 0) ratio = (float)boss->hp / (float)boss->hpMax;
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    int fillW = (int)(barW * ratio);
    Color fill = (ratio > 0.5f) ? GREEN : (ratio > 0.25f ? ORANGE : RED);
    DrawRectangle(barX + 1, barY + 1, fillW - 2 > 0 ? fillW - 2 : 0, barH - 2, fill);
}

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

    // Anti-multi-hit: aplica dano 1x por ativação da hitbox
    bool lastHitboxActive = false;
    
    // Anti-multi-hit: aplica dano 1x por ativação do ataque do boss
    bool lastBossAttackActive = false;

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        // SÓ atualiza os cálculos do mapa se a janela mudar de tamanho
        if (IsWindowResized() || GetScreenWidth() != lastScreenWidth) {
            float scaleX = (float)GetScreenWidth() / lastScreenWidth;
            float scaleY = (float)GetScreenHeight() / lastScreenHeight;

            // Ajusta a posição do jogador proporcionalmente
            cavaleiro.pos.x *= scaleX;
            cavaleiro.pos.y *= scaleY;

            // Atualiza as métricas do mapa e do player
            level_atualizar_tile(&level);

            lastScreenWidth = GetScreenWidth();
            lastScreenHeight = GetScreenHeight();
        }

        UpdatePlayer(&cavaleiro, deltaTime, &level);
        Boss_Update(&boss, deltaTime, cavaleiro.pos, cavaleiro.raio, &level);

        // Dano no boss por colisão com hitbox do player (sem mexer no player)
        // Observação: aqui o dano é fixo só para validar HUD/feedback.
        const int PLAYER_DEBUG_DAMAGE = 10;
        const int BOSS_ATTACK_DAMAGE = 15;

        bool hitboxActiveNow = cavaleiro.isHitboxActive;

        // Só tenta dar hit quando a hitbox "acabou de ativar"
        if (hitboxActiveNow && !lastHitboxActive && boss.hp > 0) {
            if (CheckCollisionCircles(cavaleiro.hitboxCenter, cavaleiro.hitboxRadius,
                                      boss.pos, boss.raio)) {
                boss.hp -= PLAYER_DEBUG_DAMAGE;
                if (boss.hp < 0) boss.hp = 0;
            }
        }

        lastHitboxActive = hitboxActiveNow;

        // Dano no player por ataque do boss
        AttackCircle bossAttack;
        bool bossAttackActive = Boss_GetAttackCircle(&boss, &bossAttack);
        
        if (bossAttackActive && !lastBossAttackActive && cavaleiro.hp > 0) {
            if (CheckCollisionCircles(bossAttack.center, bossAttack.radius,
                                      cavaleiro.pos, cavaleiro.raio)) {
                PlayerTakeDamage(&cavaleiro, BOSS_ATTACK_DAMAGE);
            }
        }
        
        lastBossAttackActive = bossAttackActive;

        BeginDrawing();
            ClearBackground(BLACK);
            level_desenhar(&level);
            DrawPlayer(cavaleiro);
            Boss_Draw(&boss);

            // HUD do player (canto superior esquerdo)
            DrawPlayerHUD(&cavaleiro, 10, 10);

            // FPS (canto superior direito)
            DrawFPS(GetScreenWidth() - 80, 10);

            // Texto de ajuda - abaixo do HUD do player
            DrawText("Alt+Enter para Fullscreen", 10, 110, 20, GRAY);

            // HUD do boss (centro inferior)
            DrawBossHUD(&boss);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}