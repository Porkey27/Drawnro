#pragma once
#include "app.h"

void ui_init(void);

/* returns true if the tap/click at (x,y) was consumed by UI (toolbar/panels)
   so the caller should NOT treat it as a canvas drawing point */
bool ui_handleTap(App *a, float x, float y);

/* applies controller one-shot actions accumulated in a->input this frame */
void ui_handleActions(App *a);

void ui_render(App *a, float sw, float sh);
