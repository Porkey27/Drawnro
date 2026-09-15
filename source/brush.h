#pragma once
#include "common.h"
#include "canvas.h"

typedef enum {
    BRUSH_HARD = 0,
    BRUSH_SOFT,
    BRUSH_AIRBRUSH,
    BRUSH_CALLIGRAPHY,
    BRUSH_PENCIL,
    BRUSH_ERASER,
    BRUSH_COUNT
} BrushType;

extern const char *BRUSH_NAMES[BRUSH_COUNT];

typedef struct {
    BrushType type;
    float size;       /* radius in canvas px, 1..300 */
    float opacity;    /* 0..1 */
    float hardness;   /* 0..1, edge softness for BRUSH_SOFT */
    float spacing;    /* fraction of diameter between stamps, 0.02..1 */
    SDL_Color color;
    bool  symmetryX;
    bool  symmetryY;
    bool  pressureSim; /* fake pressure from stroke speed */
} BrushSettings;

typedef struct {
    bool  active;
    float lastX, lastY;
    float leftover;    /* distance already covered past the previous stamp */
    float lastSpeed;
} StrokeState;

bool brush_init(SDL_Renderer *r);
void brush_destroy(void);

void brush_strokeBegin(StrokeState *s, float x, float y);
/* stamps along the segment from the stroke's last point to (x,y) */
void brush_strokeTo(SDL_Renderer *r, Canvas *c, int layerIdx, BrushSettings *b, StrokeState *s, float x, float y);
void brush_strokeEnd(StrokeState *s);
