#pragma once
#include "common.h"

typedef struct {
    SDL_Texture *tex[MAX_LAYERS];
    bool         visible[MAX_LAYERS];
    float        opacity[MAX_LAYERS];
    char         name[MAX_LAYERS][24];
    int          layerCount;
    int          active;

    int   width, height;
    float camX, camY;   /* canvas-space point currently at screen center */
    float zoom;          /* screen px per canvas px */

    /* transparency checker pattern, drawn once behind layer 0 */
    SDL_Texture *checker;
} Canvas;

bool canvas_init(Canvas *c, SDL_Renderer *r, int w, int h);
void canvas_destroy(Canvas *c);

int  canvas_addLayer(Canvas *c, SDL_Renderer *r);
void canvas_removeLayer(Canvas *c, int idx);
void canvas_clearLayer(Canvas *c, SDL_Renderer *r, int idx);

void canvas_screenToCanvas(Canvas *c, float sx, float sy, float sw, float sh, float *cx, float *cy);
void canvas_canvasToScreen(Canvas *c, float cx, float cy, float sw, float sh, float *sx, float *sy);

void canvas_pan(Canvas *c, float dxScreen, float dyScreen);
void canvas_zoomAt(Canvas *c, float screenX, float screenY, float sw, float sh, float factor);
void canvas_resetView(Canvas *c, float sw, float sh);

void canvas_render(Canvas *c, SDL_Renderer *r, SDL_Rect viewport);

/* full-texture snapshot helpers used by the undo system */
SDL_Texture *canvas_snapshotLayer(Canvas *c, SDL_Renderer *r, int idx);
void canvas_restoreLayer(Canvas *c, SDL_Renderer *r, int idx, SDL_Texture *snapshot);
