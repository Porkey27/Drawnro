#include "app.h"
#include "ui.h"
#include "text.h"
#include "save.h"

static App app;

static bool initEverything(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) return false;

    app.window = SDL_CreateWindow("DrawNRO", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                   SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);
    if (!app.window) return false;

    app.renderer = SDL_CreateRenderer(app.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!app.renderer) return false;
    SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);

    if (!text_init()) return false; /* shared system font via pl service */
    if (!brush_init(app.renderer)) return false;
    if (!canvas_init(&app.canvas, app.renderer, CANVAS_W, CANVAS_H)) return false;
    if (!colorpicker_init(app.renderer, &app.colorPicker)) return false;
    undo_init(&app.undo);
    input_init();
    ui_init();

    canvas_resetView(&app.canvas, SCREEN_W, SCREEN_H - TOPBAR_H);

    app.brush.type = BRUSH_SOFT;
    app.brush.size = 22.0f;
    app.brush.opacity = 1.0f;
    app.brush.hardness = 0.7f;
    app.brush.spacing = 0.12f;
    app.brush.color = (SDL_Color){ 230, 230, 235, 255 };
    app.brush.pressureSim = true;

    app.showGrid = false;
    app.running = true;
    app_pushToast(&app, "Welcome! ZR/finger draws, ZL erases, hold R-stick pans");
    return true;
}

static void shutdownEverything(void) {
    canvas_destroy(&app.canvas);
    brush_destroy();
    colorpicker_destroy(&app.colorPicker);
    undo_destroy(&app.undo);
    text_exit();
    if (app.renderer) SDL_DestroyRenderer(app.renderer);
    if (app.window) SDL_DestroyWindow(app.window);
    SDL_Quit();
}

static void handleGesturesAndActions(void) {
    InputState *in = &app.input;

    if (in->panning) canvas_pan(&app.canvas, in->panDX, in->panDY);
    if (in->zoomFactor != 1.0f) {
        float zx = in->zoomAtX > 0 ? in->zoomAtX : SCREEN_W / 2.0f;
        float zy = in->zoomAtY > 0 ? in->zoomAtY : (SCREEN_H - TOPBAR_H) / 2.0f + TOPBAR_H;
        canvas_zoomAt(&app.canvas, zx, zy - TOPBAR_H, SCREEN_W, SCREEN_H - TOPBAR_H, in->zoomFactor);
    }

    ui_handleActions(&app);
}

static void handleDrawing(void) {
    InputState *in = &app.input;
    bool wantDraw = in->havePointer && (in->drawDown || in->eraseDown) && !in->panning;

    bool uiConsumed = false;
    if (in->havePointer && wantDraw) {
        uiConsumed = ui_handleTap(&app, in->x, in->y);
    }

    if (wantDraw && !uiConsumed && in->y > TOPBAR_H) {
        if (!app.stroke.active) {
            undo_pushBeforeEdit(&app.undo, &app.canvas, app.renderer, app.canvas.active);
        }
        float cx, cy;
        SDL_Rect viewport = { 0, TOPBAR_H, SCREEN_W, SCREEN_H - TOPBAR_H };
        canvas_screenToCanvas(&app.canvas, in->x - viewport.x, in->y - viewport.y,
                               viewport.w, viewport.h, &cx, &cy);

        BrushSettings use = app.brush;
        if (in->eraseDown) use.type = BRUSH_ERASER;

        brush_strokeTo(app.renderer, &app.canvas, app.canvas.active, &use, &app.stroke, cx, cy);
    } else if (app.stroke.active) {
        brush_strokeEnd(&app.stroke);
        colorpicker_addToPalette(&app.colorPicker, app.brush.color);
    }
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    if (R_FAILED(plInitialize(PlServiceType_User))) return 1;

    if (!initEverything()) {
        shutdownEverything();
        plExit();
        return 1;
    }

    while (appletMainLoop() && app.running) {
        text_newFrame();
        input_beginFrame(&app.input);

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) app.running = false;
            input_handleEvent(&app.input, &ev, SCREEN_W, SCREEN_H);
        }
        input_endFrame(&app.input);

        handleGesturesAndActions();
        handleDrawing();

        SDL_SetRenderDrawColor(app.renderer, COL_BG.r, COL_BG.g, COL_BG.b, 255);
        SDL_RenderClear(app.renderer);

        SDL_Rect viewport = { 0, TOPBAR_H, SCREEN_W, SCREEN_H - TOPBAR_H };
        canvas_render(&app.canvas, app.renderer, viewport);
        ui_render(&app, SCREEN_W, SCREEN_H);

        SDL_RenderPresent(app.renderer);
    }

    shutdownEverything();
    plExit();
    return 0;
}
