#include "text.h"

#define CACHE_SIZE 96
typedef struct {
    char str[64];
    int ptsize;
    SDL_Color col;
    SDL_Texture *tex;
    int w, h;
    int lastUsed;
    bool used;
} CacheEntry;

static CacheEntry s_cache[CACHE_SIZE];
static int s_frame = 0;
static TTF_Font *s_fonts[3]; /* small, medium, large */
static const int s_sizes[3] = {16, 20, 28};
static void *s_fontMem = NULL;
static bool s_ready = false; /* false if font/TTF init failed; all text calls become no-ops */

static TTF_Font *pickFont(int ptsize) {
    int bestIdx = 0, bestDiff = 999;
    for (int i = 0; i < 3; i++) {
        int d = abs(s_sizes[i] - ptsize);
        if (d < bestDiff) { bestDiff = d; bestIdx = i; }
    }
    return s_fonts[bestIdx];
}

bool text_init(void) {
    memset(s_cache, 0, sizeof(s_cache));
    s_ready = false;

    if (TTF_Init() != 0) return false;

    PlFontData fonts[1];
    Result rc = plGetSharedFontByType(&fonts[0], PlSharedFontType_Standard);
    if (R_FAILED(rc)) return false;

    s_fontMem = fonts[0].address;
    for (int i = 0; i < 3; i++) {
        SDL_RWops *rw = SDL_RWFromConstMem(fonts[0].address, (int)fonts[0].size);
        if (!rw) return false;
        s_fonts[i] = TTF_OpenFontRW(rw, 1, s_sizes[i]);
        if (!s_fonts[i]) return false;
    }
    s_ready = true;
    return true;
}

void text_exit(void) {
    for (int i = 0; i < CACHE_SIZE; i++)
        if (s_cache[i].tex) SDL_DestroyTexture(s_cache[i].tex);
    for (int i = 0; i < 3; i++)
        if (s_fonts[i]) TTF_CloseFont(s_fonts[i]);
    TTF_Quit();
}

void text_newFrame(void) { s_frame++; }

static CacheEntry *findOrCreate(SDL_Renderer *r, const char *str, int ptsize, SDL_Color col) {
    int freeIdx = -1, oldestIdx = 0, oldestUsed = INT32_MAX;
    for (int i = 0; i < CACHE_SIZE; i++) {
        CacheEntry *e = &s_cache[i];
        if (e->used && e->ptsize == ptsize &&
            e->col.r == col.r && e->col.g == col.g && e->col.b == col.b && e->col.a == col.a &&
            strncmp(e->str, str, sizeof(e->str)) == 0) {
            e->lastUsed = s_frame;
            return e;
        }
        if (!e->used && freeIdx < 0) freeIdx = i;
        if (e->lastUsed < oldestUsed) { oldestUsed = e->lastUsed; oldestIdx = i; }
    }
    int idx = (freeIdx >= 0) ? freeIdx : oldestIdx;
    CacheEntry *e = &s_cache[idx];
    if (e->tex) { SDL_DestroyTexture(e->tex); e->tex = NULL; }

    TTF_Font *f = pickFont(ptsize);
    SDL_Surface *surf = TTF_RenderUTF8_Blended(f, str, col);
    if (!surf) return NULL;
    e->tex = SDL_CreateTextureFromSurface(r, surf);
    e->w = surf->w; e->h = surf->h;
    SDL_FreeSurface(surf);
    strncpy(e->str, str, sizeof(e->str) - 1);
    e->str[sizeof(e->str) - 1] = 0;
    e->ptsize = ptsize; e->col = col; e->used = true; e->lastUsed = s_frame;
    return e;
}

int text_draw(SDL_Renderer *r, int x, int y, const char *str, int ptsize, SDL_Color col) {
    if (!s_ready || !str || !*str) return 0;
    CacheEntry *e = findOrCreate(r, str, ptsize, col);
    if (!e) return 0;
    SDL_Rect dst = { x, y, e->w, e->h };
    SDL_RenderCopy(r, e->tex, NULL, &dst);
    return e->w;
}

int text_width(const char *str, int ptsize) {
    if (!s_ready || !str || !*str) return 0;
    TTF_Font *f = pickFont(ptsize);
    int w = 0, h = 0;
    TTF_SizeUTF8(f, str, &w, &h);
    return w;
}
