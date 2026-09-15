#pragma once
#include "common.h"
#include "canvas.h"
#include "brush.h"
#include "colorpicker.h"
#include "undo.h"
#include "input.h"

typedef struct {
    SDL_Renderer *renderer;
    SDL_Window   *window;

    Canvas canvas;
    BrushSettings brush;
    StrokeState stroke;
    ColorPickerState colorPicker;
    UndoSystem undo;
    InputState input;

    bool colorPanelOpen;
    bool layerPanelOpen;
    bool brushPanelOpen;
    bool showGrid;
    bool erasingOverride;   /* controller ZL momentary erase */
    bool strokeDirty;       /* whether undo snapshot has been pushed for current stroke */

    char toast[64];
    int  toastFrames;

    bool running;
} App;

void app_pushToast(App *a, const char *msg);
