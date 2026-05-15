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

void InitPlayer(Player *player, int screenWidth, int screenHeight) {
    player->pos = (Vector2){ (float)screenWidth/2, (float)screenHeight/2 };
    player->normalSpeed = 200.0f;
    player->isDashing = false;
    player->dashSpeedMultiplier = 4.0f;
    player->dashDuration = 0.20f;
    player->dashCooldown = 0.5f;
    player->dashTimeCounter = 0.0f;
    player->cooldownTimeCounter = 0.0f;
    player->dashDirection = (Vector2){ 0, 0 };
    player->lastMovingDir = (Vector2){ 1.0f, 0.0f };
    player->comboStep = 0;
    player->attackTimer = 0.3f;
    player->comboWindowTimer = 0.5f;
    player->cooldownattackTimer = 0.0f;
    player->cooldowncomboWindowTimer = 0.0f;
    player->isHitboxActive = false;
    player->hitboxRadius = 0.0f;
    player->hitboxCenter = (Vector2){ 0, 0 };
    player->hasHitEnemy = false;
    player->raio = 15.0f;
    
    // HUD - Vida e Estamina
    player->hpMax = 100;
    player->hp = player->hpMax;
    player->staminaMax = 100.0f;
    player->stamina = player->staminaMax;
    player->staminaRecoveryDelay = 2.0f;
    player->staminaRecoveryDelayCounter = 0.0f;
    player->staminaRecoveryRate = 20.0f; // 20 pontos por segundo
    player->alive = true;
}

void UpdatePlayer(Player *player, float deltaTime, Level *level) {
    int tw = level->tamanho_tile;
    int th = level->tamanho_tile_h;

    player->raio = ((tw + th) / 2) * 0.3f;
    player->normalSpeed = ((tw + th) / 2) * 3.5f;
    
    // Atualizar estamina
    UpdatePlayerStamina(player, deltaTime);

    // Se estiver morto, não processa entradas nem movimentos
    if (!player->alive) return;

    float r = player->raio * 1.2f;

    if (!player->isDashing && player->cooldownTimeCounter > 0.0f)
        player->cooldownTimeCounter -= deltaTime;

    Vector2 inputDir = { 0.0f, 0.0f };
    if (IsKeyDown(KEY_D)) inputDir.x += 1.0f;
    if (IsKeyDown(KEY_A)) inputDir.x -= 1.0f;
    if (IsKeyDown(KEY_S)) inputDir.y += 1.0f;
    if (IsKeyDown(KEY_W)) inputDir.y -= 1.0f;
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

    if (IsKeyPressed(KEY_SPACE) && !player->isDashing && player->cooldownTimeCounter <= 0.0f && (player->comboStep == 0 || !player->hasHitEnemy) && player->stamina >= 25.0f) {
        player->isDashing = true;
        player->dashTimeCounter = 0.0f;
        player->isHitboxActive = false;
        player->cooldownattackTimer = 0.0f;
        player->cooldowncomboWindowTimer = 0.0f;
        player->dashDirection = (inputDir.x == 0.0f && inputDir.y == 0.0f) ? player->lastMovingDir : inputDir;
        PlayerUseStamina(player, 25.0f); // Consome 25 pontos de estamina
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && player->comboStep == 0 && !player->isDashing && player->stamina >= 15.0f) {
        player->comboStep = 1;
        player->cooldownattackTimer = 0.0f;
        player->cooldowncomboWindowTimer = 0.0f;
        player->hasHitEnemy = false;
        PlayerUseStamina(player, 15.0f); // Consome 15 pontos de estamina por ataque
    }

    if (player->comboStep > 0) {
        if (player->cooldownattackTimer < player->attackTimer) {
            player->cooldownattackTimer += deltaTime;
            player->isHitboxActive = true;
            float distFront = 0.0f;
            if (player->comboStep == 1 || player->comboStep == 2) {
                player->hitboxRadius = tw * 0.7f;
                distFront = tw * 0.4f;
            } else if (player->comboStep == 3) {
                player->hitboxRadius = tw * 0.3f;
                distFront = tw * 0.8f;
            }
            player->hitboxCenter.x = player->pos.x + (player->lastMovingDir.x * distFront);
            player->hitboxCenter.y = player->pos.y + (player->lastMovingDir.y * distFront);
        } else {
            player->isHitboxActive = false;
            player->cooldowncomboWindowTimer += deltaTime;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && player->comboStep < 3) {
                player->comboStep += 1;
                player->cooldownattackTimer = 0.0f;
                player->cooldowncomboWindowTimer = 0.0f;
                player->hasHitEnemy = false;
            } else if (player->cooldowncomboWindowTimer >= player->comboWindowTimer) {
                player->comboStep = 0;
                player->cooldownattackTimer = 0.0f;
                player->cooldowncomboWindowTimer = 0.0f;
            }
        }
    } else {
        player->isHitboxActive = false;
    }

    float velX = 0;
    float velY = 0;

    if (player->isDashing) {
        velX = player->dashDirection.x * (player->normalSpeed * player->dashSpeedMultiplier) * deltaTime;
        velY = player->dashDirection.y * (player->normalSpeed * player->dashSpeedMultiplier) * deltaTime;
        
        player->dashTimeCounter += deltaTime;
        if (player->dashTimeCounter >= player->dashDuration) {
            player->isDashing = false;
            player->cooldownTimeCounter = player->dashCooldown;
        }
    } else if (player->comboStep == 3) {
        velX = player->lastMovingDir.x * (player->normalSpeed * 1.5f) * deltaTime;
        velY = player->lastMovingDir.y * (player->normalSpeed * 1.5f) * deltaTime;
    } else if (player->comboStep == 0) {
        velX = inputDir.x * player->normalSpeed * deltaTime;
        velY = inputDir.y * player->normalSpeed * deltaTime;
    }

    float novo_x = player->pos.x + velX;
    float novo_y = player->pos.y + velY;

    if (velX != 0.0f && PodeMoverPara(level, novo_x, player->pos.y, r, tw, th)) {
        player->pos.x = novo_x;
    }
    if (velY != 0.0f && PodeMoverPara(level, player->pos.x, novo_y, r, tw, th)) {
        player->pos.y = novo_y;
    }
}

void DrawPlayer(Player player) {
    if (!player.alive) return;

    Color playerOuterColor = MAROON;
    if (player.isDashing) playerOuterColor = WHITE;
    else if (player.cooldownTimeCounter > 0.0f) playerOuterColor = DARKGRAY;

    DrawCircleV(player.pos, player.raio, playerOuterColor);
    DrawCircleGradient(player.pos, player.raio * 0.65f, ORANGE, playerOuterColor);
    if (player.isHitboxActive) {
        DrawCircleLines((int)player.hitboxCenter.x, (int)player.hitboxCenter.y, player.hitboxRadius, RED);
    }
}

// HUD - Funções de Vida e Estamina
void PlayerTakeDamage(Player *player, int damage) {
    player->hp -= damage;
    if (player->hp < 0) player->hp = 0;
    if (player->hp == 0) {
        player->alive = false;
        // cancelar ações
        player->isDashing = false;
        player->isHitboxActive = false;
    }
}

void PlayerUseStamina(Player *player, float amount) {
    player->stamina -= amount;
    if (player->stamina < 0.0f) player->stamina = 0.0f;
    
    // Reset do delay de recuperação quando consome estamina
    player->staminaRecoveryDelayCounter = 0.0f;
}

void UpdatePlayerStamina(Player *player, float deltaTime) {
    // Aumentar o contador de delay
    if (player->staminaRecoveryDelayCounter < player->staminaRecoveryDelay) {
        player->staminaRecoveryDelayCounter += deltaTime;
    } else {
        // Se passou 2 segundos sem atacar/fazer dash, recuperar estamina
        player->stamina += player->staminaRecoveryRate * deltaTime;
        if (player->stamina > player->staminaMax) {
            player->stamina = player->staminaMax;
        }
    }
}

void DrawPlayerHUD(const Player *player, int x, int y) {
    // --- VIDA ---
    DrawText(TextFormat("HP: %d/%d", player->hp, player->hpMax), x, y, 20, LIGHTGRAY);
    
    const int barW = 220;
    const int barH = 14;
    const int barY = y + 24;
    
    // Barra de vida - fundo
    DrawRectangle(x, barY, barW, barH, (Color){ 40, 40, 40, 220 });
    DrawRectangleLines(x, barY, barW, barH, (Color){ 110, 110, 110, 255 });
    
    // Barra de vida - preenchimento
    float hpRatio = (float)player->hp / (float)player->hpMax;
    if (hpRatio < 0.0f) hpRatio = 0.0f;
    if (hpRatio > 1.0f) hpRatio = 1.0f;
    
    int hpFillW = (int)(barW * hpRatio);
    Color hpFill = (hpRatio > 0.5f) ? GREEN : (hpRatio > 0.25f ? ORANGE : RED);
    DrawRectangle(x + 1, barY + 1, hpFillW - 2 > 0 ? hpFillW - 2 : 0, barH - 2, hpFill);
    
    // --- ESTAMINA ---
    int staminaY = barY + barH + 20;
    DrawText(TextFormat("STAMINA: %.0f/%.0f", player->stamina, player->staminaMax), x, staminaY, 20, LIGHTGRAY);
    
    const int staminaBarY = staminaY + 24;
    
    // Barra de estamina - fundo
    DrawRectangle(x, staminaBarY, barW, barH, (Color){ 40, 40, 40, 220 });
    DrawRectangleLines(x, staminaBarY, barW, barH, (Color){ 110, 110, 110, 255 });
    
    // Barra de estamina - preenchimento
    float staminaRatio = player->stamina / player->staminaMax;
    if (staminaRatio < 0.0f) staminaRatio = 0.0f;
    if (staminaRatio > 1.0f) staminaRatio = 1.0f;
    
    int staminaFillW = (int)(barW * staminaRatio);
    DrawRectangle(x + 1, staminaBarY + 1, staminaFillW - 2 > 0 ? staminaFillW - 2 : 0, barH - 2, YELLOW);
}