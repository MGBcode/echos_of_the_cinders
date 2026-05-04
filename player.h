#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"

typedef struct {
    Vector2 pos;
    float normalSpeed;
    bool isDashing;
    float dashSpeedMultiplier;
    float dashDuration;
    float dashCooldown;
    float dashTimeCounter;
    float cooldownTimeCounter;
    Vector2 dashDirection;
} Player;

void InitPlayer(Player *player, int screenWidth, int screenHeight);
void UpdatePlayer(Player *player, float deltaTime);
void DrawPlayer(Player player);

#endif