#include "raylib.h"
#include "player.h"
#include "level.h"
#include "enemy.h"

// HUD do boss abaixo do FPS (canto superior esquerdo)
static void DrawBossHUD(const Boss *boss, int x, int y) {
    // Texto
    DrawText(TextFormat("BOSS HP: %d/%d", boss->hp, boss->hpMax), x, y, 20, LIGHTGRAY);

    // Barra
    const int barW = 220;
    const int barH = 14;
    const int barY = y + 24;

    // fundo
    DrawRectangle(x, barY, barW, barH, (Color){ 40, 40, 40, 220 });
    DrawRectangleLines(x, barY, barW, barH, (Color){ 110, 110, 110, 255 });

    float ratio = 0.0f;
    if (boss->hpMax > 0) ratio = (float)boss->hp / (float)boss->hpMax;
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    int fillW = (int)(barW * ratio);

    Color fill = (ratio > 0.5f) ? GREEN : (ratio > 0.25f ? ORANGE : RED);
    DrawRectangle(x + 1, barY + 1, fillW - 2 > 0 ? fillW - 2 : 0, barH - 2, fill);
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

            // FPS
            DrawFPS(10, 10);
            DrawText("Alt+Enter para Fullscreen", 10, 40, 20, GRAY);

            // HUD do boss logo abaixo do FPS
            // (abaixo da linha do FPS e sem conflitar com o texto do fullscreen)
            DrawBossHUD(&boss, 10, 70);
            
            // HUD do player (canto superior direito)
            int playerHudX = GetScreenWidth() - 240;
            int playerHudY = 10;
            DrawPlayerHUD(&cavaleiro, playerHudX, playerHudY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}