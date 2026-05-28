#include "enemy.h"
#include "player.h"
#include "raymath.h"
#include <stdlib.h>
#include <stdio.h>

static float RandRange(float a, float b) {
    float t = (float)rand() / (float)RAND_MAX;
    return a + (b - a) * t;
}

static Vector2 SafeNormalize(Vector2 v) {
    float len = Vector2Length(v);
    if (len < 0.0001f) return (Vector2){1.0f, 0.0f};
    return Vector2Scale(v, 1.0f / len);
}

static bool Boss_CanUseProjectile(const Boss *b) {
    return b->hp <= (b->hpMax / 2);
}

static float GetPhaseWindup(const Boss *b, float baseWindup) {
    return (b->phase == 2) ? baseWindup * 0.75f : baseWindup;
}

static float GetAttackCooldown(const Boss *b, float baseCooldown) {
    float cooldown = baseCooldown * 0.85f;
    if (b->phase == 2) cooldown *= 0.85f;
    return cooldown;
}

static float GetProjectileRange(const Boss *b) {
    return b->attackRange * 1.35f;
}

static bool CanMoveCircle(Level *level, float x, float y, float r) {
    int tw = level->tamanho_tile;
    int th = level->tamanho_tile_h;

    int left   = (int)((x - r) / tw);
    int right  = (int)((x + r) / tw);
    int top    = (int)((y - r) / th);
    int bottom = (int)((y + r) / th);

    return level_pode_mover(level, left,  top)    &&
           level_pode_mover(level, right, top)    &&
           level_pode_mover(level, left,  bottom) &&
           level_pode_mover(level, right, bottom);
}

static void MoveWithCollision(Level *level, Vector2 *pos, float r, Vector2 delta) {
    float nx = pos->x + delta.x;
    float ny = pos->y + delta.y;

    if (delta.x != 0.0f) {
        if (CanMoveCircle(level, nx, pos->y, r)) pos->x = nx;
    }
    if (delta.y != 0.0f) {
        if (CanMoveCircle(level, pos->x, ny, r)) pos->y = ny;
    }
}

static ProjectileNode *ProjectileNode_Create(Vector2 pos, Vector2 dir, float speed, float radius, int damage) {
    ProjectileNode *node = (ProjectileNode *)malloc(sizeof(ProjectileNode));
    if (node == NULL) return NULL;
    node->pos = pos;
    node->dir = dir;
    node->speed = speed;
    node->radius = radius;
    node->damage = damage;
    node->next = NULL;
    return node;
}

static void ProjectileNode_AddToList(ProjectileNode **head, ProjectileNode *node) {
    if (node == NULL) return;
    node->next = *head;
    *head = node;
}

bool Boss_GetAttackCircle(const Boss *b, AttackCircle *out) {
    if (!b->atk.active) return false;
    if (out) *out = b->atk;
    return true;
}

void Boss_Init(Boss *b, Vector2 startPos, Level *level) {
    b->pos = startPos;
    b->sizeScale = level->tamanho_tile / 32.0f;
    b->raio = 28.0f * b->sizeScale;

    b->hpMax = 300;
    b->hp = b->hpMax;

    b->state = BOSS_OBSERVE;
    b->attackType = BOSS_ATTACK_TYPE_LIGHT;
    b->lastAttackType = BOSS_ATTACK_TYPE_LIGHT;
    b->hasLastAttackType = false;
    b->attackRepeatCount = 0;
    b->stateTimer = 1.2f;
    b->stateHoldTimer = 0.0f;
    b->stateHoldMinObserve = 0.85f;
    b->stateHoldMinHunt = 0.75f;
    b->huntDashTimer = 0.0f;
    b->huntDashDuration = 0.36f;
    b->huntDashCooldown = 2.4f;
    b->huntDashActive = false;

    b->sizeScale = 2.5f;
    b->aggroRadius = 260.0f;
    b->observeSpeed = 80.0f;
    b->huntSpeed = 340.0f;
    b->observeDistance = 260.0f;
    b->attackRange = 180.0f;
    b->pursuitDistance = 360.0f;
    b->openingBrutalPending = false;
    b->aoe50Triggered = false;
    b->aoe15Triggered = false;
    b->phase = 1;

    b->lightWindup = 0.60f;
    b->lightActive = 0.16f;
    b->lightRecovery = 0.24f;
    b->lightHitRadius = 42.0f * b->sizeScale;
    b->lightForwardOffset = 32.0f * b->sizeScale;
    b->lightDamage1 = 10;
    b->lightDamage2 = 10;
    b->lightDamage3 = 15;
    b->lightMaxHits = 3;
    b->lightHitIndex = 0;

    b->brutalWindup = 0.95f;
    b->brutalActive = 0.46f;
    b->brutalRecovery = 1.10f;
    b->brutalHitRadius = 68.0f * b->sizeScale;
    b->brutalLungeSpeed = 1560.0f;
    b->brutalDamage = 40;

    b->heavyWindup = 1.20f;
    b->heavyActive = 0.33f;
    b->heavyRecovery = 1.20f;
    b->heavyHitRadius = 90.0f * b->sizeScale;
    b->heavyForwardOffset = 52.0f * b->sizeScale;
    b->heavyDamage1 = 30;
    b->heavyDamage2 = 50;
    b->heavyMaxHits = 2;
    b->heavyHitIndex = 0;

    b->atk = (AttackCircle){0};
    b->attackDir = (Vector2){1, 0};
    b->lockedTargetPos = startPos;
    b->hasHitPlayerThisAttack = false;

    b->aoeWindup = 1.80f;
    b->aoeActive = 0.55f;
    b->aoeRecovery = 1.30f;
    b->aoeRadius = 198.0f * b->sizeScale;
    b->aoeDamage = 70;

    b->projectilesHead = NULL;
    b->projBurstShot = 0;
    b->projBurstTotal = 3;
    b->projBurstTimer = 0.0f;
    b->projBurstInterval = 0.30f;

    b->projActive = false;
    b->projPos = startPos;
    b->projDir = (Vector2){0,0};
    b->projSpeed = 640.0f;
    b->projRadius = 18.0f * b->sizeScale;
    b->projDamage = 20;
}

static void EnterObserve(Boss *b) {
    b->state = BOSS_OBSERVE;
    b->stateTimer = RandRange(1.0f, 1.6f);
    b->stateHoldTimer = b->stateHoldMinObserve;
    b->atk.active = false;
    b->projActive = false;
}

static void EnterHunt(Boss *b) {
    b->state = BOSS_HUNT;
    b->stateTimer = 0.0f;
    b->stateHoldTimer = b->stateHoldMinHunt;
    b->atk.active = false;
    b->projActive = false;
}

static BossAttackType SelectAttackByDistance(Boss *b, float dist) {
    float shortRange = b->attackRange * 0.65f;
    float midRange = b->attackRange * 0.90f;
    int roll = rand() % 100;
    bool allowProjectile = Boss_CanUseProjectile(b);

    if (dist <= shortRange) {
        if (roll < 60) return BOSS_ATTACK_TYPE_LIGHT;
        return BOSS_ATTACK_TYPE_HEAVY;
    }

    if (dist <= midRange) {
        if (roll < 60) return BOSS_ATTACK_TYPE_BRUTAL;
        return BOSS_ATTACK_TYPE_HEAVY;
    }

    if (allowProjectile && b->hasLastAttackType && dist >= GetProjectileRange(b)) {
        if (roll < 85) return BOSS_ATTACK_TYPE_PROJECTILE;
        return BOSS_ATTACK_TYPE_HEAVY;
    }

    if (roll < 50) return BOSS_ATTACK_TYPE_BRUTAL;
    return BOSS_ATTACK_TYPE_HEAVY;
}

static BossAttackType ForceDifferentAttackForDistance(Boss *b, float dist, BossAttackType desired) {
    if (!b->hasLastAttackType) {
        return desired;
    }

    if (desired != b->lastAttackType) {
        return desired;
    }

    float shortRange = b->attackRange * 0.65f;
    float midRange = b->attackRange * 0.90f;

    if (dist <= shortRange) {
        return (desired == BOSS_ATTACK_TYPE_LIGHT) ? BOSS_ATTACK_TYPE_HEAVY : BOSS_ATTACK_TYPE_LIGHT;
    }

    if (dist <= midRange) {
        return (desired == BOSS_ATTACK_TYPE_BRUTAL) ? BOSS_ATTACK_TYPE_HEAVY : BOSS_ATTACK_TYPE_BRUTAL;
    }

    if (Boss_CanUseProjectile(b) && b->hasLastAttackType && dist >= GetProjectileRange(b)) {
        return (desired == BOSS_ATTACK_TYPE_PROJECTILE) ? BOSS_ATTACK_TYPE_HEAVY : BOSS_ATTACK_TYPE_PROJECTILE;
    }

    return BOSS_ATTACK_TYPE_HEAVY;
}

static void StartAttack(Boss *b, BossAttackType attackType, Vector2 playerPos) {
    if ((!Boss_CanUseProjectile(b) || !b->hasLastAttackType) && attackType == BOSS_ATTACK_TYPE_PROJECTILE) {
        attackType = BOSS_ATTACK_TYPE_BRUTAL;
    }

    if (attackType != b->lastAttackType) {
        b->lastAttackType = attackType;
        b->attackRepeatCount = 1;
    } else if (b->attackRepeatCount == 0) {
        b->attackRepeatCount = 1;
    }

    b->hasLastAttackType = true;

    b->state = BOSS_ATTACK;
    b->attackType = attackType;
    b->stateTimer = 0.0f;
    b->lockedTargetPos = playerPos;
    b->attackDir = SafeNormalize(Vector2Subtract(playerPos, b->pos));
    b->atk.active = false;
    b->hasHitPlayerThisAttack = false;
    b->lightHitIndex = 0;
    b->heavyHitIndex = 0;
    b->projActive = false;
    b->projBurstShot = 0;
    b->projBurstTimer = 0.0f;
}

void Boss_Update(Boss *b, float dt, Player *player, Level *level) {
    if (b->hp <= 0) return;

    Vector2 playerPos = player->pos;
    float playerRadius = player->raio;

    int tw = level->tamanho_tile;
    int th = level->tamanho_tile_h;
    float tileAvg = (tw + th) * 0.5f;

    float bossR = tileAvg * 0.35f;
    if (bossR < 18.0f) bossR = 18.0f;
    bossR *= b->sizeScale;
    b->raio = bossR;

    float dist = Vector2Distance(b->pos, playerPos);
    float hpRatio = (float)b->hp / (float)b->hpMax;
    b->phase = (hpRatio > 0.5f) ? 1 : 2;

    {
        ProjectileNode **current = &b->projectilesHead;
        while (*current != NULL) {
            ProjectileNode *proj = *current;

            Vector2 toPlayer = Vector2Subtract(playerPos, proj->pos);
            Vector2 targetDir = (Vector2Length(toPlayer) > 0.0001f)
                ? Vector2Normalize(toPlayer)
                : proj->dir;

            proj->dir = Vector2Normalize(Vector2Lerp(proj->dir, targetDir, 2.5f * dt));
            proj->pos = Vector2Add(proj->pos, Vector2Scale(proj->dir, proj->speed * dt));

            if (CheckCollisionCircles(proj->pos, proj->radius, playerPos, playerRadius)) {
                printf("[HIT] Projectile acertou o player: %d HP\n", proj->damage);
                PlayerTakeDamage_IgnoreParry(player, proj->damage);
                *current = proj->next;
                free(proj);
                continue;
            }

            if (!CanMoveCircle(level, proj->pos.x, proj->pos.y, proj->radius)) {
                *current = proj->next;
                free(proj);
                continue;
            }

            current = &proj->next;
        }
    }

    if (!b->aoe50Triggered && hpRatio <= 0.5f) {
        b->aoe50Triggered = true;
        StartAttack(b, BOSS_ATTACK_TYPE_AOE_BURST, playerPos);
        return;
    }
    if (!b->aoe15Triggered && hpRatio <= 0.15f) {
        b->aoe15Triggered = true;
        StartAttack(b, BOSS_ATTACK_TYPE_AOE_BURST, playerPos);
        return;
    }

    switch (b->state) {
    case BOSS_OBSERVE: {
        b->stateTimer -= dt;
        if (b->stateHoldTimer > 0.0f) b->stateHoldTimer -= dt;
        Vector2 toPlayer = Vector2Subtract(playerPos, b->pos);
        Vector2 dirToPlayer = SafeNormalize(toPlayer);
        Vector2 lateral = (Vector2){-dirToPlayer.y, dirToPlayer.x};
        Vector2 move = {0};

        float observeSpeed = b->observeSpeed * ((b->phase == 2) ? 1.15f : 1.0f);
        float idealDistance = b->observeDistance;
        float distanceDelta = dist - idealDistance;
        if (fabsf(distanceDelta) > 30.0f) {
            float speed = observeSpeed * ((distanceDelta > 0.0f) ? 1.25f : 0.85f);
            move = Vector2Scale(dirToPlayer, speed * (distanceDelta > 0.0f ? 1.0f : -1.0f) * dt);
        } else {
            move = Vector2Scale(lateral, observeSpeed * 0.55f * dt);
        }
        MoveWithCollision(level, &b->pos, b->raio * 1.1f, move);

        if (dist > b->pursuitDistance + 28.0f && b->stateHoldTimer <= 0.0f) {
            EnterHunt(b);
        } else if (b->stateTimer <= 0.0f) {
            if (dist <= b->attackRange * 1.1f) {
                if (dist <= b->attackRange * 0.75f) {
                    BossAttackType desired = SelectAttackByDistance(b, dist);
                    StartAttack(b, ForceDifferentAttackForDistance(b, dist, desired), playerPos);
                } else {
                    BossAttackType desired = SelectAttackByDistance(b, dist);
                    StartAttack(b, ForceDifferentAttackForDistance(b, dist, desired), playerPos);
                }
            } else {
                EnterHunt(b);
            }
        }
    } break;

    case BOSS_HUNT: {
        b->stateTimer += dt;
        if (b->stateHoldTimer > 0.0f) b->stateHoldTimer -= dt;

        Vector2 dirToPlayer = SafeNormalize(Vector2Subtract(playerPos, b->pos));
        float targetSpeed = b->huntSpeed * ((b->phase == 2) ? 1.15f : 1.0f);
        float projectileRange = GetProjectileRange(b);
        if (b->huntDashActive) {
            targetSpeed *= 2.8f;
            b->huntDashDuration -= dt;
            if (b->huntDashDuration <= 0.0f) {
                b->huntDashActive = false;
                b->huntDashDuration = 0.36f;
            }
        } else {
            b->huntDashTimer -= dt;
            if (b->huntDashTimer <= 0.0f && dist > b->attackRange * 0.85f && rand() % 3 == 0) {
                b->huntDashActive = true;
                b->huntDashTimer = b->huntDashCooldown;
            }
        }

        Vector2 delta = Vector2Scale(dirToPlayer, targetSpeed * dt);
        MoveWithCollision(level, &b->pos, b->raio * 1.1f, delta);

        if (b->phase == 2 && Boss_CanUseProjectile(b) && b->hasLastAttackType && dist >= projectileRange && b->stateHoldTimer <= 0.0f) {
            BossAttackType desired = SelectAttackByDistance(b, dist);
            StartAttack(b, ForceDifferentAttackForDistance(b, dist, desired), playerPos);
        } else if (dist <= b->attackRange) {
            BossAttackType desired = SelectAttackByDistance(b, dist);
            StartAttack(b, ForceDifferentAttackForDistance(b, dist, desired), playerPos);
        } else if (dist <= b->observeDistance - 28.0f && b->stateHoldTimer <= 0.0f) {
            EnterObserve(b);
        }
    } break;

    case BOSS_ATTACK: {
        b->stateTimer += dt;
        float windupEnd = 0.0f;
        float activeEnd = 0.0f;
        float totalEnd = 0.0f;
        float cooldown = 1.0f;

        switch (b->attackType) {
        case BOSS_ATTACK_TYPE_LIGHT: {
            float baseWindup = GetPhaseWindup(b, b->lightWindup);
            float perHit = b->lightWindup + b->lightActive + b->lightRecovery;
            int currentHit = (int)(b->stateTimer / perHit);
            if (currentHit >= b->lightMaxHits) currentHit = b->lightMaxHits - 1;
            float localT = fmodf(b->stateTimer, perHit);
            float prevTimer = b->stateTimer - dt;
            float localPrevT = (prevTimer > 0.0f) ? fmodf(prevTimer, perHit) : 0.0f;
            windupEnd = (currentHit == 0) ? baseWindup : 0.30f;
            activeEnd = windupEnd + b->lightActive;
            totalEnd = perHit * b->lightMaxHits;
            cooldown = GetAttackCooldown(b, 0.95f);

            if (currentHit != b->lightHitIndex) {
                b->lightHitIndex = currentHit;
                b->hasHitPlayerThisAttack = false;
            }

            if (localT < windupEnd) {
                b->lockedTargetPos = playerPos;
                b->attackDir = SafeNormalize(Vector2Subtract(playerPos, b->pos));
            }

            int currentDamage = (currentHit == 0) ? b->lightDamage1 : (currentHit == 1) ? b->lightDamage2 : b->lightDamage3;
            if (currentHit < b->lightMaxHits && localT >= windupEnd && localT < activeEnd) {
                if (localPrevT < windupEnd) {
                    Vector2 advance = Vector2Scale(b->attackDir, 220.0f * dt);
                    MoveWithCollision(level, &b->pos, b->raio * 1.1f, advance);
                }
                if (!b->hasHitPlayerThisAttack) {
                    b->atk.active = true;
                    b->atk.damage = currentDamage;
                    b->atk.radius = b->lightHitRadius;
                    b->atk.center = Vector2Add(b->pos, Vector2Scale(b->attackDir, b->lightForwardOffset));
                    if (CheckCollisionCircles(b->atk.center, b->atk.radius, playerPos, playerRadius)) {
                        b->hasHitPlayerThisAttack = true;
                        if (player->isParrying) {
                            b->atk.active = false;
                            b->state = BOSS_COOLDOWN;
                            b->stateTimer = GetAttackCooldown(b, 0.95f) * 1.75f;
                            printf("[PARRY] Light Attack aparado! Boss em cooldown prolongado.\n");
                        } else {
                            PlayerTakeDamage(player, b->atk.damage);
                            printf("[HIT] Leve acertou o player: %d HP\n", currentDamage);
                        }
                    }
                }
            } else {
                b->atk.active = false;
            }
        } break;

        case BOSS_ATTACK_TYPE_HEAVY: {
            float baseWindup = GetPhaseWindup(b, b->heavyWindup);
            float hit1Windup = baseWindup;
            float hit1Active = b->heavyActive;
            float hit1Recovery = b->heavyRecovery;
            float dashDuration = 0.25f;
            float hit2Windup = baseWindup * 0.25f;
            float hit2Active = b->heavyActive;
            float hit2Recovery = b->heavyRecovery;

            float hit1End = hit1Windup + hit1Active + hit1Recovery;
            float dashEnd = hit1End + dashDuration;
            float hit2End = dashEnd + hit2Windup + hit2Active + hit2Recovery;
            totalEnd = hit2End;
            cooldown = GetAttackCooldown(b, 1.35f);

            float prevTimer = b->stateTimer - dt;

            if (b->stateTimer < hit1End) {
                if (b->stateTimer < hit1Windup) {
                    b->attackDir = SafeNormalize(Vector2Subtract(playerPos, b->pos));
                }

                if (b->stateTimer >= hit1Windup && b->stateTimer < hit1Windup + hit1Active) {
                    Vector2 advance = Vector2Scale(b->attackDir, 220.0f * dt);
                    MoveWithCollision(level, &b->pos, b->raio * 1.1f, advance);
                    b->atk.active = true;
                    b->atk.damage = b->heavyDamage1;
                    b->atk.radius = b->heavyHitRadius;
                    b->atk.center = Vector2Add(b->pos, Vector2Scale(b->attackDir, b->heavyForwardOffset));
                    if (!b->hasHitPlayerThisAttack && CheckCollisionCircles(b->atk.center, b->atk.radius, playerPos, playerRadius)) {
                        b->hasHitPlayerThisAttack = true;
                        if (player->isParrying) {
                            b->atk.active = false;
                            b->state = BOSS_COOLDOWN;
                            b->stateTimer = GetAttackCooldown(b, 1.35f) * 1.75f;
                            printf("[PARRY] Heavy 1/2 aparado! Boss em cooldown prolongado.\n");
                        } else {
                            PlayerTakeDamage(player, b->atk.damage);
                            printf("[HIT] Heavy 1/2 acertou o player: %d HP\n", b->heavyDamage1);
                        }
                    }
                } else {
                    b->atk.active = false;
                }
            } else if (b->stateTimer < dashEnd) {
                if (prevTimer < hit1End) {
                    b->hasHitPlayerThisAttack = false;
                }

                Vector2 dirToPlayer = SafeNormalize(Vector2Subtract(playerPos, b->pos));
                b->attackDir = dirToPlayer;
                Vector2 dashDelta = Vector2Scale(b->attackDir, 1400.0f * dt);
                float remaining = Vector2Distance(b->pos, playerPos) - (b->raio + playerRadius + 20.0f);
                if (remaining > 0.0f) {
                    float step = Vector2Length(dashDelta);
                    if (step > remaining) {
                        dashDelta = Vector2Scale(b->attackDir, remaining);
                    }
                    MoveWithCollision(level, &b->pos, b->raio * 1.1f, dashDelta);
                }
                b->atk.active = false;
            } else if (b->stateTimer < hit2End) {
                float localT = b->stateTimer - dashEnd;
                if (localT < hit2Windup) {
                    b->attackDir = SafeNormalize(Vector2Subtract(playerPos, b->pos));
                }

                if (localT >= hit2Windup && localT < hit2Windup + hit2Active) {
                    Vector2 advance = Vector2Scale(b->attackDir, 320.0f * dt);
                    MoveWithCollision(level, &b->pos, b->raio * 1.1f, advance);
                    b->atk.active = true;
                    b->atk.damage = b->heavyDamage2;
                    b->atk.radius = b->heavyHitRadius;
                    b->atk.center = Vector2Add(b->pos, Vector2Scale(b->attackDir, b->heavyForwardOffset));
                    if (!b->hasHitPlayerThisAttack && CheckCollisionCircles(b->atk.center, b->atk.radius, playerPos, playerRadius)) {
                        b->hasHitPlayerThisAttack = true;
                        if (player->isParrying) {
                            b->atk.active = false;
                            b->state = BOSS_COOLDOWN;
                            b->stateTimer = GetAttackCooldown(b, 1.35f) * 1.75f;
                            printf("[PARRY] Heavy 2/2 aparado! Boss em cooldown prolongado.\n");
                        } else {
                            PlayerTakeDamage(player, b->atk.damage);
                            printf("[HIT] Heavy 2/2 acertou o player: %d HP\n", b->heavyDamage2);
                        }
                    }
                } else {
                    b->atk.active = false;
                }
            } else {
                b->atk.active = false;
            }

            if (b->stateTimer >= totalEnd) {
                b->atk.active = false;
                b->state = BOSS_COOLDOWN;
                b->stateTimer = cooldown;
            }
        } break;

        case BOSS_ATTACK_TYPE_BRUTAL: {
            windupEnd = GetPhaseWindup(b, b->brutalWindup);
            activeEnd = windupEnd + b->brutalActive;
            totalEnd = windupEnd + b->brutalActive + b->brutalRecovery;
            cooldown = GetAttackCooldown(b, 1.15f);

            if (b->stateTimer >= windupEnd && b->stateTimer < activeEnd) {
                Vector2 delta = Vector2Scale(b->attackDir, b->brutalLungeSpeed * dt);
                float safeGap = b->raio + playerRadius + 14.0f;
                float remaining = Vector2Distance(b->pos, playerPos) - safeGap;

                if (remaining <= 0.0f) {
                    b->stateTimer = activeEnd;
                    b->atk.active = false;
                } else {
                    float step = Vector2Length(delta);
                    if (step > remaining) {
                        delta = Vector2Scale(SafeNormalize(delta), remaining);
                    }

                    MoveWithCollision(level, &b->pos, b->raio * 1.1f, delta);
                }
            }
            if (b->stateTimer >= windupEnd && b->stateTimer < activeEnd) {
                b->atk.active = true;
                b->atk.damage = b->brutalDamage;
                b->atk.radius = b->brutalHitRadius;
                b->atk.center = Vector2Add(b->pos, Vector2Scale(b->attackDir, b->raio + b->atk.radius * 0.6f));
            } else {
                b->atk.active = false;
            }
            if (b->atk.active && !b->hasHitPlayerThisAttack) {
                if (CheckCollisionCircles(b->atk.center, b->atk.radius, playerPos, playerRadius)) {
                    b->hasHitPlayerThisAttack = true;
                    if (player->isParrying) {
                        b->atk.active = false;
                        b->state = BOSS_COOLDOWN;
                        b->stateTimer = GetAttackCooldown(b, 1.15f) * 1.75f;
                        printf("[PARRY] Brutal Attack aparado! Boss em cooldown prolongado.\n");
                    } else {
                        PlayerTakeDamage(player, b->atk.damage);
                        printf("[HIT] Brutal acertou o player: %d HP\n", b->brutalDamage);
                    }
                }
            }
        } break;

        case BOSS_ATTACK_TYPE_PROJECTILE: {
            windupEnd = GetPhaseWindup(b, 0.80f);
            activeEnd = windupEnd + b->projBurstInterval * (b->projBurstTotal - 1) + 1.10f;
            totalEnd = activeEnd;
            cooldown = GetAttackCooldown(b, 1.05f);

            if (b->stateTimer < windupEnd) {
                b->projBurstTimer = 0.0f;
                b->atk.active = false;
            } else {
                if (b->projBurstShot < b->projBurstTotal) {
                    b->projBurstTimer -= dt;
                    if (b->projBurstTimer <= 0.0f) {
                        b->lockedTargetPos = playerPos;
                        b->attackDir = SafeNormalize(Vector2Subtract(playerPos, b->pos));
                        b->projBurstShot += 1;
                        b->projBurstTimer = b->projBurstInterval;

                        float projRadius = 11.0f * b->sizeScale * ((b->phase == 2) ? 1.15f : 1.0f);
                        float projSpeed = 320.0f * ((b->phase == 2) ? 1.55f : 1.0f);
                        Vector2 projPos = Vector2Add(b->pos, Vector2Scale(b->attackDir, b->raio + projRadius + 4.0f));

                        ProjectileNode *newProj = ProjectileNode_Create(
                            projPos,
                            b->attackDir,
                            projSpeed,
                            projRadius,
                            20
                        );
                        if (newProj != NULL) {
                            ProjectileNode_AddToList(&b->projectilesHead, newProj);
                        }
                    }
                }
            }
        } break;

        case BOSS_ATTACK_TYPE_AOE_BURST: {
            windupEnd = GetPhaseWindup(b, b->aoeWindup);
            activeEnd = windupEnd + b->aoeActive;
            totalEnd = windupEnd + b->aoeActive + b->aoeRecovery;
            cooldown = GetAttackCooldown(b, 1.4f);

            if (b->stateTimer >= windupEnd && b->stateTimer < activeEnd) {
                b->atk.active = true;
                b->atk.center = b->pos;
                b->atk.radius = b->aoeRadius;
                b->atk.damage = b->aoeDamage;
            } else {
                b->atk.active = false;
            }
            if (b->atk.active && !b->hasHitPlayerThisAttack) {
                if (CheckCollisionCircles(b->atk.center, b->atk.radius, playerPos, playerRadius)) {
                    b->hasHitPlayerThisAttack = true;
                    PlayerTakeDamage_IgnoreParry(player, b->atk.damage);
                    printf("[HIT] AoE Burst acertou o player: %d HP\n", b->aoeDamage);
                }
            }
        } break;

        default: break;
        }

        if (b->attackType == BOSS_ATTACK_TYPE_PROJECTILE) {
            if (b->stateTimer >= totalEnd) {
                b->atk.active = false;
                b->projActive = false;
                EnterHunt(b);
                return;
            }
        } else if (b->attackType != BOSS_ATTACK_TYPE_BRUTAL) {
            if (b->stateTimer >= totalEnd) {
                b->atk.active = false;
                b->projActive = false;
                b->state = BOSS_COOLDOWN;
                b->stateTimer = cooldown;
            }
        } else {
            if (b->stateTimer >= totalEnd) {
                b->atk.active = false;
                b->state = BOSS_COOLDOWN;
                b->stateTimer = cooldown;
            }
        }
    } break;

    case BOSS_COOLDOWN: {
        b->stateTimer -= dt;
        if (b->stateTimer <= 0.0f) {
            EnterObserve(b);
        }
    } break;

    default: break;
    }
}

void Boss_Draw(const Boss *b) {
    if (b->hp <= 0) {
        DrawText("BOSS DEAD", GetScreenWidth() - 140, 20, 20, RED);
        return;
    }

    Color col = (Color){ 160, 70, 40, 255 };
    bool drawWindupTelegraph = false;
    bool drawActiveTelegraph = false;
    Vector2 teleCenter = b->atk.center;
    float teleRadius = b->atk.radius;
    Color teleColor = YELLOW;

    if (b->state == BOSS_OBSERVE) {
        col = (b->phase == 2) ? (Color){150, 80, 190, 255} : (Color){120, 80, 60, 255};
    } else if (b->state == BOSS_HUNT) {
        col = (b->phase == 2) ? (Color){170, 90, 220, 255} : (Color){180, 90, 40, 255};
    } else if (b->state == BOSS_ATTACK) {
        bool inWindup = false;
        switch (b->attackType) {
        case BOSS_ATTACK_TYPE_LIGHT: {
            float perHit = b->lightWindup + b->lightActive + b->lightRecovery;
            int currentHit = (int)(b->stateTimer / perHit);
            if (currentHit >= b->lightMaxHits) currentHit = b->lightMaxHits - 1;
            float localT = fmodf(b->stateTimer, perHit);
            float windupEnd = (currentHit == 0) ? b->lightWindup : 0.30f;
            float activeEnd = windupEnd + b->lightActive;
            teleRadius = b->lightHitRadius;
            teleCenter = Vector2Add(b->pos, Vector2Scale(b->attackDir, b->lightForwardOffset));
            if (localT < windupEnd) {
                inWindup = true;
                drawWindupTelegraph = true;
                teleColor = (Color){240, 180, 40, 180};
            } else if (localT < activeEnd) {
                drawActiveTelegraph = true;
                teleColor = (Color){220, 40, 40, 180};
            }
            col = inWindup ? YELLOW : GOLD;
            break;
        }
        case BOSS_ATTACK_TYPE_HEAVY: {
            float perHit = b->heavyWindup + b->heavyActive + b->heavyRecovery;
            int currentHit = (int)(b->stateTimer / perHit);
            if (currentHit >= b->heavyMaxHits) currentHit = b->heavyMaxHits - 1;
            float localT = fmodf(b->stateTimer, perHit);
            float windupEnd = (currentHit == 0) ? b->heavyWindup : b->heavyWindup * 0.7f;
            float activeEnd = windupEnd + b->heavyActive;
            teleRadius = b->heavyHitRadius;
            teleCenter = Vector2Add(b->pos, Vector2Scale(b->attackDir, b->heavyForwardOffset));
            if (localT < windupEnd) {
                inWindup = true;
                drawWindupTelegraph = true;
                teleColor = (Color){220, 140, 30, 180};
            } else if (localT < activeEnd) {
                drawActiveTelegraph = true;
                teleColor = (Color){200, 30, 30, 180};
            }
            col = inWindup ? ORANGE : (Color){200, 120, 60, 255};
            break;
        }
        case BOSS_ATTACK_TYPE_BRUTAL: {
            float windupEnd = b->brutalWindup;
            float activeEnd = b->brutalWindup + b->brutalActive;
            teleRadius = b->brutalHitRadius;
            teleCenter = Vector2Add(b->pos, Vector2Scale(b->attackDir, b->raio + teleRadius * 0.6f));
            if (b->stateTimer < windupEnd) {
                inWindup = true;
                drawWindupTelegraph = true;
                teleColor = (Color){220, 120, 30, 180};
            } else if (b->stateTimer < activeEnd) {
                drawActiveTelegraph = true;
                teleColor = (Color){200, 40, 40, 180};
            }
            col = inWindup ? ORANGE : RED;
            break;
        }
        case BOSS_ATTACK_TYPE_PROJECTILE: {
            float windupEnd = 0.80f;
            float projRadius = 11.0f * b->sizeScale * ((b->phase == 2) ? 1.15f : 1.0f);
            teleRadius = projRadius;
            teleCenter = Vector2Add(b->pos, Vector2Scale(b->attackDir, b->raio + teleRadius + 4.0f));
            if (b->stateTimer < windupEnd) {
                inWindup = true;
                drawWindupTelegraph = true;
                teleColor = (Color){240, 220, 40, 170};
            }
            col = inWindup ? YELLOW : GOLD;
            break;
        }
        case BOSS_ATTACK_TYPE_AOE_BURST: {
            float windupEnd = b->aoeWindup;
            float activeEnd = b->aoeWindup + b->aoeActive;
            teleRadius = b->aoeRadius;
            teleCenter = b->pos;
            if (b->stateTimer < windupEnd) {
                inWindup = true;
                drawWindupTelegraph = true;
                teleColor = (Color){180, 40, 40, 170};
            } else if (b->stateTimer < activeEnd) {
                drawActiveTelegraph = true;
                teleColor = (Color){200, 50, 50, 200};
            }
            col = inWindup ? (Color){120, 20, 20, 255} : RED;
            break;
        }
        default:
            col = RED;
            break;
        }
        if (inWindup) col.a = 230;
    } else if (b->state == BOSS_COOLDOWN) {
        col = (Color){80, 80, 120, 255};
    }

    DrawCircleV(b->pos, b->raio, col);

    if (drawWindupTelegraph) {
        DrawCircleLines((int)teleCenter.x, (int)teleCenter.y, teleRadius, teleColor);
    }
    if (drawActiveTelegraph) {
        DrawCircle((int)teleCenter.x, (int)teleCenter.y, teleRadius, teleColor);
    }

    ProjectileNode *proj = b->projectilesHead;
    while (proj != NULL) {
        DrawCircleV(proj->pos, proj->radius, RED);
        proj = proj->next;
    }
}
