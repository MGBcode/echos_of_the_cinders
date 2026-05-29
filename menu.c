#include "menu.h"
#include "timer.h"
#include <stdio.h>

static bool DrawBotao(Rectangle rect, const char *texto) {
    Vector2 mouse = GetMousePosition();
    bool hover = CheckCollisionPointRec(mouse, rect);

    Color fundoCor  = hover ? (Color){200, 200, 200, 255} : (Color){60, 60, 60, 255};
    Color textoCor  = hover ? BLACK : LIGHTGRAY;
    Color bordaCor  = hover ? WHITE : (Color){120, 120, 120, 255};

    DrawRectangleRec(rect, fundoCor);
    DrawRectangleLinesEx(rect, 2, bordaCor);

    int tamanho = 22;
    int tw = MeasureText(texto, tamanho);
    int tx = (int)rect.x + ((int)rect.width  - tw) / 2;
    int ty = (int)rect.y + ((int)rect.height - tamanho) / 2;
    DrawText(texto, tx, ty, tamanho, textoCor);

    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static Rectangle MakeMenuButton(int centerX, int topY, int width, int height) {
    return (Rectangle){ (float)centerX - width / 2.0f, (float)topY, (float)width, (float)height };
}

MenuOpcao Menu_DrawPrincipal(void) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    ClearBackground((Color){10, 10, 10, 255});
    const char *titulo = "ECHOS OF THE CINDERS";
    int tituloSize = 48;
    int tituloW    = MeasureText(titulo, tituloSize);
    DrawText(titulo, (sw - tituloW) / 2, sh / 4, tituloSize, (Color){210, 90, 40, 255});
    const char *sub = "um jogo de acao e combate";
    int subSize = 20;
    int subW    = MeasureText(sub, subSize);
    DrawText(sub, (sw - subW) / 2, sh / 4 + tituloSize + 12, subSize, (Color){140, 140, 140, 255});
    DrawLine(sw / 4, sh / 4 + tituloSize + 50,
             sw * 3 / 4, sh / 4 + tituloSize + 50,
             (Color){80, 80, 80, 255});
    int botaoW  = sw < 420 ? sw - 80 : 280;
    if (botaoW < 220) botaoW = 220;
    int botaoH  = 54;
    int espacamento = 16;
    int totalAltura = (botaoH * 4) + (espacamento * 3);
    int topY = sh / 2 - totalAltura / 2;
    int minTopY = sh / 4 + tituloSize + 86;
    if (topY < minTopY) topY = minTopY;

    Rectangle rJogar    = MakeMenuButton(sw / 2, topY, botaoW, botaoH);
    Rectangle rComandos = MakeMenuButton(sw / 2, topY + botaoH + espacamento, botaoW, botaoH);
    Rectangle rRecorde  = MakeMenuButton(sw / 2, topY + (botaoH + espacamento) * 2, botaoW, botaoH);
    Rectangle rSair     = MakeMenuButton(sw / 2, topY + (botaoH + espacamento) * 3, botaoW, botaoH);

    if (DrawBotao(rJogar,    "JOGAR"))            return MENU_OPCAO_JOGAR;
    if (DrawBotao(rComandos, "COMANDOS"))         return MENU_OPCAO_COMANDOS;
    if (DrawBotao(rRecorde,  "MELHORES TEMPOS"))  return MENU_OPCAO_RECORDES;
    if (DrawBotao(rSair,     "SAIR"))             return MENU_OPCAO_SAIR;

    const char *rodape = "WASD: mover | ESPACO: dash | LMB: atacar | ESC: voltar";
    int rodapeSize = 16;
    int rodapeW    = MeasureText(rodape, rodapeSize);
    DrawText(rodape, (sw - rodapeW) / 2, sh - 40, rodapeSize, (Color){80, 80, 80, 255});

    return MENU_OPCAO_NENHUMA;
}

void Menu_DrawComandos(void) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    ClearBackground((Color){10, 10, 10, 255});

    const char *titulo = "COMANDOS";
    int tituloSize = 48;
    int tituloW = MeasureText(titulo, tituloSize);
    DrawText(titulo,
             (sw - tituloW) / 2,
             70,
             tituloSize,
             (Color){210, 90, 40, 255});

    const char *linhas[] = {
        "WASD - mover",
        "ESPACO - dash",
        "CLIQUE ESQUERDO - atacar",
        "CLIQUE DIREITO - aparar",
        "Q - curar",
        "ESC - voltar ao menu principal",
        "SHIFT + ESC - fechar o jogo"
    };

    int y = 180;
    for (int i = 0; i < 7; i++) {
        int size = 26;
        int textW = MeasureText(linhas[i], size);
        DrawText(linhas[i], (sw - textW) / 2, y, size, LIGHTGRAY);
        y += 42;
    }

    const char *rodape = "Pressione ESC para voltar";
    int rodapeSize = 20;
    int rodapeW = MeasureText(rodape, rodapeSize);
    DrawText(rodape,
             (sw - rodapeW) / 2,
             sh - 60,
             rodapeSize,
             GRAY);
}

void Menu_DrawDerrota(void) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    ClearBackground((Color){10, 10, 10, 255});
    DrawRectangle(0, 0, sw, sh, (Color){80, 0, 0, 60});

    const char *titulo = "DERROTA";
    int tituloSize = 64;
    int tituloW    = MeasureText(titulo, tituloSize);
    DrawText(titulo, (sw - tituloW) / 2, sh / 3, tituloSize, (Color){200, 40, 40, 255});

    const char *sub = "o boss foi mais forte desta vez...";
    int subSize = 22;
    int subW    = MeasureText(sub, subSize);
    DrawText(sub, (sw - subW) / 2, sh / 3 + tituloSize + 20, subSize, LIGHTGRAY);

    const char *instrucao = "Pressione ENTER para voltar ao menu";
    int instrSize = 20;
    int instrW    = MeasureText(instrucao, instrSize);
    DrawText(instrucao, (sw - instrW) / 2, sh * 2 / 3, instrSize, (Color){160, 160, 160, 255});
}

void Menu_DrawAgradecimento(void) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    ClearBackground((Color){10, 10, 10, 255});

    DrawRectangle(0, 0, sw, sh, (Color){80, 60, 0, 40});

    const char *titulo = "VITORIA!";
    int tituloSize = 64;
    int tituloW    = MeasureText(titulo, tituloSize);
    DrawText(titulo, (sw - tituloW) / 2, sh / 4, tituloSize, GOLD);

    const char *sub1 = "Obrigado por jogar";
    int sub1Size = 28;
    int sub1W    = MeasureText(sub1, sub1Size);
    DrawText(sub1, (sw - sub1W) / 2, sh / 4 + tituloSize + 30, sub1Size, LIGHTGRAY);

    const char *sub2 = "Echos of the Cinders";
    int sub2Size = 28;
    int sub2W    = MeasureText(sub2, sub2Size);
    DrawText(sub2, (sw - sub2W) / 2, sh / 4 + tituloSize + 68, sub2Size, (Color){210, 90, 40, 255});

    const char *instrucao = "Pressione ENTER para voltar ao menu";
    int instrSize = 20;
    int instrW    = MeasureText(instrucao, instrSize);
    DrawText(instrucao, (sw - instrW) / 2, sh * 2 / 3, instrSize, (Color){160, 160, 160, 255});
}

void Menu_DrawRecordes(void)
{
    ClearBackground(BLACK);

    DrawText("MELHORES TEMPOS",
             180,
             40,
             40,
             GOLD);

    FILE *file = fopen("scores.txt", "r");

    if (file == NULL)
    {
        DrawText("Nenhum recorde salvo.",
                 220,
                 180,
                 24,
                 LIGHTGRAY);

        DrawText("ESC para voltar",
                 260,
                 500,
                 20,
                 GRAY);

        return;
    }

    char nome[4];
    float tempo;

    int y = 140;
    int posicao = 1;

    while (fscanf(file,
                  "%3s %f",
                  nome,
                  &tempo) == 2)
    {
        char linha[128];

        sprintf(linha,
                "%d. %s - %.2f segundos",
                posicao,
                nome,
                tempo);

        DrawText(linha,
                 180,
                 y,
                 28,
                 WHITE);

        y += 45;
        posicao++;

        if (posicao > 10)
            break;
    }

    fclose(file);

    DrawText("ESC para voltar",
             260,
             520,
             20,
             GRAY);
}