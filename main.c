#include "raylib.h"
#include "player.h"
#include "level.h"
#include "enemy.h"
#include "menu.h"
#include "timer.h"

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

static void RestoreCursorOnExit(void) {
    ShowCursor();
}

static void DrawBossHUD(const Boss *boss) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    int barW = (int)(screenW * 0.6f);
    int barH = 20;
    int barX = (screenW - barW) / 2;
    int barY = screenH - 60;

    char txt[64];
    snprintf(txt, sizeof(txt), "BOSS HP: %d/%d", boss->hp, boss->hpMax);

    int textW = MeasureText(txt, 20);

    DrawText(txt,
             (screenW - textW) / 2,
             barY - 26,
             20,
             LIGHTGRAY);

    DrawRectangle(barX, barY, barW, barH,
                  (Color){40,40,40,220});

    DrawRectangleLines(barX, barY, barW, barH,
                       (Color){110,110,110,255});

    float ratio = (float)boss->hp / (float)boss->hpMax;

    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    int fillW = (int)(barW * ratio);

    Color fill =
        (ratio > 0.5f) ? GREEN :
        (ratio > 0.25f) ? ORANGE :
        RED;

    DrawRectangle(barX + 1,
                  barY + 1,
                  fillW - 2 > 0 ? fillW - 2 : 0,
                  barH - 2,
                  fill);
}

static void ResetJogo(Player *cavaleiro,
                      Boss *boss,
                      TrainingDummy *dummy,
                      Level *level)
{
    level_carregar_sala(level, SALA_TREINO);
    level_atualizar_tile(level);

    InitPlayer(cavaleiro,
               2.5f,
               ALTURA_MAPA / 2.0f,
               level);

    TrainingDummy_Init(dummy,
                       7.5f,
                       ALTURA_MAPA / 2.0f + 0.5f,
                       level);
}

int main(void) {

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_FULLSCREEN_MODE);

    InitWindow(800, 600, "Echos of the Cinders");

    atexit(RestoreCursorOnExit);

    SetExitKey(KEY_NULL);

    SetTargetFPS(60);

    Player cavaleiro;
    Level level;
    Boss boss;
    TrainingDummy dummy;

    level_iniciar(&level);

    ResetJogo(&cavaleiro,
              &boss,
              &dummy,
              &level);

    TimerData timer;

    Timer_Reset(&timer);

    GameState gameState = STATE_MENU;

    bool bossJaMorreu = false;

    int lastScreenWidth  = GetScreenWidth();
    int lastScreenHeight = GetScreenHeight();

    bool lastHitboxActive = false;

    bool isPaused = false;
    bool shouldExit = false;

    char iniciais[4] = "";
    int letrasDigitadas = 0;

    while (!WindowShouldClose() && !shouldExit) {

        float dt = GetFrameTime();
        bool escPressed = IsKeyPressed(KEY_ESCAPE);
        bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

        if (shiftDown && escPressed) {
            ShowCursor();
            shouldExit = true;
            continue;
        }

        if ((gameState == STATE_TREINO || gameState == STATE_BOSS) && IsKeyPressed(KEY_C)) {
            isPaused = !isPaused;
        }

        if (gameState != STATE_TREINO && gameState != STATE_BOSS) {
            isPaused = false;
        }

        if ((gameState == STATE_TREINO ||
             gameState == STATE_BOSS) &&

            (IsWindowResized() ||
             GetScreenWidth() != lastScreenWidth))
        {
            level_atualizar_tile(&level);

            cavaleiro.pos.x = cavaleiro.tile_pos.x * level.tamanho_tile;
            cavaleiro.pos.y = cavaleiro.tile_pos.y * level.tamanho_tile_h;

            if (gameState == STATE_BOSS) {
                boss.pos.x = (boss.pos.x / lastScreenWidth) * GetScreenWidth();
                boss.pos.y = (boss.pos.y / lastScreenHeight) * GetScreenHeight();
            }

            dummy.pos.x = dummy.tile_pos.x * level.tamanho_tile;
            dummy.pos.y = dummy.tile_pos.y * level.tamanho_tile_h;
            lastScreenHeight = GetScreenHeight();
        }

        switch (gameState) {

        case STATE_MENU: {

            BeginDrawing();

            MenuOpcao opcao =
                Menu_DrawPrincipal();

            EndDrawing();

            if (opcao == MENU_OPCAO_JOGAR) {

                ResetJogo(&cavaleiro,
                          &boss,
                          &dummy,
                          &level);

                level_atualizar_tile(&level);

                Timer_Reset(&timer);

                bossJaMorreu = false;

                lastHitboxActive = false;

                HideCursor();

                gameState = STATE_TREINO;
            }

            else if (opcao ==
                     MENU_OPCAO_RECORDES)
            {
                gameState = STATE_RECORDES;
            }

            else if (opcao == MENU_OPCAO_COMANDOS)
            {
                gameState = STATE_COMANDOS;
            }

            else if (opcao == MENU_OPCAO_SAIR)
            {
                ShowCursor();
                shouldExit = true;
            }

        } break;

        case STATE_RECORDES: {

            BeginDrawing();

            Menu_DrawRecordes();

            EndDrawing();

            if (IsKeyPressed(KEY_ENTER) || escPressed)
            {
                ShowCursor();
                gameState = STATE_MENU;
            }

        } break;

        case STATE_COMANDOS: {

            BeginDrawing();

            Menu_DrawComandos();

            EndDrawing();

            if (IsKeyPressed(KEY_ESCAPE))
            {
                ShowCursor();
                gameState = STATE_MENU;
            }

        } break;

        case STATE_TREINO: {

            if (isPaused && escPressed) {
                isPaused = false;
                ShowCursor();
                Timer_Reset(&timer);
                gameState = STATE_MENU;
                break;
            }

            if (!isPaused) {

                if (IsKeyPressed(KEY_UP)) {
                    dummy.raio += 1.0f;
                }

                if (IsKeyPressed(KEY_DOWN)) {
                    dummy.raio -= 1.0f;
                    if (dummy.raio < 5.0f) dummy.raio = 5.0f;
                }

                if (dummy.hitFlashTimer > 0.0f)
                    dummy.hitFlashTimer -= dt;

                UpdatePlayer(&cavaleiro,
                             dt,
                             &level);

                bool hitNow =
                    cavaleiro.isHitboxActive;

                if (hitNow &&
                    !lastHitboxActive &&
                    dummy.alive)
                {
                    if (CheckCollisionCircles(
                            cavaleiro.hitboxCenter,
                            cavaleiro.hitboxRadius,
                            dummy.pos,
                            dummy.raio))
                    {
                        TrainingDummy_TakeDamage(
                            &dummy,
                            10);
                    }
                }

                lastHitboxActive = hitNow;

                TipeTile tileAtual =
                    level_get_tile(&level,
                                   cavaleiro.pos.x,
                                   cavaleiro.pos.y);

                if (tileAtual == TILE_PORTA) {
                    float doorTileX = LARGURA_MAPA - 1.5f;
                    float doorTileY = ALTURA_MAPA / 2.0f;
                    float doorX = doorTileX * level.tamanho_tile;
                    float doorY = doorTileY * level.tamanho_tile_h;
                    
                    float distToDoor = sqrtf(
                        (cavaleiro.pos.x - doorX) * (cavaleiro.pos.x - doorX) +
                        (cavaleiro.pos.y - doorY) * (cavaleiro.pos.y - doorY)
                    );
                    
                    float doorRadius = level.tamanho_tile * 1.2f;
                    
                    if (distToDoor < doorRadius) {

                        level_carregar_sala(&level,
                                            SALA_BOSS);

                        level_atualizar_tile(&level);

                        cavaleiro.tile_pos.x = 2.5f;
                        cavaleiro.tile_pos.y = ALTURA_MAPA / 2.0f + 0.5f;
                        cavaleiro.pos.x =
                            level.tamanho_tile * cavaleiro.tile_pos.x;

                        cavaleiro.pos.y =
                            level.tamanho_tile_h * cavaleiro.tile_pos.y;

                        Boss_Init(&boss,
                            (Vector2){
                                level.tamanho_tile *
                                (LARGURA_MAPA * 3 / 4),

                                level.tamanho_tile_h *
                                (ALTURA_MAPA / 2)
                            },
                            &level);

                        Timer_Start(&timer);

                        bossJaMorreu = false;

                        gameState = STATE_BOSS;

                        break;
                    }
                }
            }

            BeginDrawing();

            ClearBackground(BLACK);

            level_desenhar(&level);

            DrawPlayer(cavaleiro);

            TrainingDummy_Draw(&dummy);

            DrawPlayerHUD(&cavaleiro,
                          10,
                          10);

            DrawText("SALA DE TREINO",
                     10,
                     110,
                     18,
                     DARKGRAY);

            DrawText("C para pausar/resumir",
                     10,
                     140,
                     18,
                     DARKGRAY);

            if (isPaused) {
                int screenW = GetScreenWidth();
                int screenH = GetScreenHeight();
                DrawRectangle(0, 0, screenW, screenH, (Color){0, 0, 0, 150});
                const char *pauseText = "JOGO PAUSADO - C para retornar | ESC para sair";
                int textW = MeasureText(pauseText, 28);
                DrawText(pauseText,
                         (screenW - textW) / 2,
                         screenH / 2 - 16,
                         28,
                         YELLOW);
            }

            DrawFPS(GetScreenWidth() - 100, 10);

            EndDrawing();

        } break;

        case STATE_BOSS: {

            if (isPaused && escPressed) {
                isPaused = false;
                ShowCursor();
                Timer_Reset(&timer);
                gameState = STATE_MENU;
                break;
            }

            if (!isPaused) {
                Timer_Update(&timer, dt);

                UpdatePlayer(&cavaleiro,
                             dt,
                             &level);

                Boss_Update(&boss,
                            dt,
                            &cavaleiro,
                            &level);

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

                if (!cavaleiro.alive) {

                    Timer_Reset(&timer);

                    gameState = STATE_DERROTA;

                    break;
                }

                if (!bossJaMorreu &&
                    boss.hp <= 0)
                {
                    bossJaMorreu = true;

                    level_abrir_saida_boss(&level);
                }

                bool hitNow =
                    cavaleiro.isHitboxActive;

                
                if (hitNow &&
                    !lastHitboxActive &&
                    boss.hp > 0)
                {
                    if (CheckCollisionCircles(
                            cavaleiro.hitboxCenter,
                            cavaleiro.hitboxRadius,
                            boss.pos,
                            boss.raio))
                    {
                        boss.hp -= cavaleiro.currentAttackDamage;
                        if (boss.hp < 0)
                            boss.hp = 0;
                    }
                }

                lastHitboxActive = hitNow;

                (void)Boss_GetAttackCircle(&boss, NULL);

                if (bossJaMorreu) {

                    TipeTile tileAtual =
                        level_get_tile(
                            &level,
                            cavaleiro.pos.x,
                            cavaleiro.pos.y);

                    if (tileAtual == TILE_PORTA) {
                        float doorTileX = LARGURA_MAPA - 1.5f;
                        float doorTileY = ALTURA_MAPA / 2.0f;
                        float doorX = doorTileX * level.tamanho_tile;
                        float doorY = doorTileY * level.tamanho_tile_h;
                        
                        float distToDoor = sqrtf(
                            (cavaleiro.pos.x - doorX) * (cavaleiro.pos.x - doorX) +
                            (cavaleiro.pos.y - doorY) * (cavaleiro.pos.y - doorY)
                        );
                        
                        float doorRadius = level.tamanho_tile * 1.2f;
                        
                        if (distToDoor < doorRadius) {

                            Timer_Stop(&timer);

                            letrasDigitadas = 0;

                            memset(iniciais, 0,
                                   sizeof(iniciais));

                            gameState =
                                STATE_SALVAR_TEMPO;

                            break;
                        }
                    }
                }
            }

            BeginDrawing();

            ClearBackground(BLACK);

            level_desenhar(&level);

            DrawPlayer(cavaleiro);

            Boss_Draw(&boss);

            DrawPlayerHUD(&cavaleiro,
                          10,
                          10);
            DrawText("Alt+Enter para Fullscreen", 10, 110, 20, GRAY);
            DrawText("C para Pausar/Resumir", 10, 140, 20, GRAY);
            DrawBossHUD(&boss);

            DrawTimer(&timer);
            if (bossJaMorreu) {
                int screenW = GetScreenWidth();
                int screenH = GetScreenHeight();

                
                DrawRectangle(0, 0, screenW, screenH,
                              (Color){0, 0, 0, 40});

                
                const char *victoryMsg =
                    "BOSS DERROTADO!";
                int victoryW =
                    MeasureText(victoryMsg, 48);
                DrawText(victoryMsg,
                         (screenW - victoryW) / 2,
                         screenH / 2 - 60,
                         48,
                         (Color){210, 90, 40, 255});

                
                const char *instrMsg =
                    "Dirija-se para a SAIDA para completar!";
                int instrW = MeasureText(instrMsg, 24);
                DrawText(instrMsg,
                         (screenW - instrW) / 2,
                         screenH / 2 + 20,
                         24,
                         GOLD);
            }

            
            if (isPaused) {
                int screenW = GetScreenWidth();
                int screenH = GetScreenHeight();
                DrawRectangle(0, 0, screenW, screenH, (Color){0, 0, 0, 150});
                const char *pauseText = "PAUSED";
                int textW = MeasureText(pauseText, 80);
                DrawText(pauseText, (screenW - textW) / 2, screenH / 2 - 40, 80, YELLOW);
            }

            EndDrawing();

        } break;

        case STATE_SALVAR_TEMPO: {

            int key = GetCharPressed();

            while (key > 0) {

                if (key >= 'a' &&
                    key <= 'z')
                {
                    key -= 32;
                }

                if (key >= 'A' &&
                    key <= 'Z' &&
                    letrasDigitadas < 3)
                {
                    iniciais[letrasDigitadas] =
                        (char)key;

                    letrasDigitadas++;
                }

                key = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE) &&
                letrasDigitadas > 0)
            {
                letrasDigitadas--;

                iniciais[letrasDigitadas] = '\0';
            }

            if (letrasDigitadas == 3 &&
                IsKeyPressed(KEY_ENTER))
            {
                SaveScore(iniciais,
                          timer.currentTime);

                gameState =
                    STATE_AGRADECIMENTO;
            }

            BeginDrawing();

            ClearBackground(BLACK);

            DrawText("NOVO RECORDE!",
                     240,
                     120,
                     40,
                     GOLD);

            
            char tempoStr[64];
            int minutos = (int)(timer.currentTime / 60.0f);
            int segundos = (int)timer.currentTime % 60;
            sprintf(tempoStr,
                    "Tempo: %02d:%02d",
                    minutos,
                    segundos);
            int tempoW = MeasureText(tempoStr, 24);
            DrawText(tempoStr,
                     (GetScreenWidth() - tempoW) / 2,
                     180,
                     24,
                     GOLD);

            DrawText("Digite 3 letras:",
                     220,
                     240,
                     30,
                     LIGHTGRAY);

            
            DrawRectangle(250,
                          310,
                          300,
                          60,
                          (Color){40, 40, 40, 255});
            DrawRectangleLinesEx(
                (Rectangle){250, 310, 300, 60},
                2,
                GOLD);

            
            int boxWidth = 80;
            int boxHeight = 50;
            int startX = 280;
            int boxY = 320;
            int spacing = 100;

            for (int i = 0; i < 3; i++) {
                
                Color boxColor = (i < letrasDigitadas)
                    ? (Color){100, 100, 100, 255}
                    : (Color){50, 50, 50, 255};
                Color textColor = (i < letrasDigitadas)
                    ? WHITE
                    : GRAY;

                DrawRectangle(startX + i * spacing,
                              boxY,
                              boxWidth,
                              boxHeight,
                              boxColor);
                DrawRectangleLines(startX + i * spacing,
                                   boxY,
                                   boxWidth,
                                   boxHeight,
                                   GOLD);

                
                if (i < letrasDigitadas) {
                    char letra[2] = {iniciais[i], '\0'};
                    int letraW = MeasureText(letra, 48);
                    DrawText(letra,
                             startX + i * spacing + (boxWidth - letraW) / 2,
                             boxY + 2,
                             48,
                             textColor);
                } else {
                    DrawText("_",
                             startX + i * spacing + (boxWidth - MeasureText("_", 40)) / 2,
                             boxY + 8,
                             40,
                             GRAY);
                }
            }

            
            const char *instrucao =
                (letrasDigitadas < 3)
                ? "Digite as 3 letras"
                : "ENTER para salvar | BACKSPACE para corrigir";
            int instrW = MeasureText(instrucao, 20);
            DrawText(instrucao,
                     (GetScreenWidth() - instrW) / 2,
                     430,
                     20,
                     LIGHTGRAY);

            
            char posStr[32];
            sprintf(posStr, "%d/3", letrasDigitadas);
            int posW = MeasureText(posStr, 20);
            DrawText(posStr,
                     (GetScreenWidth() - posW) / 2,
                     470,
                     20,
                     GRAY);

            EndDrawing();

        } break;

        case STATE_DERROTA: {

            BeginDrawing();

            Menu_DrawDerrota();

            EndDrawing();

            if (IsKeyPressed(KEY_ENTER))
            {
                ShowCursor();
                gameState = STATE_MENU;
            }

        } break;

        
        
        

        case STATE_AGRADECIMENTO: {

            BeginDrawing();

            Menu_DrawAgradecimento();

            EndDrawing();

            if (IsKeyPressed(KEY_ENTER))
            {
                ShowCursor();
                gameState = STATE_MENU;
            }

        } break;
        }
    }

    CloseWindow();

    return 0;
}