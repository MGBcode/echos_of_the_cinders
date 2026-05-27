#ifndef MENU_H
#define MENU_H

#include "raylib.h"

typedef enum GameState {

    STATE_MENU,
    STATE_TREINO,
    STATE_BOSS,
    STATE_DERROTA,
    STATE_AGRADECIMENTO,

    
    STATE_RECORDES,
    STATE_SALVAR_TEMPO

} GameState;

typedef enum MenuOpcao {

    MENU_OPCAO_NENHUMA,
    MENU_OPCAO_JOGAR,
    MENU_OPCAO_RECORDES

} MenuOpcao;

MenuOpcao Menu_DrawPrincipal(void);

void Menu_DrawDerrota(void);

void Menu_DrawAgradecimento(void);


void Menu_DrawRecordes(void);

#endif