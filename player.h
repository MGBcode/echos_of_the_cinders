#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"
#include "level.h"

//Vamos criar uma estrutura para armazernar as texturas das direções de cada estado.
//No caso, aqui, estaremos pegando o índice que corresponde a um arquivo png.
typedef struct{
    Texture2D idle[8];
    Texture2D walk[8];
    Texture2D attack[8];
    Texture2D comboAttack[8];
    Texture2D heavyAttack[8];
    Texture2D die[8];
    Texture2D dash[8];
    Texture2D parry[8];
    Texture2D heal[8];

    int idleFrameCols;
    int idleFrameRows;
    int idleTotalFrames;
    int walkFrameCols;
    int walkFrameRows;
    int walkTotalFrames;
    int attackFrameCols;
    int attackFrameRows;
    int attackTotalFrames;
    int comboAttackFrameCols;
    int comboAttackFrameRows;
    int comboAttackTotalFrames;
    int heavyAttackFrameCols;
    int heavyAttackFrameRows;
    int heavyAttackTotalFrames;
    int dieFrameCols;
    int dieFrameRows;
    int dieTotalFrames;
    int dashFrameCols;
    int dashFrameRows;
    int dashTotalFrames;
    int parryFrameCols;
    int parryFrameRows;
    int parryTotalFrames;
    int healFrameCols;
    int healFrameRows;
    int healTotalFrames;

    float idleAnimSpeed;
    float walkAnimSpeed;
    float attackAnimSpeed;
    float comboAttackAnimSpeed;
    float heavyAttackAnimSpeed;
    float dieAnimSpeed;
    float dashAnimSpeed;
    float parryAnimSpeed;
    float healAnimSpeed;
} PlayerTextures;

typedef enum {
    PLAYER_STATE_IDLE,
    PLAYER_STATE_HEAVY_ATTACK
} PlayerState;

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
    PlayerState state;
    float heavyChargeTimer;
    int currentAttackDamage;
    bool isChargingHeavy;
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
    float baseStaminaRecoveryDelay;
    float baseStaminaRecoveryRate;
    bool isExhausted;
    bool alive;

    PlayerTextures *textures;
    Texture2D *activeTexture; //Ponteiro para a textura rodando agora.
    Rectangle frameRec; //Retângulo de corte do frame atual.
    int currentFrame; //Frame atual da imagem.
    int activeMaxFrames; //Quantos frames tem a imagem atual.
    float frameTimer; //Cronômetro para trocar frame.
    int currentDirIndex; //Direção atual(0 a 7).
    Vector2 spriteAnchorOffset; //Offset visual do sprite relativo ao ponto físico do player.
} Player;

//Adicionando ponteiro para PlayerTextures na inicialização.
void InitPlayer(Player *player, float tileX, float tileY, Level *level, PlayerTextures *textures);

void UpdatePlayer(Player *player, float deltaTime, Level *level);

void DrawPlayer(Player player);

void PlayerTakeDamage(Player *player, int damage);
void PlayerTakeDamage_IgnoreParry(Player *player, int damage);
void PlayerUseStamina(Player *player, float amount);
void UpdatePlayerStamina(Player *player, float deltaTime);
void DrawPlayerHUD(const Player *player, int x, int y);

#endif