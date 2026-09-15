#pragma once
#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#define SCREEN_W 1280
#define SCREEN_H 720

#define CANVAS_W 1920
#define CANVAS_H 1080

#define MAX_LAYERS 6
#define MAX_UNDO   10

#define TOPBAR_H   72
#define PANEL_W    260

static inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
static inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
static inline float distf(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0;
    return sqrtf(dx * dx + dy * dy);
}

/* --- shared UI palette (dark, "cool" flat theme) --- */
#define COL_BG        (SDL_Color){ 18,  18,  22, 255}
#define COL_PANEL     (SDL_Color){ 26,  27,  34, 235}
#define COL_PANEL_ALT (SDL_Color){ 34,  35,  44, 235}
#define COL_ACCENT    (SDL_Color){ 96, 175, 255, 255}
#define COL_ACCENT2   (SDL_Color){255, 110, 170, 255}
#define COL_TEXT      (SDL_Color){230, 232, 240, 255}
#define COL_TEXT_DIM  (SDL_Color){140, 143, 158, 255}
#define COL_DANGER    (SDL_Color){255,  92,  92, 255}
#define COL_OK        (SDL_Color){110, 220, 150, 255}
#define COL_BORDER    (SDL_Color){ 60,  62,  75, 255}
