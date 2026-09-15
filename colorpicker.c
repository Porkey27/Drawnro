#include "colorpicker.h"

static void hsv2rgb(float h, float s, float v, Uint8 *r, Uint8 *g, Uint8 *b) {
    float rr, gg, bb;
    int i = (int)(h * 6.0f);
    float f = h * 6.0f - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);
    switch (i % 6) {
        case 0: rr = v; gg = t; bb = p; break;
        case 1: rr = q; gg = v; bb = p; break;
        case 2: rr = p; gg = v; bb = t; break;
        case 3: rr = p; gg = q; bb = v; break;
        case 4: rr = t; gg = p; bb = v; break;
        default: rr = v; gg = p; bb = q; break;
    }
    *r = (Uint8)clampf(rr * 255, 0, 255);
    *g = (Uint8)clampf(gg * 255, 0, 255);
    *b = (Uint8)clampf(bb * 255, 0, 255);
}

static void rgb2hsv(Uint8 r8, Uint8 g8, Uint8 b8, float *h, float *s, float *v) {
    float r = r8 / 255.0f, g = g8 / 255.0f, b = b8 / 255.0f;
    float mx = fmaxf(r, fmaxf(g, b)), mn = fminf(r, fminf(g, b));
    float d = mx - mn;
    *v = mx;
    *s = mx <= 0 ? 0 : d / mx;
    if (d < 0.00001f) { *h = 0; return; }
    if (mx == r) *h = fmodf((g - b) / d, 6.0f) / 6.0f;
    else if (mx == g) *h = ((b - r) / d + 2.0f) / 6.0f;
    else *h = ((r - g) / d + 4.0f) / 6.0f;
    if (*h < 0) *h += 1.0f;
}

bool colorpicker_init(SDL_Renderer *r, ColorPickerState *cp) {
    memset(cp, 0, sizeof(*cp));
    cp->h = 0.55f; cp->s = 0.8f; cp->v = 0.95f;
    hsv2rgb(cp->h, cp->s, cp->v, &cp->color.r, &cp->color.g, &cp->color.b);
    cp->color.a = 255;

    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, WHEEL_RES, WHEEL_RES, 32, SDL_PIXELFORMAT_RGBA32);
    SDL_LockSurface(surf);
    Uint32 *px = (Uint32 *)surf->pixels;
    float R = WHEEL_RES / 2.0f;
    for (int y = 0; y < WHEEL_RES; y++) {
        for (int x = 0; x < WHEEL_RES; x++) {
            float dx = (x + 0.5f - R), dy = (y + 0.5f - R);
            float dist = sqrtf(dx * dx + dy * dy) / R;
            if (dist > 1.0f) { px[y * WHEEL_RES + x] = 0; continue; }
            float ang = atan2f(dy, dx);
            if (ang < 0) ang += 2 * (float)M_PI;
            float hue = ang / (2 * (float)M_PI);
            float sat = dist;
            Uint8 rr, gg, bb;
            hsv2rgb(hue, sat, 1.0f, &rr, &gg, &bb);
            Uint8 a = dist > 0.94f ? (Uint8)((1.0f - (dist - 0.94f) / 0.06f) * 255) : 255;
            px[y * WHEEL_RES + x] = SDL_MapRGBA(surf->format, rr, gg, bb, a);
        }
    }
    SDL_UnlockSurface(surf);
    cp->wheel = SDL_CreateTextureFromSurface(r, surf);
    SDL_SetTextureBlendMode(cp->wheel, SDL_BLENDMODE_BLEND);
    SDL_FreeSurface(surf);

    /* seed palette with a friendly default set */
    SDL_Color seed[8] = {
        {255,255,255,255},{20,20,24,255},{230,60,60,255},{255,170,40,255},
        {255,225,60,255},{80,200,120,255},{60,140,255,255},{170,90,230,255}
    };
    for (int i = 0; i < 8; i++) { cp->palette[i] = seed[i]; cp->paletteCount = 8; }
    return cp->wheel != NULL;
}

void colorpicker_destroy(ColorPickerState *cp) {
    if (cp->wheel) SDL_DestroyTexture(cp->wheel);
}

void colorpicker_setRGB(ColorPickerState *cp, Uint8 r8, Uint8 g8, Uint8 b8) {
    cp->color = (SDL_Color){ r8, g8, b8, 255 };
    rgb2hsv(r8, g8, b8, &cp->h, &cp->s, &cp->v);
}

void colorpicker_addToPalette(ColorPickerState *cp, SDL_Color c) {
    for (int i = 0; i < cp->paletteCount; i++) {
        if (cp->palette[i].r == c.r && cp->palette[i].g == c.g && cp->palette[i].b == c.b) return;
    }
    if (cp->paletteCount < PALETTE_SIZE) {
        cp->palette[cp->paletteCount++] = c;
    } else {
        memmove(&cp->palette[0], &cp->palette[1], sizeof(SDL_Color) * (PALETTE_SIZE - 1));
        cp->palette[PALETTE_SIZE - 1] = c;
    }
}

static void fillRect(SDL_Renderer *r, SDL_Rect rc, SDL_Color c) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(r, &rc);
}

void colorpicker_render(SDL_Renderer *r, ColorPickerState *cp, int cx, int cy, int radius) {
    SDL_Rect wdst = { cx - radius, cy - radius, radius * 2, radius * 2 };
    SDL_RenderCopy(r, cp->wheel, NULL, &wdst);

    /* selection ring on the wheel */
    float ang = cp->h * 2 * (float)M_PI;
    float px = cx + cosf(ang) * cp->s * radius;
    float py = cy + sinf(ang) * cp->s * radius;
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    for (int i = 0; i < 360; i += 20) {
        float a2 = i * (float)M_PI / 180.0f;
        SDL_RenderDrawPoint(r, (int)(px + cosf(a2) * 6), (int)(py + sinf(a2) * 6));
    }

    /* value slider to the right of the wheel */
    int barX = cx + radius + 22, barY = cy - radius, barW = 26, barH = radius * 2;
    for (int i = 0; i < barH; i++) {
        float v = 1.0f - (float)i / barH;
        Uint8 rr, gg, bb;
        hsv2rgb(cp->h, cp->s, v, &rr, &gg, &bb);
        SDL_SetRenderDrawColor(r, rr, gg, bb, 255);
        SDL_RenderDrawLine(r, barX, barY + i, barX + barW, barY + i);
    }
    SDL_SetRenderDrawColor(r, COL_BORDER.r, COL_BORDER.g, COL_BORDER.b, 255);
    SDL_Rect barBorder = { barX, barY, barW, barH };
    SDL_RenderDrawRect(r, &barBorder);
    int markY = barY + (int)((1.0f - cp->v) * barH);
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderDrawLine(r, barX - 4, markY, barX + barW + 4, markY);

    /* palette row below */
    int swW = 30, gap = 8;
    int py2 = cy + radius + 26;
    int px2 = cx - radius;
    for (int i = 0; i < cp->paletteCount; i++) {
        SDL_Rect rc = { px2 + i * (swW + gap), py2, swW, swW };
        fillRect(r, rc, cp->palette[i]);
        SDL_SetRenderDrawColor(r, COL_BORDER.r, COL_BORDER.g, COL_BORDER.b, 255);
        SDL_RenderDrawRect(r, &rc);
    }

    /* current colour preview */
    SDL_Rect preview = { barX + barW + 20, barY, 44, 44 };
    fillRect(r, preview, cp->color);
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderDrawRect(r, &preview);
}

bool colorpicker_handleTap(ColorPickerState *cp, int cx, int cy, int radius, float x, float y) {
    float dx = x - cx, dy = y - cy;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist <= radius) {
        float ang = atan2f(dy, dx);
        if (ang < 0) ang += 2 * (float)M_PI;
        cp->h = ang / (2 * (float)M_PI);
        cp->s = clampf(dist / radius, 0, 1);
        hsv2rgb(cp->h, cp->s, cp->v, &cp->color.r, &cp->color.g, &cp->color.b);
        cp->color.a = 255;
        return true;
    }
    int barX = cx + radius + 22, barY = cy - radius, barW = 26, barH = radius * 2;
    if (x >= barX - 6 && x <= barX + barW + 6 && y >= barY && y <= barY + barH) {
        cp->v = clampf(1.0f - (y - barY) / (float)barH, 0, 1);
        hsv2rgb(cp->h, cp->s, cp->v, &cp->color.r, &cp->color.g, &cp->color.b);
        cp->color.a = 255;
        return true;
    }
    int swW = 30, gap = 8;
    int py2 = cy + radius + 26;
    int px2 = cx - radius;
    for (int i = 0; i < cp->paletteCount; i++) {
        SDL_Rect rc = { px2 + i * (swW + gap), py2, swW, swW };
        if (x >= rc.x && x <= rc.x + rc.w && y >= rc.y && y <= rc.y + rc.h) {
            cp->color = cp->palette[i];
            rgb2hsv(cp->color.r, cp->color.g, cp->color.b, &cp->h, &cp->s, &cp->v);
            return true;
        }
    }
    return false;
}
