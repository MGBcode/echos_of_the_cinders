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
    // HUD - Vida e Estamina
    int hp;
    int hpMax;
    float stamina;
    float staminaMax;
    float staminaRecoveryDelay;
    float staminaRecoveryDelayCounter;
    float staminaRecoveryRate;
    // Estado de vida
    bool alive;
} Player;

void InitPlayer(Player *player, int screenWidth, int screenHeight);

void UpdatePlayer(Player *player, float deltaTime, Level *level);

void DrawPlayer(Player player);

// Funções de HUD - Vida e Estamina
void PlayerTakeDamage(Player *player, int damage);
void PlayerUseStamina(Player *player, float amount);
void UpdatePlayerStamina(Player *player, float deltaTime);
void DrawPlayerHUD(const Player *player, int x, int y);

#endif