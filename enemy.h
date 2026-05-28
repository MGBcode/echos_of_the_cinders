#ifndef ENEMY_H
#define ENEMY_H

#include "raylib.h"
#include <stdbool.h>
#include "player.h"
#include "level.h"

typedef struct AttackCircle {
    Vector2 center;
    float radius;
    int damage;
    bool active;
} AttackCircle;

typedef struct ProjectileNode {
    Vector2 pos;
    Vector2 dir;
    float speed;
    float radius;
    int damage;
    struct ProjectileNode *next;
} ProjectileNode;

typedef enum {
    BOSS_ATTACK_TYPE_LIGHT,
    BOSS_ATTACK_TYPE_HEAVY,
    BOSS_ATTACK_TYPE_BRUTAL,
    BOSS_ATTACK_TYPE_PROJECTILE,
    BOSS_ATTACK_TYPE_AOE_BURST
} BossAttackType;

typedef enum {
    BOSS_OBSERVE,
    BOSS_HUNT,
    BOSS_ATTACK,
    BOSS_COOLDOWN
} BossState;

typedef struct Boss {
    Vector2 pos;
    float raio;

    int hp;
    int hpMax;

    BossState state;
    BossAttackType attackType;
    BossAttackType lastAttackType;
    bool hasLastAttackType;
    int attackRepeatCount;
    float stateTimer;
    float sizeScale;
    float stateHoldTimer;
    float stateHoldMinObserve;
    float stateHoldMinHunt;

    float huntDashTimer;
    float huntDashDuration;
    float huntDashCooldown;
    bool huntDashActive;

    float aggroRadius;
    float observeSpeed;
    float huntSpeed;
    float observeDistance;
    float attackRange;
    float pursuitDistance;

    int phase;
    bool openingBrutalPending;
    bool aoe50Triggered;
    bool aoe15Triggered;

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


    float brutalWindup;
    float brutalActive;
    float brutalRecovery;
    float brutalHitRadius;
    float brutalLungeSpeed;
    int brutalDamage;

    float heavyWindup;
    float heavyActive;
    float heavyRecovery;
    float heavyHitRadius;
    float heavyForwardOffset;
    int   heavyDamage1;
    int   heavyDamage2;
    int   heavyMaxHits;
    int   heavyHitIndex;

    AttackCircle atk;
    Vector2 attackDir;
    Vector2 lockedTargetPos;
    bool hasHitPlayerThisAttack;

    
    float aoeWindup;
    float aoeActive;
    float aoeRecovery;
    float aoeRadius;
    int aoeDamage;

    ProjectileNode *projectilesHead;
    int projBurstShot;
    int projBurstTotal;
    float projBurstTimer;
    float projBurstInterval;

    bool projActive;
    Vector2 projPos;
    Vector2 projDir;
    float projSpeed;
    float projRadius;
    int projDamage;

} Boss;

void Boss_Init(Boss *b, Vector2 startPos, Level *level);

void Boss_Update(Boss *b,
                 float dt,
                 Player *player,
                 Level *level);

void Boss_Draw(const Boss *b);

bool Boss_GetAttackCircle(const Boss *b, AttackCircle *out);

// TrainingDummy API lives in dummy.h
#include "dummy.h"

#endif