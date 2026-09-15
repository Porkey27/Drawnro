#include "canvas.h"

static SDL_Texture *makeTargetTexture(SDL_Renderer *r, int w, int h, bool clear) {
    SDL_Texture *t = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888,
                                        SDL_TEXTUREACCESS_TARGET, w, h);
    if (!t) return NULL;
    SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    if (clear) {
        SDL_Texture *prevTarget = SDL_GetRenderTarget(r);
        SDL_SetRenderTarget(r, t);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
        SDL_RenderClear(r);
        SDL_SetRenderTarget(r, prevTarget);
    }
    return t;
}

static SDL_Texture *makeChecker(SDL_Renderer *r, int w, int h) {
    SDL_Texture *t = makeTargetTexture(r, w, h, false);
    SDL_Texture *prev = SDL_GetRenderTarget(r);
    SDL_SetRenderTarget(r, t);
    SDL_SetRenderDrawColor(r, 40, 41, 48, 255);
    SDL_RenderClear(r);
    SDL_SetRenderDrawColor(r, 52, 53, 62, 255);
    int cell = 24;
    for (int y = 0; y < h; y += cell) {
        for (int x = 0; x < w; x += cell) {
            bool even = ((x / cell) + (y / cell)) % 2 == 0;
            if (even) {
                SDL_Rect rc = { x, y, cell, cell };
                SDL_RenderFillRect(r, &rc);
            }
        }
    }
    SDL_SetRenderTarget(r, prev);
    return t;
}

bool canvas_init(Canvas *c, SDL_Renderer *r, int w, int h) {
    memset(c, 0, sizeof(*c));
    c->width = w; c->height = h;
    c->zoom = 1.0f;
    c->camX = w / 2.0f;
    c->camY = h / 2.0f;
    c->checker = makeChecker(r, w, h);
    canvas_addLayer(c, r);
    return c->layerCount > 0;
}

void canvas_destroy(Canvas *c) {
    for (int i = 0; i < c->layerCount; i++)
        if (c->tex[i]) SDL_DestroyTexture(c->tex[i]);
    if (c->checker) SDL_DestroyTexture(c->checker);
}

int canvas_addLayer(Canvas *c, SDL_Renderer *r) {
    if (c->layerCount >= MAX_LAYERS) return -1;
    int idx = c->layerCount++;
    c->tex[idx] = makeTargetTexture(r, c->width, c->height, true);
    c->visible[idx] = true;
    c->opacity[idx] = 1.0f;
    snprintf(c->name[idx], sizeof(c->name[idx]), "Layer %d", idx + 1);
    c->active = idx;
    return idx;
}

void canvas_removeLayer(Canvas *c, int idx) {
    if (c->layerCount <= 1 || idx < 0 || idx >= c->layerCount) return;
    SDL_DestroyTexture(c->tex[idx]);
    for (int i = idx; i < c->layerCount - 1; i++) {
        c->tex[i] = c->tex[i + 1];
        c->visible[i] = c->visible[i + 1];
        c->opacity[i] = c->opacity[i + 1];
        memcpy(c->name[i], c->name[i + 1], sizeof(c->name[i]));
    }
    c->layerCount--;
    if (c->active >= c->layerCount) c->active = c->layerCount - 1;
}

void canvas_clearLayer(Canvas *c, SDL_Renderer *r, int idx) {
    SDL_Texture *prev = SDL_GetRenderTarget(r);
    SDL_SetRenderTarget(r, c->tex[idx]);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
    SDL_RenderClear(r);
    SDL_SetRenderTarget(r, prev);
}

void canvas_screenToCanvas(Canvas *c, float sx, float sy, float sw, float sh, float *cx, float *cy) {
    *cx = c->camX + (sx - sw / 2.0f) / c->zoom;
    *cy = c->camY + (sy - sh / 2.0f) / c->zoom;
}

void canvas_canvasToScreen(Canvas *c, float cx, float cy, float sw, float sh, float *sx, float *sy) {
    *sx = sw / 2.0f + (cx - c->camX) * c->zoom;
    *sy = sh / 2.0f + (cy - c->camY) * c->zoom;
}

void canvas_pan(Canvas *c, float dxScreen, float dyScreen) {
    c->camX -= dxScreen / c->zoom;
    c->camY -= dyScreen / c->zoom;
}

void canvas_zoomAt(Canvas *c, float screenX, float screenY, float sw, float sh, float factor) {
    float cx, cy;
    canvas_screenToCanvas(c, screenX, screenY, sw, sh, &cx, &cy);
    c->zoom = clampf(c->zoom * factor, 0.1f, 16.0f);
    float nx, ny;
    canvas_screenToCanvas(c, screenX, screenY, sw, sh, &nx, &ny);
    c->camX += (cx - nx);
    c->camY += (cy - ny);
}

void canvas_resetView(Canvas *c, float sw, float sh) {
    c->zoom = fminf(sw / (float)c->width, sh / (float)c->height) * 0.92f;
    c->camX = c->width / 2.0f;
    c->camY = c->height / 2.0f;
}

void canvas_render(Canvas *c, SDL_Renderer *r, SDL_Rect viewport) {
    SDL_RenderSetClipRect(r, &viewport);
    float sw = viewport.w, sh = viewport.h;
    float sx0, sy0;
    canvas_canvasToScreen(c, 0, 0, sw, sh, &sx0, &sy0);
    SDL_FRect dst = { viewport.x + sx0, viewport.y + sy0, c->width * c->zoom, c->height * c->zoom };

    /* checker background behind everything so transparency reads clearly */
    SDL_SetTextureAlphaMod(c->checker, 255);
    SDL_RenderCopyF(r, c->checker, NULL, &dst);

    for (int i = 0; i < c->layerCount; i++) {
        if (!c->visible[i]) continue;
        SDL_SetTextureAlphaMod(c->tex[i], (Uint8)(clampf(c->opacity[i], 0, 1) * 255));
        SDL_RenderCopyF(r, c->tex[i], NULL, &dst);
    }

    /* canvas border */
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, COL_BORDER.r, COL_BORDER.g, COL_BORDER.b, 255);
    SDL_Rect border = { (int)dst.x, (int)dst.y, (int)dst.w, (int)dst.h };
    SDL_RenderDrawRect(r, &border);

    SDL_RenderSetClipRect(r, NULL);
}

SDL_Texture *canvas_snapshotLayer(Canvas *c, SDL_Renderer *r, int idx) {
    SDL_Texture *snap = makeTargetTexture(r, c->width, c->height, false);
    SDL_Texture *prev = SDL_GetRenderTarget(r);
    SDL_SetRenderTarget(r, snap);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
    SDL_RenderClear(r);
    SDL_SetTextureBlendMode(c->tex[idx], SDL_BLENDMODE_NONE);
    SDL_RenderCopy(r, c->tex[idx], NULL, NULL);
    SDL_SetTextureBlendMode(c->tex[idx], SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(r, prev);
    return snap;
}

void canvas_restoreLayer(Canvas *c, SDL_Renderer *r, int idx, SDL_Texture *snapshot) {
    SDL_Texture *prev = SDL_GetRenderTarget(r);
    SDL_SetRenderTarget(r, c->tex[idx]);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
    SDL_RenderClear(r);
    SDL_SetTextureBlendMode(snapshot, SDL_BLENDMODE_NONE);
    SDL_RenderCopy(r, snapshot, NULL, NULL);
    SDL_SetRenderTarget(r, prev);
}
