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
    b->raio = 28.0f;

    b->hpMax = 200;
    b->hp = b->hpMax;

    b->state = BOSS_WANDER;
    b->stateTimer = 0.0f;

    b->aggroRadius = 220.0f;
    b->openingBrutalPending = false;

    b->wanderDir = (Vector2){1, 0};
    b->wanderSpeed = 70.0f;
    b->wanderChangeTimer = RandRange(0.8f, 1.8f);

    b->combatSpeed = 90.0f;

    // Light attack
    b->lightWindup = 0.20f;
    b->lightActive = 0.12f;
    b->lightRecovery = 0.35f;
    b->lightHitRadius = 45.0f;
    b->lightForwardOffset = 35.0f;
    b->lightDamage = 10;

    // Brutal attack (abertura)
    b->brutalWindup = 0.70f;
    b->brutalActive = 0.15f;
    b->brutalRecovery = 0.90f;
    b->brutalHitRadius = 70.0f;
    b->brutalLungeSpeed = 420.0f;
    b->brutalDamage = 75;

    b->atk = (AttackCircle){0};
    b->attackDir = (Vector2){1, 0};
    b->lockedTargetPos = startPos;
    b->hasHitPlayerThisAttack = false;
}

static void EnterAlert(Boss *b) {
    b->state = BOSS_ALERT;
    b->stateTimer = 0.90f;
    b->atk.active = false;
    b->openingBrutalPending = true; // sempre que sai do WANDER
}

static void StartAttack(Boss *b, BossState attackState, Vector2 playerPos) {
    b->state = attackState;
    b->stateTimer = 0.0f;
    b->lockedTargetPos = playerPos;
    b->attackDir = SafeNormalize(Vector2Subtract(playerPos, b->pos));
    b->atk.active = false;
    b->hasHitPlayerThisAttack = false;
}

void Boss_Update(Boss *b, float dt, Vector2 playerPos, float playerRadius, Level *level) {
    if (b->hp <= 0) return;

    // Ajusta tamanho/velocidade conforme tile
    int tw = level->tamanho_tile;
    int th = level->tamanho_tile_h;
    float tileAvg = (tw + th) * 0.5f;

    float bossR = tileAvg * 0.35f;
    if (bossR < 18.0f) bossR = 18.0f;
    b->raio = bossR;

    float dist = Vector2Distance(b->pos, playerPos);

    switch (b->state) {
        case BOSS_WANDER: {
            b->wanderChangeTimer -= dt;
            if (b->wanderChangeTimer <= 0.0f) {
                Vector2 r = (Vector2){ RandRange(-1, 1), RandRange(-1, 1) };
                b->wanderDir = SafeNormalize(r);
                b->wanderChangeTimer = RandRange(0.8f, 1.8f);
            }

            Vector2 delta = Vector2Scale(b->wanderDir, b->wanderSpeed * dt);
            MoveWithCollision(level, &b->pos, b->raio * 1.1f, delta);

            if (dist <= b->aggroRadius) {
                EnterAlert(b);
            }
        } break;

        case BOSS_ALERT: {
            b->stateTimer -= dt;
            if (b->stateTimer <= 0.0f) {
                if (b->openingBrutalPending) {
                    b->openingBrutalPending = false;
                    StartAttack(b, BOSS_ATTACK_BRUTAL, playerPos);
                } else {
                    b->state = BOSS_COMBAT;
                }
            }
        } break;

        case BOSS_ATTACK_BRUTAL: {
            b->stateTimer += dt;

            float windupEnd = b->brutalWindup;
            float activeEnd = b->brutalWindup + b->brutalActive;
            float totalEnd  = b->brutalWindup + b->brutalActive + b->brutalRecovery;

            if (b->stateTimer <= activeEnd) {
                Vector2 delta = Vector2Scale(b->attackDir, b->brutalLungeSpeed * dt);
                MoveWithCollision(level, &b->pos, b->raio * 1.1f, delta);
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

                    // Antes: -%d
                    printf("[HIT] Brutal acertou o player: %d HP\n", b->brutalDamage);
                }
            }

            if (b->stateTimer >= totalEnd) {
                b->atk.active = false;
                b->state = BOSS_COOLDOWN;
                b->stateTimer = 0.35f;
            }
        } break;

        case BOSS_COMBAT: {
            Vector2 dirToPlayer = SafeNormalize(Vector2Subtract(playerPos, b->pos));
            Vector2 delta = Vector2Scale(dirToPlayer, b->combatSpeed * dt);
            MoveWithCollision(level, &b->pos, b->raio * 1.1f, delta);

            if (dist < (tileAvg * 2.2f)) {
                StartAttack(b, BOSS_ATTACK_LIGHT, playerPos);
            }

            if (dist > b->aggroRadius * 1.25f) {
                b->state = BOSS_WANDER;
                b->openingBrutalPending = false;
            }
        } break;

        case BOSS_ATTACK_LIGHT: {
            b->stateTimer += dt;

            float windupEnd = b->lightWindup;
            float activeEnd = b->lightWindup + b->lightActive;
            float totalEnd  = b->lightWindup + b->lightActive + b->lightRecovery;

            if (b->stateTimer >= windupEnd && b->stateTimer < activeEnd) {
                b->atk.active = true;
                b->atk.damage = b->lightDamage;
                b->atk.radius = b->lightHitRadius;
                b->atk.center = Vector2Add(b->pos, Vector2Scale(b->attackDir, b->lightForwardOffset));
            } else {
                b->atk.active = false;
            }

            if (b->atk.active && !b->hasHitPlayerThisAttack) {
                if (CheckCollisionCircles(b->atk.center, b->atk.radius, playerPos, playerRadius)) {
                    b->hasHitPlayerThisAttack = true;

                    // Antes: -%d
                    printf("[HIT] Leve acertou o player: %d HP\n", b->lightDamage);
                }
            }

            if (b->stateTimer >= totalEnd) {
                b->atk.active = false;
                b->state = BOSS_COOLDOWN;
                b->stateTimer = 0.30f;
            }
        } break;

        case BOSS_COOLDOWN: {
            b->stateTimer -= dt;
            if (b->stateTimer <= 0.0f) {
                b->state = BOSS_COMBAT;
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
    if (b->state == BOSS_ALERT) {
        int blink = ((int)(GetTime() * 10.0) % 2);
        col = blink ? RED : MAROON;
    } else if (b->state == BOSS_ATTACK_BRUTAL) col = ORANGE;
    else if (b->state == BOSS_ATTACK_LIGHT) col = GOLD;

    DrawCircleV(b->pos, b->raio, col);

    if (b->atk.active) {
        DrawCircleLines((int)b->atk.center.x, (int)b->atk.center.y, b->atk.radius, RAYWHITE);
    }

    // REMOVIDO: HUD de HP do boss daqui.
    // A HUD vai ser desenhada na main, abaixo do FPS.
}