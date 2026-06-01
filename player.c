#include "player.h"
#include "raymath.h"
#include "level.h"
#include <stddef.h>

bool PodeMoverPara(Level *level, float px, float py, float raio, int tw, int th) {
    /*A linhas abaixo pegam as coordenadas dos tiles que o player ocupa*/
    int esq   = (int)((px - raio) / tw);
    int dir   = (int)((px + raio) / tw);
    int cima  = (int)((py - raio) / th);
    int baixo = (int)((py + raio) / th);

    //A função level_pode_mover é chamada para cada um dos 4 tiles que o player ocupa, para verificar se ele pode se mover para essa posição. Se algum dos tiles for bloqueado, a função retorna false, indicando que o player não pode se mover para essa posição.
    return level_pode_mover(level, esq, cima) &&
           level_pode_mover(level, dir, cima) &&
           level_pode_mover(level, esq, baixo) &&
           level_pode_mover(level, dir, baixo);
}

void InitPlayer(Player *player, float tileX, float tileY, Level *level, PlayerTextures *textures) { //Adicionado o parâmetro de textura do player.
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
    player->lastMovingDir = (Vector2){ 0.0f, 1.0f }; //Começa olhando para o SUL.
    player->attackDirection = player->lastMovingDir;
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

    //Inicialização das variáveis de animação:
    player->textures = textures; //player aqui está recebendo o endereço do PlayerTextures criado na main, que já tem as texturas carregadas. Assim, player->textures aponta para as texturas do player.
    player->activeTexture = NULL; //Começa sem textura ativa, a textura será definida no UpdatePlayer de acordo com o estado do player.
    player->currentFrame = 0; //Começa no frame 0, mas a textura só será definida no UpdatePlayer, então isso é apenas uma inicialização segura.
    player->activeMaxFrames = 1; //Será substituído pelo valor correto da animação ativa no UpdatePlayer.
    player->frameTimer = 0.0f; //Inicializa o timer de troca de frames.
    player->currentDirIndex = 7; //Começa olhando para o SUL, que é dir8 no pacote.
    player->spriteAnchorOffset = (Vector2){ 0.0f, 0.0f };
    player->frameRec = (Rectangle){0, 0, 0, 0}; //Inicializa o retângulo de corte da textura. O UpdatePlayer vai atualizar isso de acordo com o frame atual e a direção.
}

Rectangle GetPlayerHitbox(Player *player) {
    float width = 30.0f;  // Ajuste para a largura do corpo do cavaleiro
    float height = 40.0f; // Ajuste para a altura do tronco
    float offsetY = 0.0f; // Subir mais (ajuste adicional +20px)
    return (Rectangle){ player->pos.x - (width / 2.0f), player->pos.y - offsetY, width, height };
}

//SetupAnimationSprite é a função responsável por configurar a textura de animação do player, definindo o frame atual, o retângulo de corte e a velocidade de animação. Ela é chamada dentro do UpdatePlayer para atualizar a animação de acordo com o estado do player.
static void SetupAnimationSprite(Player *player, Texture2D *texture, int frameCols, int frameRows, int totalFrames, float deltaTime, float animSpeed) {
    if (frameCols < 1) frameCols = 1;
    if (frameRows < 1) frameRows = 1;
    if (totalFrames < 1) totalFrames = 1;

    if (player->activeTexture != texture || player->activeMaxFrames != totalFrames) {
        player->activeTexture = texture;
        player->activeMaxFrames = totalFrames;
        player->currentFrame = 0;
        player->frameTimer = 0.0f;
    }

    if (player->activeTexture == NULL) {
        return;
    }

    float frameW = (float)player->activeTexture->width / (float)frameCols;
    float frameH = (float)player->activeTexture->height / (float)frameRows;

    if (frameW <= 0.0f) frameW = (float)player->activeTexture->width;
    if (frameH <= 0.0f) frameH = (float)player->activeTexture->height;

    player->frameTimer += deltaTime;
    if (player->frameTimer >= (1.0f / animSpeed)) {
        player->frameTimer = 0.0f;
        player->currentFrame++;
        if (player->currentFrame >= player->activeMaxFrames) {
            player->currentFrame = player->alive ? 0 : player->activeMaxFrames - 1;
        }
    }

    int frameIndex = player->currentFrame;
    int frameCol = frameIndex % frameCols;
    int frameRow = frameIndex / frameCols;

    player->frameRec.width = frameW;
    player->frameRec.height = frameH;
    player->frameRec.x = frameCol * frameW;
    player->frameRec.y = frameRow * frameH;
}

//UpdateAttackHitboxFromSprite é a função responsável por atualizar a posição da hitbox de ataque do player com base na posição do sprite e na direção do ataque. Ela é chamada dentro do UpdatePlayer para garantir que a hitbox esteja sempre alinhada com o sprite durante as animações de ataque.
static void UpdateAttackHitboxFromSprite(Player *player) {
    //O IF aqui verifica se a hitbox de ataque está ativa. Se não estiver ativa, não faz sentido atualizar a posição da hitbox, então a função retorna imediatamente.
    if (!player->isHitboxActive) {
        return;
    }

    // A hitbox usa a direção travada no início do ataque, para não depender do input atual.
    Vector2 aimDir = player->attackDirection;
    if (aimDir.x == 0.0f && aimDir.y == 0.0f) {
        aimDir = player->lastMovingDir;
    }

    aimDir = Vector2Normalize(aimDir);

    Rectangle playerHitbox = GetPlayerHitbox(player);
    Vector2 hitboxCenter = {
        playerHitbox.x + (playerHitbox.width * 0.5f),
        playerHitbox.y + (playerHitbox.height * 0.5f)
    };

    float frameW = player->frameRec.width;
    float frameH = player->frameRec.height;
    float maxReach = (frameW > frameH ? frameW : frameH) * 0.25f;
    if (player->comboStep == 3) maxReach *= 1.20f;
    else if (player->comboStep == 4) maxReach *= 1.35f;

    float useReach = maxReach * 0.9f;
    if (useReach < 1.0f) useReach = 0.0f;

    float forwardOffset = (frameW > frameH ? frameW : frameH) * 0.18f;
    if (player->comboStep == 3) forwardOffset *= 1.10f;
    else if (player->comboStep == 4) forwardOffset *= 1.20f;

    float attackOffset = useReach + forwardOffset;

    player->hitboxCenter = Vector2Add(hitboxCenter, Vector2Scale(aimDir, attackOffset));
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

    bool hasMovementInput = false;
    
    if (!player->alive) {
        goto LOGICA_VISUAL; //Se o player não está vivo, ele não pode fazer nada além de processar a animação de morte. Então pulamos toda a lógica de movimentação e ações e vamos direto para a parte de atualização da animação, que está no final da função, após a label LOGICA_VISUAL. 
    }

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
    hasMovementInput = (inputDir.x != 0.0f || inputDir.y != 0.0f);

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
                //(Código original de ataque pesado).
                player->heavyChargeTimer = 2.5f;
                player->currentAttackDamage = 50;
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
                player->attackDirection = (inputDir.x == 0.0f && inputDir.y == 0.0f) ? player->lastMovingDir : inputDir;
                player->isHitboxActive = false;
                player->hasHitEnemy = false;
            }
        }

        if (IsKeyReleased(KEY_E) && player->isChargingHeavy) {
            float cost;
            if (player->heavyChargeTimer >= 2.5f) {
                player->currentAttackDamage = 50;
                cost = 60.0f;
            } else if (player->heavyChargeTimer >= 1.0f) {
                float fator = (player->heavyChargeTimer - 1.0f) / 1.5f;
                if (fator < 0.0f) fator = 0.0f;
                if (fator > 1.0f) fator = 1.0f;
                player->currentAttackDamage = 25 + (int)(fator * 25.0f);
                cost = 25.0f + (fator * 35.0f);
            } else {
                player->currentAttackDamage = 25;
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
            player->attackDirection = (inputDir.x == 0.0f && inputDir.y == 0.0f) ? player->lastMovingDir : inputDir;
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
                player->attackDirection = (inputDir.x == 0.0f && inputDir.y == 0.0f) ? player->lastMovingDir : inputDir;
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
                player->attackDirection = (inputDir.x == 0.0f && inputDir.y == 0.0f) ? player->lastMovingDir : inputDir;
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

    //(Código original de resolução da hitbox de ataque).
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

            if (player->comboStep == 1) {
                player->currentAttackDamage = 10;
            } else if (player->comboStep == 2) {
                player->currentAttackDamage = 10;
            } else if (player->comboStep == 3) {
                player->currentAttackDamage = 15;
            }

            if (player->comboStep == 1 || player->comboStep == 2) {
                player->hitboxRadius = tw * 0.7f;
            } else if (player->comboStep == 3) {
                player->hitboxRadius = tw * 0.3f;
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
            }
        }
    } else if (player->attackState == 2) {
        player->attackStateTimer += deltaTime;
        float active;
        if (player->comboStep == 4) {
            active = player->attackActive[0];
        } else {
            active = player->attackActive[player->comboStep - 1];
        }

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

    if (velX != 0.0f) {
        if (PodeMoverPara(level, novo_x, player->pos.y, r, tw, th)) {
            player->pos.x = novo_x;
        } else {
            velX = 0.0f;
        }
    }

    if (velY != 0.0f) {
        if (PodeMoverPara(level, player->pos.x, novo_y, r, tw, th)) {
            player->pos.y = novo_y;
        } else {
            velY = 0.0f;
        }
    }

    player->tile_pos.x = player->pos.x / tw;
    player->tile_pos.y = player->pos.y / th;

LOGICA_VISUAL:
    //Atualização da animação do player:
    if (player->textures != NULL) { 
        // 1. Converter Vetor para Índice de Direção (0 a 7 correspondendo a dir1..dir8)
        float dx = player->lastMovingDir.x;
        float dy = player->lastMovingDir.y;
        
        // Ordem real dos arquivos dir1..dir8 deste pacote:
        // dir1=SW, dir2=W, dir3=NW, dir4=N, dir5=NE, dir6=E, dir7=SE, dir8=S.
        if (dx == 0 && dy > 0) player->currentDirIndex = 7;       // Sul -> dir8
        else if (dx > 0 && dy > 0) player->currentDirIndex = 6;   // Sudeste -> dir7
        else if (dx > 0 && dy == 0) player->currentDirIndex = 5;  // Leste -> dir6
        else if (dx > 0 && dy < 0) player->currentDirIndex = 4;   // Nordeste -> dir5
        else if (dx == 0 && dy < 0) player->currentDirIndex = 3;  // Norte -> dir4
        else if (dx < 0 && dy < 0) player->currentDirIndex = 2;   // Noroeste -> dir3
        else if (dx < 0 && dy == 0) player->currentDirIndex = 1;  // Oeste -> dir2
        else if (dx < 0 && dy > 0) player->currentDirIndex = 0;   // Sudoeste -> dir1

        // 2. Determinar a textura e grade pela Máquina de Estados
        Texture2D* nextTexture = &player->textures->idle[player->currentDirIndex];
        int nextFrameCols = player->textures->idleFrameCols;
        int nextFrameRows = player->textures->idleFrameRows;
        int nextTotalFrames = player->textures->idleTotalFrames;
        float animSpeed = player->textures->idleAnimSpeed;

        if (!player->alive) {
            nextTexture = &player->textures->die[player->currentDirIndex];
            nextFrameCols = player->textures->dieFrameCols;
            nextFrameRows = player->textures->dieFrameRows;
            nextTotalFrames = player->textures->dieTotalFrames;
            animSpeed = player->textures->dieAnimSpeed;
        } else if (player->isHealing) {
            player->spriteAnchorOffset = (Vector2){ 0.0f, 2.0f };
            nextTexture = &player->textures->heal[player->currentDirIndex];
            nextFrameCols = player->textures->healFrameCols;
            nextFrameRows = player->textures->healFrameRows;
            nextTotalFrames = player->textures->healTotalFrames;
            animSpeed = player->textures->healAnimSpeed;
        } else if (player->isParrying) {
            player->spriteAnchorOffset = (Vector2){ 0.0f, 0.0f };
            nextTexture = &player->textures->parry[player->currentDirIndex];
            nextFrameCols = player->textures->parryFrameCols;
            nextFrameRows = player->textures->parryFrameRows;
            nextTotalFrames = player->textures->parryTotalFrames;
            animSpeed = player->textures->parryAnimSpeed;
        } else if (player->isDashing) {
            player->spriteAnchorOffset = (Vector2){ 0.0f, 0.0f };
            nextTexture = &player->textures->dash[player->currentDirIndex];
            nextFrameCols = player->textures->dashFrameCols;
            nextFrameRows = player->textures->dashFrameRows;
            nextTotalFrames = player->textures->dashTotalFrames;
            animSpeed = player->textures->dashAnimSpeed;
        } else if (player->attackState > 0) {
            animSpeed = player->textures->attackAnimSpeed;
            //Ao selecionar abaixo, o player->comboStep já foi atualizado para o próximo ataque, então ele já aponta para a animação correta do próximo ataque. Assim, se comboStep for 1, ele mostra a animação do primeiro ataque, se for 2, mostra a animação do segundo ataque, e assim por diante. Isso garante que a animação corresponda ao estado atual do combo.
            if (player->comboStep == 1) {
                player->spriteAnchorOffset = (Vector2){ 0.0f, 4.0f };
                nextTexture = &player->textures->attack[player->currentDirIndex];
                nextFrameCols = player->textures->attackFrameCols;
                nextFrameRows = player->textures->attackFrameRows;
                nextTotalFrames = player->textures->attackTotalFrames;
                animSpeed = player->textures->attackAnimSpeed;
            } else if (player->comboStep == 2) {
                player->spriteAnchorOffset = (Vector2){ 0.0f, 4.0f };
                nextTexture = &player->textures->attack[player->currentDirIndex];
                nextFrameCols = player->textures->attackFrameCols;
                nextFrameRows = player->textures->attackFrameRows;
                nextTotalFrames = player->textures->attackTotalFrames;
                animSpeed = player->textures->attackAnimSpeed;
            } else if (player->comboStep == 3) {
                player->spriteAnchorOffset = (Vector2){ 0.0f, 2.5f };
                nextTexture = &player->textures->comboAttack[player->currentDirIndex];
                nextFrameCols = player->textures->comboAttackFrameCols;
                nextFrameRows = player->textures->comboAttackFrameRows;
                nextTotalFrames = player->textures->comboAttackTotalFrames;
                animSpeed = player->textures->comboAttackAnimSpeed;
            } else if (player->comboStep == 4) {
                player->spriteAnchorOffset = (Vector2){ 0.0f, 1.5f };
                nextTexture = &player->textures->heavyAttack[player->currentDirIndex];
                nextFrameCols = player->textures->heavyAttackFrameCols;
                nextFrameRows = player->textures->heavyAttackFrameRows;
                nextTotalFrames = player->textures->heavyAttackTotalFrames;
                animSpeed = player->textures->heavyAttackAnimSpeed;
            }
        } else if (hasMovementInput) {
            player->spriteAnchorOffset = (Vector2){ 0.0f, 0.0f };
            nextTexture = &player->textures->walk[player->currentDirIndex];
            nextFrameCols = player->textures->walkFrameCols;
            nextFrameRows = player->textures->walkFrameRows;
            nextTotalFrames = player->textures->walkTotalFrames;
            animSpeed = player->textures->walkAnimSpeed;
        } else {
            player->spriteAnchorOffset = (Vector2){ 0.0f, 0.0f };
        }

        SetupAnimationSprite(player, nextTexture, nextFrameCols, nextFrameRows, nextTotalFrames, deltaTime, animSpeed);
        UpdateAttackHitboxFromSprite(player);
    }
}

void DrawPlayer(Player player) {
    Rectangle playerHitbox = GetPlayerHitbox(&player);
    float centerX = playerHitbox.x + (playerHitbox.width / 2.0f);
    float centerY = playerHitbox.y + (playerHitbox.height / 2.0f);

    if (player.activeTexture != NULL && player.activeTexture->id != 0) {
        Rectangle destRec = {
            centerX + player.spriteAnchorOffset.x,
            centerY + player.spriteAnchorOffset.y,
            player.frameRec.width,
            player.frameRec.height
        };
        Vector2 origin = {
            player.frameRec.width * 0.5f,
            player.frameRec.height * 0.5f
        };

        Color tint = WHITE;
        if (player.isDashing) tint = ORANGE;
        else if (player.cooldownTimeCounter > 0.0f) tint = GRAY;

        DrawTexturePro(*player.activeTexture, player.frameRec, destRec, origin, 0.0f, tint);
    } else {
        if (!player.alive) return;
        Color playerOuterColor = MAROON;
        if (player.isDashing) playerOuterColor = WHITE;
        else if (player.cooldownTimeCounter > 0.0f) playerOuterColor = DARKGRAY;

        DrawCircleV(player.pos, player.raio, playerOuterColor);
        DrawCircleGradient(player.pos, player.raio * 0.65f, ORANGE, playerOuterColor);
    }

    if (player.isParrying) {
        DrawCircleLines((int)centerX, (int)centerY, player.raio * 1.35f, BLUE);
    }
    if (player.isHitboxActive) {
        DrawCircleLines((int)player.hitboxCenter.x, (int)player.hitboxCenter.y, player.hitboxRadius, RED);
    }
}

void PlayerTakeDamage(Player *player, int damage) {
    if (player->isParrying || player->isDashing) return;

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