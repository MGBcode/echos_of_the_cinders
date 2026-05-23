#include "raylib.h"
#include "player.h"
#include "level.h"
#include "enemy.h"
#include <stdio.h>
#include <math.h>

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

    // Pausa do jogo
    bool isPaused = false;
    bool lastCKeyPressed = false;

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        // Detectar pausa com tecla C
        bool currentCKeyPressed = IsKeyPressed(KEY_C);
        if (currentCKeyPressed && !lastCKeyPressed) {
            isPaused = !isPaused;
        }
        lastCKeyPressed = currentCKeyPressed;

        // SÓ atualiza os cálculos do mapa se a janela mudar de tamanho
        if (IsWindowResized() || GetScreenWidth() != lastScreenWidth) {
            float scaleX = (float)GetScreenWidth() / lastScreenWidth;
            float scaleY = (float)GetScreenHeight() / lastScreenHeight;

            // Ajusta a posição do jogador proporcionalmente
            cavaleiro.pos.x *= scaleX;
            cavaleiro.pos.y *= scaleY;
            boss.pos.x *= scaleX;
            boss.pos.y *= scaleY;

            // Atualiza as métricas do mapa e do player
            level_atualizar_tile(&level);

            lastScreenWidth = GetScreenWidth();
            lastScreenHeight = GetScreenHeight();
        }

        // Só atualiza o jogo se não estiver pausado
        if (!isPaused) {
            UpdatePlayer(&cavaleiro, deltaTime, &level);
            Boss_Update(&boss, deltaTime, cavaleiro.pos, cavaleiro.raio, &level);

            // Colisão sólida entre Player e Boss
            {
                float dx = cavaleiro.pos.x - boss.pos.x;
                float dy = cavaleiro.pos.y - boss.pos.y;
                float dist = sqrtf(dx * dx + dy * dy);
                float minDist = cavaleiro.raio + boss.raio;
                if (dist < minDist) {
                    if (dist > 0.001f) {
                        float push = minDist - dist + 0.5f;
                        cavaleiro.pos.x += dx / dist * push;
                        cavaleiro.pos.y += dy / dist * push;
                    } else {
                        cavaleiro.pos.x += minDist;
                    }
                }
            }

            // Dano no boss por colisão com hitbox do player (sem mexer no player)
            // Observação: aqui o dano é fixo só para validar HUD/feedback.
            const int PLAYER_DEBUG_DAMAGE = 10;

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
                    PlayerTakeDamage(&cavaleiro, bossAttack.damage);
                }
            }
            
            lastBossAttackActive = bossAttackActive;
        }

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
            DrawText("C para Pausar/Resumir", 10, 140, 20, GRAY);
            DrawText(TextFormat("Curas: %d / %d", cavaleiro.frascosAtuais, cavaleiro.frascosMax), 10, 170, 20, LIGHTGRAY);
            if (cavaleiro.isHealing) {
                DrawText("Curando...", 10, 200, 20, GOLD);
            }

            // HUD do boss (centro inferior)
            DrawBossHUD(&boss);

            // Tela de pausa
            if (isPaused) {
                int screenW = GetScreenWidth();
                int screenH = GetScreenHeight();
                DrawRectangle(0, 0, screenW, screenH, (Color){0, 0, 0, 150});
                const char *pauseText = "PAUSED";
                int textW = MeasureText(pauseText, 80);
                DrawText(pauseText, (screenW - textW) / 2, screenH / 2 - 40, 80, YELLOW);
            }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}