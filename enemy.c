#include "enemy.h"
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

static float GetPhaseWindup(const Boss *b, float baseWindup) {
    return (b->phase == 2) ? baseWindup * 0.75f : baseWindup;
}

static float GetAttackCooldown(const Boss *b, float baseCooldown) {
    float cooldown = baseCooldown * 0.85f; // agressividade geral
    if (b->phase == 2) cooldown *= 0.85f;   // fase 2 reduz ainda mais a punição
    return cooldown;
}

// Colisão em tiles (igual ao player, mas para o boss)
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

// Move com colisão separando X e Y
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

bool Boss_GetAttackCircle(const Boss *b, AttackCircle *out) {
    if (!b->atk.active) return false;
    if (out) *out = b->atk;
    return true;
}

// -------------------- boss logic --------------------
void Boss_Init(Boss *b, Vector2 startPos) {
    b->pos = startPos;
    b->sizeScale = 2.5f;
    b->raio = 28.0f * b->sizeScale;

    b->hpMax = 200;
    b->hp = b->hpMax;

    b->state = BOSS_OBSERVE;
    b->attackType = BOSS_ATTACK_TYPE_LIGHT;
    b->lastAttackType = BOSS_ATTACK_TYPE_LIGHT;
    b->attackRepeatCount = 0;
    b->stateTimer = 1.2f;
    b->stateHoldTimer = 0.0f;
    b->stateHoldMinObserve = 0.85f;
    b->stateHoldMinHunt = 0.75f;
    b->huntDashTimer = 0.0f;
    b->huntDashDuration = 0.18f;
    b->huntDashCooldown = 2.4f;
    b->huntDashActive = false;

    b->sizeScale = 2.5f;
    b->aggroRadius = 260.0f;
    b->observeSpeed = 40.0f;
    b->huntSpeed = 170.0f;
    b->observeDistance = 260.0f;
    b->attackRange = 180.0f;
    b->pursuitDistance = 360.0f;
    b->openingBrutalPending = false;
    b->aoe50Triggered = false;
    b->aoe15Triggered = false;
    b->phase = 1;

    // Light attack
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

    // Brutal attack
    b->brutalWindup = 0.95f;
    b->brutalActive = 0.46f;
    b->brutalRecovery = 1.10f;
    b->brutalHitRadius = 68.0f * b->sizeScale;
    b->brutalLungeSpeed = 780.0f;
    b->brutalDamage = 40;

    // Heavy attack
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

    // AoE burst
    b->aoeWindup = 1.80f;
    b->aoeActive = 0.55f;
    b->aoeRecovery = 1.30f;
    b->aoeRadius = 180.0f * b->sizeScale;
    b->aoeDamage = 70;

    // Projectile
    b->projActive = false;
    b->projPos = startPos;
    b->projDir = (Vector2){0,0};
    b->projSpeed = 640.0f;
    b->projRadius = 14.0f * b->sizeScale;
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

static BossAttackType ChooseAttack(Boss *b, BossAttackType attackType, BossAttackType alternateType) {
    if (attackType == b->lastAttackType) {
        if (b->attackRepeatCount >= 2) {
            attackType = alternateType;
        }
    }

    if (attackType == b->lastAttackType) {
        b->attackRepeatCount++;
    } else {
        b->lastAttackType = attackType;
        b->attackRepeatCount = 1;
    }
    return attackType;
}

static BossAttackType SelectAttackByDistance(Boss *b, float dist) {
    float shortRange = b->attackRange * 0.65f;
    float midRange = b->attackRange * 0.90f;
    int roll = rand() % 100;

    if (dist <= shortRange) {
        if (roll < 65) return BOSS_ATTACK_TYPE_LIGHT;
        if (roll < 90) return BOSS_ATTACK_TYPE_HEAVY;
        return BOSS_ATTACK_TYPE_LIGHT;
    }

    if (dist <= midRange) {
        if (roll < 55) return BOSS_ATTACK_TYPE_BRUTAL;
        if (roll < 85) return BOSS_ATTACK_TYPE_HEAVY;
        return (b->phase == 2) ? BOSS_ATTACK_TYPE_PROJECTILE : BOSS_ATTACK_TYPE_BRUTAL;
    }

    // Long distance, only attack if player entrou no alcance justo ou phase 2 projectile punishes
    if (b->phase == 2) {
        if (roll < 70) return BOSS_ATTACK_TYPE_PROJECTILE;
        return BOSS_ATTACK_TYPE_BRUTAL;
    }
    return BOSS_ATTACK_TYPE_BRUTAL;
}

static void StartAttack(Boss *b, BossAttackType attackType, Vector2 playerPos) {
    if (attackType != b->lastAttackType) {
        b->lastAttackType = attackType;
        b->attackRepeatCount = 1;
    } else if (b->attackRepeatCount == 0) {
        b->attackRepeatCount = 1;
    }

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
}

void Boss_Update(Boss *b, float dt, Vector2 playerPos, float playerRadius, Level *level) {
    if (b->hp <= 0) return;

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
                        BossAttackType alternate = (desired == BOSS_ATTACK_TYPE_LIGHT) ? BOSS_ATTACK_TYPE_HEAVY : BOSS_ATTACK_TYPE_LIGHT;
                        StartAttack(b, ChooseAttack(b, desired, alternate), playerPos);
                    } else {
                        BossAttackType desired = SelectAttackByDistance(b, dist);
                        BossAttackType alternate = (desired == BOSS_ATTACK_TYPE_PROJECTILE) ? BOSS_ATTACK_TYPE_BRUTAL : BOSS_ATTACK_TYPE_PROJECTILE;
                        StartAttack(b, ChooseAttack(b, desired, alternate), playerPos);
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
            if (b->huntDashActive) {
                targetSpeed *= 2.8f;
                b->huntDashDuration -= dt;
                if (b->huntDashDuration <= 0.0f) {
                    b->huntDashActive = false;
                    b->huntDashDuration = 0.18f;
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

            if (dist <= b->attackRange) {
                BossAttackType desired = SelectAttackByDistance(b, dist);
                BossAttackType alternate = (desired == BOSS_ATTACK_TYPE_PROJECTILE) ? BOSS_ATTACK_TYPE_BRUTAL : BOSS_ATTACK_TYPE_PROJECTILE;
                if (desired == BOSS_ATTACK_TYPE_LIGHT || desired == BOSS_ATTACK_TYPE_HEAVY) {
                    alternate = (desired == BOSS_ATTACK_TYPE_LIGHT) ? BOSS_ATTACK_TYPE_HEAVY : BOSS_ATTACK_TYPE_LIGHT;
                }
                StartAttack(b, ChooseAttack(b, desired, alternate), playerPos);
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
                                printf("[HIT] Leve acertou o player: %d HP\n", currentDamage);
                            }
                        }
                    } else {
                        b->atk.active = false;
                    }
                } break;

                case BOSS_ATTACK_TYPE_HEAVY: {
                    float baseWindup = GetPhaseWindup(b, b->heavyWindup);
                    float perHit = b->heavyWindup + b->heavyActive + b->heavyRecovery;
                    int currentHit = (int)(b->stateTimer / perHit);
                    if (currentHit >= b->heavyMaxHits) currentHit = b->heavyMaxHits - 1;
                    float localT = fmodf(b->stateTimer, perHit);
                    float prevTimer = b->stateTimer - dt;
                    float localPrevT = (prevTimer > 0.0f) ? fmodf(prevTimer, perHit) : 0.0f;
                    windupEnd = (currentHit == 0) ? baseWindup : b->heavyWindup * 0.7f;
                    activeEnd = windupEnd + b->heavyActive;
                    totalEnd = perHit * b->heavyMaxHits;
                    cooldown = GetAttackCooldown(b, 1.35f);

                    if (currentHit != b->heavyHitIndex) {
                        b->heavyHitIndex = currentHit;
                        b->hasHitPlayerThisAttack = false;
                    }

                    if (localT < windupEnd) {
                        b->lockedTargetPos = playerPos;
                        b->attackDir = SafeNormalize(Vector2Subtract(playerPos, b->pos));
                    }

                    int currentDamage = (currentHit == 0) ? b->heavyDamage1 : b->heavyDamage2;
                    if (currentHit < b->heavyMaxHits && localT >= windupEnd && localT < activeEnd) {
                        if (localPrevT < windupEnd) {
                            Vector2 advance = Vector2Scale(b->attackDir, 220.0f * dt);
                            MoveWithCollision(level, &b->pos, b->raio * 1.1f, advance);
                        }
                        if (!b->hasHitPlayerThisAttack) {
                            b->atk.active = true;
                            b->atk.damage = currentDamage;
                            b->atk.radius = b->heavyHitRadius;
                            b->atk.center = Vector2Add(b->pos, Vector2Scale(b->attackDir, b->heavyForwardOffset));
                            if (CheckCollisionCircles(b->atk.center, b->atk.radius, playerPos, playerRadius)) {
                                b->hasHitPlayerThisAttack = true;
                                printf("[HIT] Heavy acertou o player: %d HP\n", currentDamage);
                            }
                        }
                    } else {
                        if (localT >= activeEnd) b->hasHitPlayerThisAttack = false;
                        b->atk.active = false;
                    }
                } break;

                case BOSS_ATTACK_TYPE_BRUTAL: {
                    windupEnd = GetPhaseWindup(b, b->brutalWindup);
                    activeEnd = windupEnd + b->brutalActive;
                    totalEnd = windupEnd + b->brutalActive + b->brutalRecovery;
                    cooldown = GetAttackCooldown(b, 1.15f);

                    if (b->stateTimer >= windupEnd && b->stateTimer < activeEnd) {
                        Vector2 delta = Vector2Scale(b->attackDir, b->brutalLungeSpeed * dt);
                        MoveWithCollision(level, &b->pos, b->raio * 1.1f, delta);
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
                            printf("[HIT] Brutal acertou o player: %d HP\n", b->brutalDamage);
                        }
                    }
                } break;

                case BOSS_ATTACK_TYPE_PROJECTILE: {
                    windupEnd = GetPhaseWindup(b, 0.80f);
                    activeEnd = windupEnd + 0.12f;
                    totalEnd = windupEnd + 1.10f;
                    cooldown = GetAttackCooldown(b, 1.05f);

                    if (b->stateTimer < windupEnd) {
                        b->atk.active = false;
                    } else if (b->stateTimer >= windupEnd && !b->projActive) {
                        b->projActive = true;
                        b->projPos = Vector2Add(b->pos, Vector2Scale(b->attackDir, b->raio + b->projRadius + 4.0f));
                        b->projDir = b->attackDir;
                        b->atk.active = true;
                        b->atk.center = b->projPos;
                        b->atk.radius = b->projRadius * ((b->phase == 2) ? 1.15f : 1.0f);
                        b->atk.damage = b->projDamage;
                    }
                    if (b->projActive) {
                        float projSpeed = b->projSpeed * ((b->phase == 2) ? 1.55f : 1.0f);
                        float projRadius = b->projRadius * ((b->phase == 2) ? 1.15f : 1.0f);
                        Vector2 move = Vector2Scale(b->projDir, projSpeed * dt);
                        float newx = b->projPos.x + move.x;
                        float newy = b->projPos.y + move.y;
                        if (CanMoveCircle(level, newx, newy, projRadius)) {
                            b->projPos.x = newx;
                            b->projPos.y = newy;
                            b->atk.center = b->projPos;
                            b->atk.radius = projRadius;
                        } else {
                            b->projActive = false;
                            b->atk.active = false;
                        }
                        if (b->atk.active && !b->hasHitPlayerThisAttack) {
                            if (CheckCollisionCircles(b->atk.center, b->atk.radius, playerPos, playerRadius)) {
                                b->hasHitPlayerThisAttack = true;
                                printf("[HIT] Projectile acertou o player: %d HP\n", b->projDamage);
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
                            printf("[HIT] AoE Burst acertou o player: %d HP\n", b->aoeDamage);
                        }
                    }
                } break;

                default: break;
            }

            if (b->attackType != BOSS_ATTACK_TYPE_BRUTAL) {
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
            } break;
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
            } break;
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
            } break;
            case BOSS_ATTACK_TYPE_PROJECTILE: {
                float windupEnd = 0.80f;
                teleRadius = b->projRadius * ((b->phase == 2) ? 1.15f : 1.0f);
                teleCenter = Vector2Add(b->pos, Vector2Scale(b->attackDir, b->raio + teleRadius + 4.0f));
                if (b->stateTimer < windupEnd) {
                    inWindup = true;
                    drawWindupTelegraph = true;
                    teleColor = (Color){240, 220, 40, 170};
                } else if (b->projActive) {
                    drawActiveTelegraph = true;
                    teleColor = (Color){220, 160, 30, 180};
                }
                col = inWindup ? YELLOW : GOLD;
            } break;
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
            } break;
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

    // REMOVIDO: HUD de HP do boss daqui.
    // A HUD vai ser desenhada na main, abaixo do FPS.
}