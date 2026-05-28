#ifndef DUMMY_H
#define DUMMY_H

#include "raylib.h"
#include "level.h"

typedef struct TrainingDummy {

    Vector2 pos;
    Vector2 tile_pos;

    float raio;

    int hp;

    bool alive;

    float hitFlashTimer;

} TrainingDummy;

void TrainingDummy_Init(TrainingDummy *dummy,
                        float tileX,
                        float tileY,
                        Level *level);

void TrainingDummy_TakeDamage(TrainingDummy *dummy,
                              int damage);

void TrainingDummy_Draw(TrainingDummy *dummy);

#endif
