#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"
#include "level.h"

typedef struct {
    Vector2 pos;
    Vector2 tile_pos;
    float raio;
    float normalSpeed;
    bool isDashing;
    float dashSpeedMultiplier;
    float dashDuration;
    float dashCooldown;
    float dashTimeCounter;
    float cooldownTimeCounter;
    Vector2 dashDirection;
    Vector2 lastMovingDir;
    float diagonalBufferTimer;
    int comboStep;
    float attackTimer;
    float cooldownattackTimer;
    float cooldowncomboWindowTimer;
    float comboWindowTimer;
    Vector2 hitboxCenter;
    float hitboxRadius;
    bool isHitboxActive;
    bool hasHitEnemy;
} Player;

void InitPlayer(Player *player, int screenWidth, int screenHeight);

void UpdatePlayer(Player *player, float deltaTime, Level *level); 

void DrawPlayer(Player player);

#endif