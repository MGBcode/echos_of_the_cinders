#include "level.h"
#include <raylib.h>

#include <stdio.h>

void level_atualizar_tile(Level *level) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    if (sw <= 0) sw = 800;
    if (sh <= 0) sh = 600;

    level->tamanho_tile   = sw / LARGURA_MAPA;
    level->tamanho_tile_h = sh / ALTURA_MAPA;
}

void level_carregar_sala(Level *level, SalaId sala) {
    level->salaAtual = sala;

    for (int y = 0; y < ALTURA_MAPA; y++) {
        for (int x = 0; x < LARGURA_MAPA; x++) {
            if (y == 0 || y == ALTURA_MAPA - 1 || x == 0 || x == LARGURA_MAPA - 1)
                level->grade[y][x] = TILE_PAREDE;
            else
                level->grade[y][x] = TILE_VAZIO;
        }
    }

    if (sala == SALA_TREINO) {
        level->grade[ALTURA_MAPA / 2][LARGURA_MAPA - 2] = TILE_PORTA;

    } else if (sala == SALA_BOSS) {
        // Saida do boss comeca FECHADA (parede) e so vira porta quando o boss morrer.
        // Coordenada alinhada com a checagem de porta em main.c.
        level->grade[ALTURA_MAPA / 2][LARGURA_MAPA - 2] = TILE_PAREDE;
    }

    level_atualizar_tile(level);
}

void level_iniciar(Level *level) {
    level_carregar_sala(level, SALA_TREINO);
}

bool level_pode_mover(Level *level, int tile_x, int tile_y) {
    if (tile_x < 0 || tile_x >= LARGURA_MAPA) return false;
    if (tile_y < 0 || tile_y >= ALTURA_MAPA)  return false;

    TipeTile tile = level->grade[tile_y][tile_x];
    return tile == TILE_VAZIO || tile == TILE_PORTA;
}

TipeTile level_get_tile(Level *level, float px, float py) {
    int tx = (int)(px / level->tamanho_tile);
    int ty = (int)(py / level->tamanho_tile_h);

    if (tx < 0 || tx >= LARGURA_MAPA) return TILE_PAREDE;
    if (ty < 0 || ty >= ALTURA_MAPA)  return TILE_PAREDE;

    return level->grade[ty][tx];
}

void level_abrir_saida_boss(Level *level) {
    level->grade[ALTURA_MAPA / 2][LARGURA_MAPA - 2] = TILE_PORTA;
}

void level_desenhar(Level *level) {
    int tw = level->tamanho_tile;
    int th = level->tamanho_tile_h;

    for (int y = 0; y < ALTURA_MAPA; y++) {
        for (int x = 0; x < LARGURA_MAPA; x++) {
            // Desenhar usando texturas quando disponíveis, caso contrário fallback em retângulos coloridos
            Rectangle dest = { x * tw, y * th, tw, th };

            if (level->grade[y][x] == TILE_VAZIO) {
                if (level->chao_tex.id != 0)
                {
                    Rectangle src = { 0, 0, (float)level->chao_tex.width, (float)level->chao_tex.height };
                    DrawTexturePro(level->chao_tex, src, dest, (Vector2){0,0}, 0.0f, WHITE);
                }
                else DrawRectangle(x * tw, y * th, tw, th, (Color){30,30,30,255});
            } else if (level->grade[y][x] == TILE_PAREDE) {
                if (level->parede_tex.id != 0)
                {
                    Rectangle src = { 0, 0, (float)level->parede_tex.width, (float)level->parede_tex.height };
                    DrawTexturePro(level->parede_tex, src, dest, (Vector2){0,0}, 0.0f, WHITE);
                }
                else DrawRectangle(x * tw, y * th, tw, th, (Color){80,80,80,255});
            } else {
                // Para blocos/portas, desenha chão e um overlay de cor
                if (level->chao_tex.id != 0)
                {
                    Rectangle src = { 0, 0, (float)level->chao_tex.width, (float)level->chao_tex.height };
                    DrawTexturePro(level->chao_tex, src, dest, (Vector2){0,0}, 0.0f, WHITE);
                } else DrawRectangle(x * tw, y * th, tw, th, (Color){30,30,30,255});

                Color overlay = (level->grade[y][x] == TILE_BLOCO) ? (Color){160,110,40,200} : (Color){160,110,40,200};
                DrawRectangle(x * tw, y * th, tw, th, overlay);
                DrawRectangleLines(x * tw, y * th, tw, th, BLACK);
            }
        }
    }

    
    int portaTileY = ALTURA_MAPA / 2;
    int portaPixelX = (LARGURA_MAPA - 1) * tw - 2;
    int portaPixelY = portaTileY * th + th / 2 - 10;

    if (level->salaAtual == SALA_TREINO) {
        DrawText("ARENA ->", portaPixelX - 80, portaPixelY, 14, (Color){80, 220, 80, 255});
    } else if (level->salaAtual == SALA_BOSS) {
        TipeTile portaSaida = level->grade[portaTileY][LARGURA_MAPA - 2];
        if (portaSaida == TILE_PORTA) {
            DrawText("SAIDA ->", portaPixelX - 80, portaPixelY, 14, (Color){80, 220, 80, 255});
        } else {
            DrawText("BLOQUEADA", portaPixelX - 90, portaPixelY, 14, RED);
        }
    }
}

void level_load_textures(Level *level) {
    // Inicializar com 0 para evitar dados inválidos
    level->chao_tex = (Texture2D){0};
    level->parede_tex = (Texture2D){0};

    // Tentar carregar as texturas do diretório de assets
    level->chao_tex = LoadTexture("assets/mapa/chao.png");
    level->parede_tex = LoadTexture("assets/mapa/parede.png");

    // Log simples caso falhe
    if (level->chao_tex.id == 0) TraceLog(LOG_WARNING, "Falha ao carregar assets/mapa/chao.png");
    if (level->parede_tex.id == 0) TraceLog(LOG_WARNING, "Falha ao carregar assets/mapa/parede.png");
}

void level_unload_textures(Level *level) {
    if (level->chao_tex.id != 0) UnloadTexture(level->chao_tex);
    if (level->parede_tex.id != 0) UnloadTexture(level->parede_tex);
    level->chao_tex = (Texture2D){0};
    level->parede_tex = (Texture2D){0};
}