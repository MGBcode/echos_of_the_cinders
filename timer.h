#ifndef TIMER_H
#define TIMER_H

#include "raylib.h"
#include <stdbool.h>

#define MIN_VALID_TIME 1.0f

typedef struct {
    float currentTime;
    bool running;
} TimerData;

typedef struct {
    char initials[4];
    float time;
} ScoreEntry;


void Timer_Reset(TimerData *timer);
void Timer_Start(TimerData *timer);
void Timer_Stop(TimerData *timer);
void Timer_Update(TimerData *timer, float dt);
void DrawTimer(const TimerData *timer);


void SaveScore(const char *iniciais, float tempo);
void LoadScores(ScoreEntry scores[], int *count);


static inline bool IsValidTime(float tempo) {
    return tempo >= MIN_VALID_TIME;
}

#endif