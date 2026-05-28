#include "dummy.h"
#include <stdio.h>

void TrainingDummy_Init(TrainingDummy *dummy,
                        float tileX,
                        float tileY,
                        Level *level)
{
    dummy->tile_pos = (Vector2){tileX, tileY};
    dummy->pos.x = tileX * level->tamanho_tile;
    dummy->pos.y = tileY * level->tamanho_tile_h;
    dummy->raio = level->tamanho_tile * 0.55f;

    dummy->hp = 999999;

    dummy->alive = true;

    dummy->hitFlashTimer = 0.0f;
}

void TrainingDummy_TakeDamage(TrainingDummy *dummy,
                              int damage)
{
    dummy->hp -= damage;

    dummy->hitFlashTimer = 0.15f;
}

void TrainingDummy_Draw(TrainingDummy *dummy)
{
    Color color =
        (dummy->hitFlashTimer > 0.0f)
        ? YELLOW
        : GRAY;

    DrawCircleV(dummy->pos,
                dummy->raio,
                color);

    DrawText("DUMMY",
             dummy->pos.x - 35,
             dummy->pos.y - 55,
             20,
             LIGHTGRAY);
}
