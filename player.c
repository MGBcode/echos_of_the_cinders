#include "player.h"
#include "raymath.h"

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
}
void UpdatePlayer(Player *player, float deltaTime) {
    if (!player->isDashing && player->cooldownTimeCounter > 0.0f) {
        player->cooldownTimeCounter -= deltaTime;
    }

    Vector2 inputDir = { 0.0f, 0.0f };
    if (IsKeyDown(KEY_D)) inputDir.x += 1.0f;
    if (IsKeyDown(KEY_A))  inputDir.x -= 1.0f;
    if (IsKeyDown(KEY_S))  inputDir.y += 1.0f;
    if (IsKeyDown(KEY_W))    inputDir.y -= 1.0f;
    inputDir = Vector2Normalize(inputDir);

    bool isInputDiagonal = (inputDir.x != 0.0f && inputDir.y != 0.0f);
    
    if (inputDir.x != 0.0f || inputDir.y != 0.0f) {
        
        if (isInputDiagonal) {
            player->lastMovingDir = inputDir;
            player->diagonalBufferTimer = 0; 
        } else {
            player->diagonalBufferTimer += deltaTime;
            bool wasLastDirDiagonal = (player->lastMovingDir.x != 0.0f && player->lastMovingDir.y != 0.0f);
            
            if (player->diagonalBufferTimer > 0.05f || !wasLastDirDiagonal) {
                player->lastMovingDir = inputDir;
            }
        }
    } else {
        player->diagonalBufferTimer = 0;
    }
    if (IsKeyPressed(KEY_SPACE) && !player->isDashing && player->cooldownTimeCounter <= 0.0f && player->cooldowncomboWindowTimer == 0.0f) {
        player->isDashing = true;
        player->dashTimeCounter = 0.0f;
        if (inputDir.x == 0.0f && inputDir.y == 0.0f) {
            player->dashDirection = player->lastMovingDir; 
        } else {
            player->dashDirection = inputDir;
        }
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && player->comboStep == 0 && !player->isDashing) {
        player->comboStep = 1;
        player->cooldownattackTimer = 0.0f;
        player->cooldowncomboWindowTimer = 0.0f;
    }
    if (player->comboStep > 0) {
        
        if (player->cooldownattackTimer < player->attackTimer) {
            player->cooldownattackTimer += deltaTime;
            player->isHitboxActive = true;
            float distFront = 0.0f;
            if (player->comboStep == 1 || player->comboStep == 2) {
                player->hitboxRadius = 35.0f;
                distFront = 20.0f;
            } else if (player->comboStep == 3) {
                player->hitboxRadius = 15.0f;
                distFront = 40.0f;
            }
            player->hitboxCenter.x = player->pos.x + (player->lastMovingDir.x * distFront);
            player->hitboxCenter.y = player->pos.y + (player->lastMovingDir.y * distFront);

            // FUTURO: Aqui usaremos CheckCollisionCircleRec(player->hitboxCenter, player->hitboxRadius, bossRec)

        }
        else {
            player->isHitboxActive = false;
            player->cooldowncomboWindowTimer += deltaTime;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && player->comboStep < 3) {
                player->comboStep += 1;
                player->cooldownattackTimer = 0.0f;      
                player->cooldowncomboWindowTimer = 0.0f; 
            }
            else if (player->cooldowncomboWindowTimer >= player->comboWindowTimer) {
                player->comboStep = 0; 
                player->cooldownattackTimer = 0.0f;
                player->cooldowncomboWindowTimer = 0.0f;
            }
        }
    } else {
        player->isHitboxActive = false;
    }
    
    if (player->isDashing) {
        player->pos.x += player->dashDirection.x * (player->normalSpeed * player->dashSpeedMultiplier) * deltaTime;
        player->pos.y += player->dashDirection.y * (player->normalSpeed * player->dashSpeedMultiplier) * deltaTime;
        player->dashTimeCounter += deltaTime;
        
        if (player->dashTimeCounter >= player->dashDuration) {
            player->isDashing = false;
            player->cooldownTimeCounter = player->dashCooldown;
        }
    } 
    else if (player->comboStep == 3) {
        player->pos.x += player->lastMovingDir.x * (player->normalSpeed * 1.5f) * deltaTime;
        player->pos.y += player->lastMovingDir.y * (player->normalSpeed * 1.5f) * deltaTime;
    } 
    else if (player->comboStep == 0) {
        player->pos.x += inputDir.x * player->normalSpeed * deltaTime;
        player->pos.y += inputDir.y * player->normalSpeed * deltaTime;
    }
}

void DrawPlayer(Player player) {
    Color playerOuterColor = MAROON;
    if (player.isDashing) playerOuterColor = WHITE;
    else if (player.cooldownTimeCounter > 0.0f) playerOuterColor = DARKGRAY;

    DrawCircleV(player.pos, 15, playerOuterColor);
    DrawCircleGradient(player.pos, 10.0f, ORANGE, playerOuterColor);
    if (player.isHitboxActive) {
        DrawCircleLines((int)player.hitboxCenter.x, (int)player.hitboxCenter.y, player.hitboxRadius, RED);
    }
}