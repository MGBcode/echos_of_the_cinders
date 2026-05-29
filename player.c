#include "player.h"
#include "raymath.h"
#include "level.h"

bool PodeMoverPara(Level *level, float px, float py, float raio, int tw, int th) {
    int esq   = (int)((px - raio) / tw);
    int dir   = (int)((px + raio) / tw);
    int cima  = (int)((py - raio) / th);
    int baixo = (int)((py + raio) / th);

    return level_pode_mover(level, esq, cima) &&
           level_pode_mover(level, dir, cima) &&
           level_pode_mover(level, esq, baixo) &&
           level_pode_mover(level, dir, baixo);
}

void InitPlayer(Player *player, float tileX, float tileY, Level *level) {
    player->tile_pos = (Vector2){ tileX, tileY };
    player->pos = (Vector2){ tileX * level->tamanho_tile, tileY * level->tamanho_tile_h };
    player->raio = level->tamanho_tile * 0.35f;
    player->normalSpeed = 340.0f;
    player->isDashing = false;
    player->dashSpeedMultiplier = 4.0f;
    player->dashDuration = 0.20f;
    player->dashCooldown = 0.5f;
    player->dashTimeCounter = 0.0f;
    player->cooldownTimeCounter = 0.0f;
    player->dashDirection = (Vector2){ 0, 0 };
    player->lastMovingDir = (Vector2){ 1.0f, 0.0f };
    player->lastHorizontalInput = 1;
    player->lastVerticalInput = 0;
    player->comboStep = 0;
    player->attackWindup[0] = 0.40f; player->attackActive[0] = 0.10f; player->attackRecovery[0] = 0.6f;
    player->attackWindup[1] = 0.30f; player->attackActive[1] = 0.10f; player->attackRecovery[1] = 0.6f;
    player->attackWindup[2] = 0.12f; player->attackActive[2] = 0.40f; player->attackRecovery[2] = 0.2f;
    player->attackState = 0;
    player->attackStateTimer = 0.0f;
    player->comboWindowTimer = 0.5f;
    player->isHitboxActive = false;
    player->hitboxRadius = 0.0f;
    player->hitboxCenter = (Vector2){ 0, 0 };
    player->hasHitEnemy = false;
    player->isParrying = false;
    player->parryTimer = 0.0f;
    player->parryDuration = 0.4f;
    player->hpMax = 100;
    player->hp = player->hpMax;
    player->staminaMax = 100.0f;
    player->stamina = player->staminaMax;
    player->staminaRecoveryDelay = 1.5f;
    player->baseStaminaRecoveryDelay = 1.5f;
    player->staminaRecoveryDelayCounter = 0.0f;
    player->staminaRecoveryRate = 25.0f;
    player->baseStaminaRecoveryRate = 25.0f;
    player->isExhausted = false;
    player->frascosMax = 3;
    player->frascosAtuais = 3;
    player->isHealing = false;
    player->healingTimer = 0.0f;
    player->state = PLAYER_STATE_IDLE;
    player->heavyChargeTimer = 0.0f;
    player->currentAttackDamage = 0;
    player->isChargingHeavy = false;
    player->alive = true;
}

void UpdatePlayer(Player *player, float deltaTime, Level *level) {
    int tw = level->tamanho_tile;
    int th = level->tamanho_tile_h;
    bool staminaInfinite = (level->salaAtual == SALA_TREINO);
    player->raio = level->tamanho_tile * 0.35f;
    player->normalSpeed = 340.0f;
    if (staminaInfinite) {
        player->stamina = player->staminaMax;
        player->staminaRecoveryDelayCounter = player->staminaRecoveryDelay;
    } else {
        UpdatePlayerStamina(player, deltaTime);
    }

    if (player->isParrying) {
        player->parryTimer -= deltaTime;
        if (player->parryTimer <= 0.0f) {
            player->isParrying = false;
            player->parryTimer = 0.0f;
        }
    }

    if (player->isHealing) {
        player->healingTimer -= deltaTime;
        if (player->healingTimer <= 0.0f) {
            player->healingTimer = 0.0f;
            player->isHealing = false;
            player->hp += 60;
            if (player->hp > player->hpMax) player->hp = player->hpMax;
        }
    }

    if (!player->alive) return;

    float r = player->raio * 1.2f;

    if (!player->isDashing && player->cooldownTimeCounter > 0.0f)
        player->cooldownTimeCounter -= deltaTime;

    Vector2 inputDir = { 0.0f, 0.0f };
    bool rightDown = IsKeyDown(KEY_D);
    bool leftDown = IsKeyDown(KEY_A);
    bool downDown = IsKeyDown(KEY_S);
    bool upDown = IsKeyDown(KEY_W);

    if (IsKeyPressed(KEY_D)) player->lastHorizontalInput = 1;
    if (IsKeyPressed(KEY_A)) player->lastHorizontalInput = -1;
    if (IsKeyPressed(KEY_S)) player->lastVerticalInput = 1;
    if (IsKeyPressed(KEY_W)) player->lastVerticalInput = -1;

    if (rightDown && leftDown) {
        inputDir.x = (float)player->lastHorizontalInput;
    } else if (rightDown) {
        inputDir.x = 1.0f;
        player->lastHorizontalInput = 1;
    } else if (leftDown) {
        inputDir.x = -1.0f;
        player->lastHorizontalInput = -1;
    }

    if (downDown && upDown) {
        inputDir.y = (float)player->lastVerticalInput;
    } else if (downDown) {
        inputDir.y = 1.0f;
        player->lastVerticalInput = 1;
    } else if (upDown) {
        inputDir.y = -1.0f;
        player->lastVerticalInput = -1;
    }

    inputDir = Vector2Normalize(inputDir);

    bool isInputDiagonal = (inputDir.x != 0.0f && inputDir.y != 0.0f);

    if (inputDir.x != 0.0f || inputDir.y != 0.0f) {
        if (isInputDiagonal) {
            player->lastMovingDir = inputDir;
            player->diagonalBufferTimer = 0;
        } else {
            player->diagonalBufferTimer += deltaTime;
            bool wasLastDirDiagonal = (player->lastMovingDir.x != 0.0f && player->lastMovingDir.y != 0.0f);
            if (player->diagonalBufferTimer > 0.05f || !wasLastDirDiagonal)
                player->lastMovingDir = inputDir;
        }
    } else {
        player->diagonalBufferTimer = 0;
    }

    if (IsKeyPressed(KEY_E) && player->attackState == 0 && !player->isDashing && !player->isHealing && !player->isParrying) {
        player->state = PLAYER_STATE_HEAVY_ATTACK;
        player->isChargingHeavy = true;
        player->heavyChargeTimer = 0.0f;
    }

    if (player->isChargingHeavy) {
        if (IsKeyDown(KEY_E)) {
            player->heavyChargeTimer += deltaTime;
            if (player->heavyChargeTimer >= 2.5f) {
                player->heavyChargeTimer = 2.5f;
                player->currentAttackDamage = 40;
                float cost = 60.0f;
                player->stamina -= cost;
                if (player->stamina < 0.0f) player->stamina = 0.0f;
                player->staminaRecoveryDelayCounter = 0.0f;
                if (player->stamina == 0.0f) {
                    player->isExhausted = true;
                    player->staminaRecoveryDelay = 2.0f;
                    player->staminaRecoveryRate = 20.0f;
                }
                player->isChargingHeavy = false;
                player->state = PLAYER_STATE_IDLE;
                player->attackState = 1;
                player->attackStateTimer = 0.0f;
                player->comboStep = 4;
                player->isHitboxActive = false;
                player->hasHitEnemy = false;
            }
        }

        if (IsKeyReleased(KEY_E) && player->isChargingHeavy) {
            float cost;
            if (player->heavyChargeTimer >= 2.5f) {
                player->currentAttackDamage = 40;
                cost = 60.0f;
            } else if (player->heavyChargeTimer >= 1.0f) {
                float fator = (player->heavyChargeTimer - 1.0f) / 1.5f;
                if (fator < 0.0f) fator = 0.0f;
                if (fator > 1.0f) fator = 1.0f;
                player->currentAttackDamage = 20 + (int)(fator * 20.0f);
                cost = 25.0f + (fator * 35.0f);
            } else {
                player->currentAttackDamage = 20;
                cost = 25.0f;
            }
            player->stamina -= cost;
            if (player->stamina < 0.0f) player->stamina = 0.0f;
            player->staminaRecoveryDelayCounter = 0.0f;
            if (player->stamina == 0.0f) {
                player->isExhausted = true;
                player->staminaRecoveryDelay = 2.0f;
                player->staminaRecoveryRate = 20.0f;
            }
            player->isChargingHeavy = false;
            player->state = PLAYER_STATE_IDLE;
            player->attackState = 1;
            player->attackStateTimer = 0.0f;
            player->comboStep = 4;
            player->isHitboxActive = false;
            player->hasHitEnemy = false;
        }
    }

    if (IsKeyPressed(KEY_SPACE) && !player->isDashing && player->cooldownTimeCounter <= 0.0f && (staminaInfinite || player->stamina >= 15.0f)) {
        player->attackState = 0;
        player->attackStateTimer = 0.0f;
        player->comboStep = 0;
        player->isHitboxActive = false;
        player->hasHitEnemy = false;

        player->isDashing = true;
        player->dashTimeCounter = 0.0f;
        player->dashDirection = (inputDir.x == 0.0f && inputDir.y == 0.0f) ? player->lastMovingDir : inputDir;
        if (!staminaInfinite) {
            PlayerUseStamina(player, 15.0f);
        }
    }
    if (IsKeyPressed(KEY_Q) &&
        player->frascosAtuais > 0 &&
        !player->isDashing &&
        player->attackState == 0 &&
        !player->isHealing)
    {
        player->isHealing = true;
        player->healingTimer = 1.0f;
        player->frascosAtuais -= 1;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        if (player->attackState == 0 &&
            !player->isDashing &&
            !player->isHealing &&
            !player->isParrying)
        {
            player->isParrying = true;
            player->parryTimer = player->parryDuration;
        }
    }

    const float ATTACK_COST = 22.5f;
    if (!player->isHealing &&
        !player->isParrying &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        if (player->attackState == 3) {
            if (player->comboStep < 3 && (staminaInfinite || player->stamina >= ATTACK_COST) && !player->isDashing) {
                player->comboStep += 1;
                player->attackState = 1; 
                player->attackStateTimer = 0.0f;
                player->hasHitEnemy = false;
                if (!staminaInfinite) {
                    PlayerUseStamina(player, ATTACK_COST);
                }
            }
        } else if (player->attackState == 0) {
            
            if (!player->isDashing && (staminaInfinite || player->stamina >= ATTACK_COST)) {
                player->comboStep = 1;
                player->attackState = 1; 
                player->attackStateTimer = 0.0f;
                player->hasHitEnemy = false;
                if (!staminaInfinite) {
                    PlayerUseStamina(player, ATTACK_COST);
                }
            }
        }
    }

    if (player->isHealing) {
        player->attackState = 0;
        player->attackStateTimer = 0.0f;
        player->comboStep = 0;
        player->isHitboxActive = false;
        player->hasHitEnemy = false;
    }

    if (player->attackState == 1) {
        player->attackStateTimer += deltaTime;
        float windup;
        if (player->comboStep == 4) {
            if (player->heavyChargeTimer >= 1.0f) {
                windup = 0.0f;
            } else {
                windup = 1.0f;
            }
        } else {
            windup = player->attackWindup[player->comboStep - 1];
        }

        if (player->attackStateTimer >= windup) {
            player->attackState = 2;
            player->attackStateTimer = 0.0f;
            player->isHitboxActive = true;
            player->hasHitEnemy = false;

            float distFront = 0.0f;
            if (player->comboStep == 1 || player->comboStep == 2) {
                player->hitboxRadius = tw * 0.7f;
                distFront = tw * 0.4f;
            } else if (player->comboStep == 3) {
                player->hitboxRadius = tw * 0.3f;
                distFront = tw * 0.8f;
            } else if (player->comboStep == 4) {
                float baseRadius = tw * 0.7f;
                float charge = player->heavyChargeTimer;
                if (charge >= 2.5f) {
                    player->hitboxRadius = baseRadius * 2.5f;
                } else if (charge >= 1.0f) {
                    float fator = (charge - 1.0f) / 1.5f;
                    if (fator < 0.0f) fator = 0.0f;
                    if (fator > 1.0f) fator = 1.0f;
                    player->hitboxRadius = baseRadius * (2.0f + fator * 0.5f);
                } else {
                    player->hitboxRadius = baseRadius * 2.0f;
                }
                distFront = tw * 0.4f;
            }
            player->hitboxCenter.x = player->pos.x + (player->lastMovingDir.x * distFront);
            player->hitboxCenter.y = player->pos.y + (player->lastMovingDir.y * distFront);
        }
    } else if (player->attackState == 2) {
        player->attackStateTimer += deltaTime;
        float active;
        if (player->comboStep == 4) {
            active = player->attackActive[0];
        } else {
            active = player->attackActive[player->comboStep - 1];
        }

        float distFront = 0.0f;
        if (player->comboStep == 1 || player->comboStep == 2) {
            distFront = tw * 0.4f;
        } else if (player->comboStep == 3) {
            distFront = tw * 0.8f;
        } else if (player->comboStep == 4) {
            distFront = tw * 0.4f;
        }
        player->hitboxCenter.x = player->pos.x + (player->lastMovingDir.x * distFront);
        player->hitboxCenter.y = player->pos.y + (player->lastMovingDir.y * distFront);

        if (player->attackStateTimer >= active) {
            player->attackState = 3;
            player->attackStateTimer = 0.0f;
            player->isHitboxActive = false;
        }
    } else if (player->attackState == 3) {
        player->attackStateTimer += deltaTime;
        float recovery;
        if (player->comboStep == 4) {
            recovery = player->attackRecovery[0];
        } else {
            recovery = player->attackRecovery[player->comboStep - 1];
        }
        if (player->attackStateTimer >= recovery) {
            player->attackState = 0;
            player->attackStateTimer = 0.0f;
            player->comboStep = 0;
            player->isHitboxActive = false;
            player->hasHitEnemy = false;
        }
    } else {
        player->isHitboxActive = false;
    }

    float velX = 0;
    float velY = 0;
    float movementSpeed = player->normalSpeed * (player->isHealing ? 0.4f : 1.0f);

    

    if (player->isDashing) {
        velX = player->dashDirection.x * (player->normalSpeed * player->dashSpeedMultiplier) * deltaTime;
        velY = player->dashDirection.y * (player->normalSpeed * player->dashSpeedMultiplier) * deltaTime;
        
        player->dashTimeCounter += deltaTime;
        if (player->dashTimeCounter >= player->dashDuration) {
            player->isDashing = false;
            player->cooldownTimeCounter = player->dashCooldown;
        }
    } else if (player->attackState == 2 && player->comboStep == 3) {
        
        velX = player->lastMovingDir.x * (player->normalSpeed * 1.5f) * deltaTime;
        velY = player->lastMovingDir.y * (player->normalSpeed * 1.5f) * deltaTime;
    } else if (player->attackState == 1 || player->attackState == 2 || player->isParrying) {
        
        velX = 0;
        velY = 0;
    } else {
        
        velX = inputDir.x * movementSpeed * deltaTime;
        velY = inputDir.y * movementSpeed * deltaTime;
    }

    float novo_x = player->pos.x + velX;
    float novo_y = player->pos.y + velY;

    if (velX != 0.0f && PodeMoverPara(level, novo_x, player->pos.y, r, tw, th)) {
        player->pos.x = novo_x;
    }
    if (velY != 0.0f && PodeMoverPara(level, player->pos.x, novo_y, r, tw, th)) {
        player->pos.y = novo_y;
    }

    player->tile_pos.x = player->pos.x / tw;
    player->tile_pos.y = player->pos.y / th;
}

void DrawPlayer(Player player) {
    if (!player.alive) return;

    Color playerOuterColor = MAROON;
    if (player.isDashing) playerOuterColor = WHITE;
    else if (player.cooldownTimeCounter > 0.0f) playerOuterColor = DARKGRAY;

    DrawCircleV(player.pos, player.raio, playerOuterColor);
    DrawCircleGradient(player.pos, player.raio * 0.65f, ORANGE, playerOuterColor);
    if (player.isParrying) {
        DrawCircleLines((int)player.pos.x, (int)player.pos.y, player.raio * 1.5f, BLUE);
    }
    if (player.isHitboxActive) {
        DrawCircleLines((int)player.hitboxCenter.x, (int)player.hitboxCenter.y, player.hitboxRadius, RED);
    }
}


void PlayerTakeDamage(Player *player, int damage) {
    if (player->isParrying) {
        return;
    }

    if (player->isDashing) return;

    player->hp -= damage;
    if (player->hp < 0) player->hp = 0;
    if (player->hp == 0) {
        player->alive = false;
        
        player->isDashing = false;
        player->isHitboxActive = false;
        player->isParrying = false;
        player->isHealing = false;
    }
}

void PlayerTakeDamage_IgnoreParry(Player *player, int damage) {
    if (player->isDashing) return;

    player->isParrying = false;

    player->hp -= damage;
    if (player->hp < 0) player->hp = 0;
    if (player->hp == 0) {
        player->alive = false;
        player->isDashing = false;
        player->isHitboxActive = false;
        player->isParrying = false;
        player->isHealing = false;
    }
}

void PlayerUseStamina(Player *player, float amount) {
    player->stamina -= amount;
    if (player->stamina < 0.0f) player->stamina = 0.0f;
    
    
    player->staminaRecoveryDelayCounter = 0.0f;
}

void UpdatePlayerStamina(Player *player, float deltaTime) {
    if (player->staminaRecoveryDelayCounter < player->staminaRecoveryDelay) {
        player->staminaRecoveryDelayCounter += deltaTime;
    } else {
        player->stamina += player->staminaRecoveryRate * deltaTime;
        if (player->stamina > player->staminaMax) {
            player->stamina = player->staminaMax;
            if (player->isExhausted) {
                player->isExhausted = false;
                player->staminaRecoveryDelay = player->baseStaminaRecoveryDelay;
                player->staminaRecoveryRate = player->baseStaminaRecoveryRate;
            }
        }
    }
}

void DrawPlayerHUD(const Player *player, int x, int y) {
    
    DrawText(TextFormat("HP: %d/%d", player->hp, player->hpMax), x, y, 20, LIGHTGRAY);
    
    const int barW = 220;
    const int barH = 14;
    const int barY = y + 24;
    
    
    DrawRectangle(x, barY, barW, barH, (Color){ 40, 40, 40, 220 });
    DrawRectangleLines(x, barY, barW, barH, (Color){ 110, 110, 110, 255 });
    
    
    float hpRatio = (float)player->hp / (float)player->hpMax;
    if (hpRatio < 0.0f) hpRatio = 0.0f;
    if (hpRatio > 1.0f) hpRatio = 1.0f;
    
    int hpFillW = (int)(barW * hpRatio);
    Color hpFill = (hpRatio > 0.5f) ? GREEN : (hpRatio > 0.25f ? ORANGE : RED);
    DrawRectangle(x + 1, barY + 1, hpFillW - 2 > 0 ? hpFillW - 2 : 0, barH - 2, hpFill);
    
    
    int staminaY = barY + barH + 20;
    DrawText(TextFormat("STAMINA: %.0f/%.0f", player->stamina, player->staminaMax), x, staminaY, 20, LIGHTGRAY);
    
    const int staminaBarY = staminaY + 24;
    
    
    DrawRectangle(x, staminaBarY, barW, barH, (Color){ 40, 40, 40, 220 });
    DrawRectangleLines(x, staminaBarY, barW, barH, (Color){ 110, 110, 110, 255 });
    
    
    float staminaRatio = player->stamina / player->staminaMax;
    if (staminaRatio < 0.0f) staminaRatio = 0.0f;
    if (staminaRatio > 1.0f) staminaRatio = 1.0f;
    
    int staminaFillW = (int)(barW * staminaRatio);
    Color staminaFillColor = player->isExhausted ? RED : YELLOW;
    DrawRectangle(x + 1, staminaBarY + 1, staminaFillW - 2 > 0 ? staminaFillW - 2 : 0, barH - 2, staminaFillColor);

    DrawText(TextFormat("Curas: %d/%d", player->frascosAtuais, player->frascosMax),
             x,
             staminaBarY + barH + 18,
             20,
             LIGHTGRAY);

    if (player->isHealing) {
        DrawText("Curando...",
                 x,
                 staminaBarY + barH + 42,
                 20,
                 GOLD);
    }
}