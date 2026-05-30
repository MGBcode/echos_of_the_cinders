#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"
#include "level.h"

//Vamos criar uma estrutura para armazernar as texturas das direções de cada estado.
//No caso, aqui, estaremos pegando o índice que corresponde a um arquivo png.
typedef struct{
    //As texturas vão ser organizadas primeiro pelo estado do player(idle, walk, attack, etc) e depois pela direção.
    //Texture2D é a estrutura de textura do Raylib, e aqui estamos criando arrays de 8 texturas para cada estado, correspondendo às direções.
    //As arrays são de tamanho 8 porque vamos pegar as 8 direções do pacote de sprites.
    Texture2D idle[8];
    Texture2D walk[8];
    Texture2D attack[8];
    Texture2D comboAttack[8];
    Texture2D heavyAttack[8];
    Texture2D die[8];
    Texture2D dash[8];
    Texture2D parry[8];
    Texture2D heal[8];

    //Aqui estamos armazenando o número de colunas, linhas e frames totais de cada animação, para facilitar na hora de configurar o sprite.
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

    //Já aqui estamos armazenando a velocidade de animação de cada estado(Tipo de animação), para facilitar na hora de configurar o sprite.
    float idleAnimSpeed;
    float walkAnimSpeed;
    float attackAnimSpeed;
    float comboAttackAnimSpeed;
    float heavyAttackAnimSpeed;
    float dieAnimSpeed;
    float dashAnimSpeed;
    float parryAnimSpeed;
    float healAnimSpeed;
} PlayerTextures; //PlayerTextures é a estrutura que armazena todas as texturas do player, organizadas por estado e direção, além de informações sobre a configuração dos sprites (número de frames, velocidade de animação, etc).

//A estrutura PlayerState é um enum que define os estados de animação do player.
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
    Vector2 attackDirection;
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

    PlayerTextures *textures; //Um ponteiro para a estrutura Playertextures, que contém todas as texturas do player. E permite acessar as texturas de forma organizada.
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
//UpdatePlayer agora recebe o Level para acessar informações como o tamanho do tile, que é importante para calcular o raio do player e outras mecânicas relacionadas ao mapa.
void UpdatePlayer(Player *player, float deltaTime, Level *level);
//DrawPlayer recebe o Player por valor, pois não precisa modificar o estado do player, apenas ler suas informações para desenhá-lo. E isso evita a necessidade de usar ponteiros e desreferenciamento, deixando o código mais limpo.
void DrawPlayer(Player player);

//Funções para lidar com dano, stamina e HUD do player:
void PlayerTakeDamage(Player *player, int damage);
void PlayerTakeDamage_IgnoreParry(Player *player, int damage);
void PlayerUseStamina(Player *player, float amount);
//UpdatePlayerStamina é a função que lida com a recuperação de stamina do player, e é chamada dentro do UpdatePlayer para atualizar a stamina a cada frame.
void UpdatePlayerStamina(Player *player, float deltaTime);
//DrawPlayerHUD é a função responsável por desenhar a interface de vida, stamina e frascos do player na tela. Ela recebe o Player por ponteiro, pois precisa acessar suas informações para desenhar o HUD corretamente.
void DrawPlayerHUD(const Player *player, int x, int y);

#endif