#ifndef LEVEL_H
#define LEVEL_H

#include <stdbool.h>

// Aumentamos a grade para os quadrados ficarem menores
#define LARGURA_MAPA  25
#define ALTURA_MAPA   19

typedef enum {
    TILE_VAZIO,
    TILE_PAREDE,
    TILE_BLOCO,
    TILE_PORTA
} TipeTile;

typedef struct {
    TipeTile grade[ALTURA_MAPA][LARGURA_MAPA];
    int tamanho_tile;
    int tamanho_tile_h;
} Level;

void     level_iniciar(Level *level);
void     level_atualizar_tile(Level *level);
void     level_desenhar(Level *level);
bool     level_pode_mover(Level *level, int tile_x, int tile_y);
void     level_abrir_porta(Level *level);
TipeTile level_get_tile(Level *level, float px, float py);

#endif