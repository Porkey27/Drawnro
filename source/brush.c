#include "brush.h"

const char *BRUSH_NAMES[BRUSH_COUNT] = {
    "Hard Round", "Soft Round", "Airbrush", "Calligraphy", "Pencil", "Eraser"
};

#define STAMP_RES 128
static SDL_Texture *s_stamps[BRUSH_COUNT];

/* build one greyscale-alpha stamp per brush type, straight into a texture we
   can colour-mod and alpha-mod per draw call. All stamps are STAMP_RES^2,
   white RGB with an alpha channel encoding the falloff shape. */
static SDL_Texture *buildStamp(SDL_Renderer *r, BrushType type) {
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, STAMP_RES, STAMP_RES, 32, SDL_PIXELFORMAT_RGBA32);
    SDL_LockSurface(surf);
    Uint32 *px = (Uint32 *)surf->pixels;
    float cx = STAMP_RES / 2.0f, cy = STAMP_RES / 2.0f;
    float R = STAMP_RES / 2.0f;

    srand(1234); /* deterministic noise for the pencil/airbrush textures */

    for (int y = 0; y < STAMP_RES; y++) {
        for (int x = 0; x < STAMP_RES; x++) {
            float dx = (x + 0.5f - cx), dy = (y + 0.5f - cy);
            float d;
            float a = 0.0f;

            switch (type) {
                case BRUSH_HARD:
                    d = sqrtf(dx * dx + dy * dy) / R;
                    a = d <= 1.0f ? 1.0f : 0.0f;
                    /* 1px antialiased edge */
                    if (d > 0.92f && d <= 1.0f) a = 1.0f - (d - 0.92f) / 0.08f;
                    break;
                case BRUSH_SOFT:
                    d = sqrtf(dx * dx + dy * dy) / R;
                    a = d >= 1.0f ? 0.0f : powf(1.0f - d, 1.6f);
                    break;
                case BRUSH_AIRBRUSH:
                    d = sqrtf(dx * dx + dy * dy) / R;
                    a = d >= 1.0f ? 0.0f : powf(1.0f - d, 3.0f);
                    a *= (0.55f + 0.45f * (rand() / (float)RAND_MAX));
                    break;
                case BRUSH_CALLIGRAPHY: {
                    /* flattened ellipse to fake a 40-degree chisel nib */
                    float ang = 0.7f;
                    float rx = dx * cosf(ang) + dy * sinf(ang);
                    float ry = -dx * sinf(ang) + dy * cosf(ang);
                    rx /= R; ry /= (R * 0.35f);
                    d = sqrtf(rx * rx + ry * ry);
                    a = d <= 1.0f ? 1.0f : 0.0f;
                    if (d > 0.85f && d <= 1.0f) a = 1.0f - (d - 0.85f) / 0.15f;
                    break;
                }
                case BRUSH_PENCIL:
                    d = sqrtf(dx * dx + dy * dy) / R;
                    a = d >= 1.0f ? 0.0f : powf(1.0f - d, 1.1f);
                    a *= (0.35f + 0.65f * (rand() / (float)RAND_MAX));
                    break;
                case BRUSH_ERASER:
                    d = sqrtf(dx * dx + dy * dy) / R;
                    a = d >= 1.0f ? 0.0f : powf(1.0f - d, 1.3f);
                    break;
                default: break;
            }
            a = clampf(a, 0.0f, 1.0f);
            Uint8 A = (Uint8)(a * 255.0f);
            px[y * STAMP_RES + x] = (A << 24) | (255 << 16) | (255 << 8) | 255; /* ABGR8888 handled by format below */
        }
    }
    SDL_UnlockSurface(surf);

    /* SDL_PIXELFORMAT_RGBA32 already matches byte order for us across platforms
       when we set channels individually via SDL_MapRGBA; redo properly: */
    SDL_LockSurface(surf);
    for (int y = 0; y < STAMP_RES; y++) {
        for (int x = 0; x < STAMP_RES; x++) {
            Uint32 v = px[y * STAMP_RES + x];
            Uint8 A = (Uint8)(v >> 24);
            px[y * STAMP_RES + x] = SDL_MapRGBA(surf->format, 255, 255, 255, A);
        }
    }
    SDL_UnlockSurface(surf);

    SDL_Texture *t = SDL_CreateTextureFromSurface(r, surf);
    SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    SDL_FreeSurface(surf);
    return t;
}

bool brush_init(SDL_Renderer *r) {
    for (int i = 0; i < BRUSH_COUNT; i++) {
        s_stamps[i] = buildStamp(r, (BrushType)i);
        if (!s_stamps[i]) return false;
    }
    return true;
}

void brush_destroy(void) {
    for (int i = 0; i < BRUSH_COUNT; i++)
        if (s_stamps[i]) SDL_DestroyTexture(s_stamps[i]);
}

void brush_strokeBegin(StrokeState *s, float x, float y) {
    s->active = true;
    s->lastX = x; s->lastY = y;
    s->leftover = 0.0f;
    s->lastSpeed = 0.0f;
}

void brush_strokeEnd(StrokeState *s) { s->active = false; }

static void stampOne(SDL_Renderer *r, SDL_Texture *stamp, float cxPix, float cyPix,
                      float sizePix, SDL_Color color, float alphaScale) {
    SDL_Rect dst = {
        (int)(cxPix - sizePix), (int)(cyPix - sizePix),
        (int)(sizePix * 2), (int)(sizePix * 2)
    };
    SDL_SetTextureColorMod(stamp, color.r, color.g, color.b);
    Uint8 a = (Uint8)clampf(color.a * alphaScale, 0, 255);
    SDL_SetTextureAlphaMod(stamp, a);
    SDL_RenderCopy(r, stamp, NULL, &dst);
}

static void stampAt(SDL_Renderer *r, Canvas *c, BrushSettings *b, float canvasX, float canvasY, float speed) {
    SDL_Texture *stamp = s_stamps[b->type];
    SDL_BlendMode bm = SDL_BLENDMODE_BLEND;
    if (b->type == BRUSH_ERASER) {
        bm = SDL_ComposeCustomBlendMode(
            SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_ZERO, SDL_BLENDOPERATION_ADD,
            SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);
    } else if (b->type == BRUSH_AIRBRUSH) {
        bm = SDL_BLENDMODE_BLEND;
    }
    SDL_SetTextureBlendMode(stamp, bm);
    SDL_SetRenderDrawBlendMode(r, bm);

    float sizePix = b->size;
    if (b->pressureSim) {
        float t = clampf(speed / 40.0f, 0.0f, 1.0f);
        sizePix *= lerpf(1.15f, 0.6f, t); /* slower = fatter, faster = thinner */
    }
    float alphaScale = clampf(b->opacity, 0.0f, 1.0f);
    if (b->type == BRUSH_SOFT) alphaScale *= lerpf(0.35f, 1.0f, b->hardness);

    SDL_Color col = b->color;
    col.a = 255;

    stampOne(r, stamp, canvasX, canvasY, sizePix, col, alphaScale);
    if (b->symmetryX) stampOne(r, stamp, (float)CANVAS_W - canvasX, canvasY, sizePix, col, alphaScale);
    if (b->symmetryY) stampOne(r, stamp, canvasX, (float)CANVAS_H - canvasY, sizePix, col, alphaScale);
    if (b->symmetryX && b->symmetryY)
        stampOne(r, stamp, (float)CANVAS_W - canvasX, (float)CANVAS_H - canvasY, sizePix, col, alphaScale);
}

void brush_strokeTo(SDL_Renderer *r, Canvas *c, int layerIdx, BrushSettings *b, StrokeState *s, float x, float y) {
    if (!s->active) { brush_strokeBegin(s, x, y); }

    SDL_Texture *prev = SDL_GetRenderTarget(r);
    SDL_SetRenderTarget(r, c->tex[layerIdx]);

    float step = fmaxf(b->size * 2.0f * clampf(b->spacing, 0.02f, 1.0f), 0.75f);
    float dx = x - s->lastX, dy = y - s->lastY;
    float segLen = sqrtf(dx * dx + dy * dy);
    float speed = segLen;

    if (segLen < 0.0001f) {
        if (s->leftover <= 0.0001f) stampAt(r, c, b, x, y, speed);
        SDL_SetRenderTarget(r, prev);
        return;
    }

    float travelled = -s->leftover;
    float ux = dx / segLen, uy = dy / segLen;
    while (travelled + step <= segLen) {
        travelled += step;
        float px = s->lastX + ux * travelled;
        float py = s->lastY + uy * travelled;
        stampAt(r, c, b, px, py, speed);
    }
    s->leftover = segLen - travelled;
    s->lastX = x; s->lastY = y;
    s->lastSpeed = speed;

    SDL_SetRenderTarget(r, prev);
}
