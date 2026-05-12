#ifndef ENEMY_H
#define ENEMY_H

#include "raylib.h"
#include <stdbool.h>
#include "level.h"

typedef enum {
    BOSS_WANDER = 0,
    BOSS_ALERT,
    BOSS_ATTACK_BRUTAL,
    BOSS_COMBAT,
    BOSS_ATTACK_LIGHT,
    BOSS_COOLDOWN
} BossState;

typedef struct {
    Vector2 center;
    float radius;
    int damage;
    bool active;
} AttackCircle;

typedef struct {
    Vector2 pos;
    float raio;

    int hp;
    int hpMax;

    BossState state;
    float stateTimer;

    float aggroRadius;
    bool openingBrutalPending;

    Vector2 wanderDir;
    float wanderSpeed;
    float wanderChangeTimer;

    float combatSpeed;

    // Light attack
    float lightWindup;
    float lightActive;
    float lightRecovery;
    float lightHitRadius;
    float lightForwardOffset;
    int   lightDamage;

    // Brutal attack
    float brutalWindup;
    float brutalActive;
    float brutalRecovery;
    float brutalHitRadius;
    float brutalLungeSpeed;
    int   brutalDamage;

    Vector2 attackDir;
    Vector2 lockedTargetPos;

    AttackCircle atk;
    bool hasHitPlayerThisAttack;
} Boss;

void Boss_Init(Boss *b, Vector2 startPos);
void Boss_Update(Boss *b, float dt, Vector2 playerPos, float playerRadius, Level *level);
void Boss_Draw(const Boss *b);

// retorna true se a hitbox do boss está ativa
bool Boss_GetAttackCircle(const Boss *b, AttackCircle *out);

#endif