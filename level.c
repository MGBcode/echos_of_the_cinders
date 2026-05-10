#include "level.h"
#include <raylib.h>


static int mapa_base[ALTURA_MAPA][LARGURA_MAPA] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1},
};

void level_atualizar_tile(Level *level) {
    level->tamanho_tile   = GetScreenWidth()  / LARGURA_MAPA;
    level->tamanho_tile_h = (GetScreenHeight() - 2) / ALTURA_MAPA;
}

void level_iniciar(Level *level) {
    for (int y = 0; y < ALTURA_MAPA; y++)
        for (int x = 0; x < LARGURA_MAPA; x++)
            level->grade[y][x] = (TipeTile)mapa_base[y][x];

    level_atualizar_tile(level);
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

void level_abrir_porta(Level *level) {
    level->grade[0][LARGURA_MAPA / 2] = TILE_PORTA;
}

void level_desenhar(Level *level) {
    int tw = level->tamanho_tile;
    int th = level->tamanho_tile_h;

    for (int y = 0; y < ALTURA_MAPA; y++) {
        for (int x = 0; x < LARGURA_MAPA; x++) {
            Color cor;
            switch (level->grade[y][x]) {
                case TILE_VAZIO:  cor = (Color){ 30,  30,  30,  255 }; break;
                case TILE_PAREDE: cor = (Color){ 80,  80,  80,  255 }; break;
                case TILE_BLOCO:  cor = (Color){ 160, 110, 40,  255 }; break;
                case TILE_PORTA:  cor = (Color){ 80,  220, 80,  255 }; break;
                default:          cor = BLACK;                          break;
            }
            DrawRectangle(x * tw, y * th, tw, th, cor);
            DrawRectangleLines(x * tw, y * th, tw, th, BLACK);
        }
    }
}
