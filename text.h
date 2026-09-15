#pragma once
#include "common.h"

bool text_init(void);
void text_exit(void);

/* draws text with top-left at x,y ; returns width drawn */
int  text_draw(SDL_Renderer *r, int x, int y, const char *str, int ptsize, SDL_Color col);
int  text_width(const char *str, int ptsize);
void text_newFrame(void); /* call once per frame to age the glyph cache */
