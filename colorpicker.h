#pragma once
#include "common.h"

#define WHEEL_RES 180
#define PALETTE_SIZE 10

typedef struct {
    SDL_Texture *wheel;      /* hue/sat wheel, alpha=0 outside circle */
    float h, s, v;           /* 0..1 each */
    SDL_Color color;         /* resolved RGBA, a always 255 */
    SDL_Color palette[PALETTE_SIZE];
    int paletteCount;
} ColorPickerState;

bool colorpicker_init(SDL_Renderer *r, ColorPickerState *cp);
void colorpicker_destroy(ColorPickerState *cp);

/* panel drawn with the wheel centred at (cx,cy), radius R, plus a value strip
   to the right and a palette row below; returns the screen rect it occupied */
void colorpicker_render(SDL_Renderer *r, ColorPickerState *cp, int cx, int cy, int radius);

/* returns true if (x,y) hit something in the panel and updated the colour */
bool colorpicker_handleTap(ColorPickerState *cp, int cx, int cy, int radius, float x, float y);

void colorpicker_addToPalette(ColorPickerState *cp, SDL_Color c);
void colorpicker_setRGB(ColorPickerState *cp, Uint8 r8, Uint8 g8, Uint8 b8);
