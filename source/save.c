#include "save.h"
#include <png.h>
#include <sys/stat.h>
#include <time.h>

bool save_canvasToPng(Canvas *c, SDL_Renderer *r, char *outPath, size_t outPathSize) {
    mkdir("sdmc:/switch", 0777);
    mkdir("sdmc:/switch/drawnro", 0777);

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char path[256];
    snprintf(path, sizeof(path), "sdmc:/switch/drawnro/drawing_%04d%02d%02d_%02d%02d%02d.png",
              tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    SDL_Texture *composite = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888,
                                                SDL_TEXTUREACCESS_TARGET, c->width, c->height);
    if (!composite) return false;
    SDL_SetTextureBlendMode(composite, SDL_BLENDMODE_BLEND);

    SDL_Texture *prevTarget = SDL_GetRenderTarget(r);
    SDL_SetRenderTarget(r, composite);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255); /* flatten onto white, like most paint apps' export */
    SDL_RenderClear(r);
    for (int i = 0; i < c->layerCount; i++) {
        if (!c->visible[i]) continue;
        SDL_SetTextureBlendMode(c->tex[i], SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(c->tex[i], (Uint8)(clampf(c->opacity[i], 0, 1) * 255));
        SDL_RenderCopy(r, c->tex[i], NULL, NULL);
    }

    int w = c->width, h = c->height;
    Uint8 *pixels = (Uint8 *)malloc((size_t)w * h * 4);
    bool ok = pixels && SDL_RenderReadPixels(r, NULL, SDL_PIXELFORMAT_RGBA8888, pixels, w * 4) == 0;

    SDL_SetRenderTarget(r, prevTarget);
    SDL_DestroyTexture(composite);

    if (!ok) { free(pixels); return false; }

    FILE *fp = fopen(path, "wb");
    if (!fp) { free(pixels); return false; }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png ? png_create_info_struct(png) : NULL;
    if (!png || !info) { fclose(fp); free(pixels); return false; }

    if (setjmp(png_jmpbuf(png))) { fclose(fp); free(pixels); png_destroy_write_struct(&png, &info); return false; }

    png_init_io(png, fp);
    png_set_IHDR(png, info, w, h, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    png_bytep *rows = (png_bytep *)malloc(sizeof(png_bytep) * h);
    for (int y = 0; y < h; y++) rows[y] = pixels + (size_t)y * w * 4;
    png_write_image(png, rows);
    png_write_end(png, NULL);

    free(rows);
    png_destroy_write_struct(&png, &info);
    fclose(fp);
    free(pixels);

    if (outPath) snprintf(outPath, outPathSize, "%s", path);
    return true;
}
