#ifndef ENEMY_H
#define ENEMY_H

#include "raylib.h"
#include <stdbool.h>
#include "level.h"

typedef enum {
    BOSS_OBSERVE = 0,
    BOSS_HUNT,
    BOSS_ATTACK,
    BOSS_COOLDOWN
} BossState;

typedef enum {
    BOSS_ATTACK_TYPE_LIGHT = 0,
    BOSS_ATTACK_TYPE_HEAVY,
    BOSS_ATTACK_TYPE_BRUTAL,
    BOSS_ATTACK_TYPE_PROJECTILE,
    BOSS_ATTACK_TYPE_AOE_BURST
} BossAttackType;

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
    BossAttackType attackType;
    BossAttackType lastAttackType;
    int attackRepeatCount;
    float stateTimer;

    float sizeScale;
    float aggroRadius;
    float observeSpeed;
    float huntSpeed;
    float observeDistance;
    float attackRange;
    float pursuitDistance;
    bool openingBrutalPending;
    bool aoe50Triggered;
    bool aoe15Triggered;

    // Phase helper
    int phase; // 1 ou 2

    // State transition helpers
    float stateHoldTimer;
    float stateHoldMinObserve;
    float stateHoldMinHunt;
    float huntDashTimer;
    float huntDashDuration;
    float huntDashCooldown;
    bool  huntDashActive;

    // Light attack
    float lightWindup;
    float lightActive;
    float lightRecovery;
    float lightHitRadius;
    float lightForwardOffset;
    int   lightDamage1;
    int   lightDamage2;
    int   lightDamage3;
    int   lightMaxHits;
    int   lightHitIndex;

    // Brutal attack
    float brutalWindup;
    float brutalActive;
    float brutalRecovery;
    float brutalHitRadius;
    float brutalLungeSpeed;
    int   brutalDamage;

    // Heavy attack
    float heavyWindup;
    float heavyActive;
    float heavyRecovery;
    float heavyHitRadius;
    float heavyForwardOffset;
    int   heavyDamage1;
    int   heavyDamage2;
    int   heavyMaxHits;
    int   heavyHitIndex;

    Vector2 attackDir;
    Vector2 lockedTargetPos;

    AttackCircle atk;
    bool hasHitPlayerThisAttack;

    // AoE burst
    float aoeWindup;
    float aoeActive;
    float aoeRecovery;
    float aoeRadius;
    int   aoeDamage;

    // Projetil
    bool  projActive;
    Vector2 projPos;
    Vector2 projDir;
    float projSpeed;
    float projRadius;
    int   projDamage;
} Boss;

void Boss_Init(Boss *b, Vector2 startPos);
void Boss_Update(Boss *b, float dt, Vector2 playerPos, float playerRadius, Level *level);
void Boss_Draw(const Boss *b);

// retorna true se a hitbox do boss está ativa
bool Boss_GetAttackCircle(const Boss *b, AttackCircle *out);

#endif