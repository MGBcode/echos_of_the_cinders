#include "raylib.h"
#include "raymath.h"

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Echos of the Cinders - Movimentacao e Dash");

    // Variáveis base do jogador
    Vector2 playerPos = { (float)screenWidth/2, (float)screenHeight/2 };
    float normalSpeed = 200.0f; // esse número representa a vélocidade base do jogador,ele tá em pixels por segundo

    // váriaveis do dash
    bool isDashing = false;           
    float dashSpeedMultiplier = 4.0f; 
    float dashDuration = 0.20f;       
    float dashCooldown = 0.5f;        
    
    float dashTimeCounter = 0.0f;     
    float cooldownTimeCounter = 0.0f; 
    Vector2 dashDirection = { 0, 0 };
    // -------------------------

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // serve pra atualizar o tempo do jogo
        float deltaTime = GetFrameTime(); 

        //atualiza os cronometros
        if (!isDashing && cooldownTimeCounter > 0.0f) {
            cooldownTimeCounter -= deltaTime; // Vai diminuindo a recarga até chegar a zero
        }

        // inputs que o player pode dar
        Vector2 inputDir = { 0.0f, 0.0f };
        if (IsKeyDown(KEY_D)) inputDir.x += 1.0f;
        if (IsKeyDown(KEY_A))  inputDir.x -= 1.0f;
        if (IsKeyDown(KEY_S))  inputDir.y += 1.0f;
        if (IsKeyDown(KEY_W))    inputDir.y -= 1.0f;

        // Normaliza o vetor (Evita que andar na diagonal seja mais rápido)
        inputDir = Vector2Normalize(inputDir);

        // ativação do dash
        // Verifica se apertou ESPAÇO, se não está no meio de um dash, e se a recarga zerou
        if (IsKeyPressed(KEY_SPACE) && !isDashing && cooldownTimeCounter <= 0.0f) {
            isDashing = true;
            dashTimeCounter = 0.0f; // Zera o tempo do dash atual
            
            // Define para onde o dash vai. Se estiver parado, vai para a direita por padrão
            if (inputDir.x == 0.0f && inputDir.y == 0.0f) {
                dashDirection = (Vector2){ 1.0f, 0.0f }; 
            } else {
                dashDirection = inputDir;
            }
        }

        // --- 4. APLICAÇÃO DO MOVIMENTO ---
        if (isDashing) {
            // Lógica enquanto o dash está acontecendo
            playerPos.x += dashDirection.x * (normalSpeed * dashSpeedMultiplier) * deltaTime;
            playerPos.y += dashDirection.y * (normalSpeed * dashSpeedMultiplier) * deltaTime;

            dashTimeCounter += deltaTime; // Atualiza o tempo que passou
            
            if (dashTimeCounter >= dashDuration) {
                // Acabou o dash
                isDashing = false;
                cooldownTimeCounter = dashCooldown; // Inicia a recarga
            }
        } else {
            // Lógica de andar normal
            playerPos.x += inputDir.x * normalSpeed * deltaTime;
            playerPos.y += inputDir.y * normalSpeed * deltaTime;
        }

        // desenho das coisas
        BeginDrawing();
            ClearBackground(BLACK);

            // Muda a cor do cavaleiro dependendo dele estar no dash ou recarregando
            Color playerOuterColor = MAROON;
            if (isDashing) {
                playerOuterColor = WHITE; 
            } else if (cooldownTimeCounter > 0.0f) {
                playerOuterColor = DARKGRAY; 
            }

            DrawCircleV(playerPos, 15, playerOuterColor);
            DrawCircleGradient(playerPos, 10.0f, ORANGE, playerOuterColor);

            // Textos para ajudar a ver os valores rodando
            DrawText("Aperte ESPACO para Dash", 20, 20, 20, LIGHTGRAY);
            DrawText(TextFormat("Cooldown: %.2f", cooldownTimeCounter), 20, 50, 20, RED);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}