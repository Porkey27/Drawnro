#pragma once
#include "common.h"
#include "canvas.h"

/* renders all visible layers at full canvas resolution and writes a timestamped
   PNG to sdmc:/switch/drawnro/. returns true and fills outPath on success. */
bool save_canvasToPng(Canvas *c, SDL_Renderer *r, char *outPath, size_t outPathSize);
