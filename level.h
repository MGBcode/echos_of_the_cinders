#ifndef LEVEL_H
#define LEVEL_H

#include <stdbool.h>
#include "raylib.h"

#define LARGURA_MAPA 25
#define ALTURA_MAPA 19

typedef enum {
    TILE_VAZIO,
    TILE_PAREDE,
    TILE_BLOCO,
    TILE_PORTA
} TipeTile;

typedef enum {
    SALA_TREINO = 0,
    SALA_BOSS
} SalaId;

typedef struct {
    TipeTile grade[ALTURA_MAPA][LARGURA_MAPA];
    int tamanho_tile;
    int tamanho_tile_h;
    SalaId salaAtual;
    Texture2D chao_tex;
    Texture2D parede_tex;
} Level;

void level_iniciar(Level *level);
void level_carregar_sala(Level *level, SalaId sala);
void level_atualizar_tile(Level *level);
void level_desenhar(Level *level);
void level_load_textures(Level *level);
void level_unload_textures(Level *level);
bool level_pode_mover(Level *level, int tile_x, int tile_y);
void level_abrir_saida_boss(Level *level);
TipeTile level_get_tile(Level *level, float px, float py);

#endif