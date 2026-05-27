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
    int lastHorizontalInput;
    int lastVerticalInput;
    float diagonalBufferTimer;
    int comboStep;
    int attackState;
    float attackStateTimer;
    float attackWindup[3];
    float attackActive[3];
    float attackRecovery[3];
    float comboWindowTimer;
    Vector2 hitboxCenter;
    float hitboxRadius;
    bool isHitboxActive;
    bool hasHitEnemy;
    bool isParrying;
    float parryTimer;
    float parryDuration;
    int frascosMax;
    int frascosAtuais;
    bool isHealing;
    float healingTimer;
    int hp;
    int hpMax;
    float stamina;
    float staminaMax;
    float staminaRecoveryDelay;
    float staminaRecoveryDelayCounter;
    float staminaRecoveryRate;
    bool alive;
} Player;

void InitPlayer(Player *player, float tileX, float tileY, Level *level);

void UpdatePlayer(Player *player, float deltaTime, Level *level);

void DrawPlayer(Player player);

void PlayerTakeDamage(Player *player, int damage);
void PlayerTakeDamage_IgnoreParry(Player *player, int damage);
void PlayerUseStamina(Player *player, float amount);
void UpdatePlayerStamina(Player *player, float deltaTime);
void DrawPlayerHUD(const Player *player, int x, int y);

#endif