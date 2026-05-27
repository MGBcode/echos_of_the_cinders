#include "timer.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_SCORES 10
#define SCORE_FILE "scores.txt"

void Timer_Reset(TimerData *timer) {
    timer->currentTime = 0.0f;
    timer->running = false;
}

void Timer_Start(TimerData *timer) {
    timer->currentTime = 0.0f;
    timer->running = true;
}

void Timer_Stop(TimerData *timer) {
    timer->running = false;
}

void Timer_Update(TimerData *timer, float dt) {
    if (timer->running) {
        timer->currentTime += dt;
    }
}

void DrawTimer(const TimerData *timer) {

    int minutos = (int)(timer->currentTime / 60.0f);
    int segundos = (int)timer->currentTime % 60;

    char texto[64];

    sprintf(texto,
            "TEMPO: %02d:%02d",
            minutos,
            segundos);

    DrawText(texto,
             GetScreenWidth() - 220,
             40,
             28,
             GOLD);
}





void SaveScore(const char *iniciais, float tempo) {
    if (tempo < 1.0f) {
        return;
    }

    ScoreEntry scores[MAX_SCORES];
    int count = 0;
    LoadScores(scores, &count);

    if (count < MAX_SCORES) {
        strcpy(scores[count].initials, iniciais);
        scores[count].time = tempo;
        count++;
    } else if (tempo < scores[MAX_SCORES - 1].time) {
        strcpy(scores[MAX_SCORES - 1].initials, iniciais);
        scores[MAX_SCORES - 1].time = tempo;
    } else {
        return;
    }

    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (scores[j].time < scores[i].time) {
                ScoreEntry temp = scores[i];
                scores[i] = scores[j];
                scores[j] = temp;
            }
        }
    }

    FILE *file = fopen(SCORE_FILE, "w");

    if (file == NULL) {
        return;
    }

    for (int i = 0; i < count; i++) {
        fprintf(file,
                "%s %.2f\n",
                scores[i].initials,
                scores[i].time);
    }

    fclose(file);
}

void LoadScores(ScoreEntry scores[], int *count) {
    *count = 0;
    FILE *file = fopen(SCORE_FILE, "r");

    if (file == NULL) {
        return;
    }

    while ((*count < MAX_SCORES) &&
           fscanf(file,
                  "%3s %f",
                  scores[*count].initials,
                  &scores[*count].time) == 2)
    {
        (*count)++;
    }

    fclose(file);

    for (int i = 0; i < *count - 1; i++) {
        for (int j = i + 1; j < *count; j++) {
            if (scores[j].time < scores[i].time)
            {
                ScoreEntry temp = scores[i];
                scores[i] = scores[j];
                scores[j] = temp;
            }
        }
    }
}