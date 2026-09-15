#pragma once
#include "common.h"

typedef struct {
    /* primary pointer, in screen coordinates (either finger or virtual
       controller cursor) */
    float x, y;
    bool  havePointer;

    bool  drawDown;     /* finger touching / ZR held  -> paint with primary colour */
    bool  eraseDown;    /* ZL held -> temporary eraser while drawing with touch/controller */

    /* two-finger pan/zoom (touch) or right-stick pan (controller) */
    bool  panning;
    float panDX, panDY;
    float zoomFactor;   /* multiplicative, 1.0 = no change this frame */
    float zoomAtX, zoomAtY;

    bool  controllerConnected;

    /* one-shot (edge-triggered) actions, valid for a single frame */
    bool  actUndo, actRedo, actSave, actNewLayer, actDelLayer, actToggleLayerVis;
    bool  actCycleBrush, actCyclePrevBrush, actToggleColorPanel, actToggleLayerPanel;
    bool  actSizeUp, actSizeDown, actOpacityUp, actOpacityDown;
    bool  actResetView, actToggleSymmetry, actToggleMenu, actClearLayer;
    bool  actNextLayer, actPrevLayer;
} InputState;

void input_init(void);
void input_beginFrame(InputState *s);
void input_handleEvent(InputState *s, const SDL_Event *ev, float screenW, float screenH);
void input_endFrame(InputState *s); /* call after all events processed, updates controller axes */
