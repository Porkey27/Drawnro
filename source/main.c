#include "app.h"
#include "ui.h"
#include "text.h"
#include "save.h"
#include <sys/stat.h>

static App app;

/* ---------------------------------------------------------------------
 * Diagnostics: if anything fails to initialise, we log it to a file on
 * the SD card AND paint the whole screen a specific solid colour for a
 * few seconds before closing, so a failure is visible/photographable
 * even without any way to read logs off the console.
 *
 *   RED     - SDL_Init failed
 *   ORANGE  - window creation failed
 *   YELLOW  - renderer creation failed (both accelerated and software)
 *   MAGENTA - plInitialize (system font service) failed
 *   GREEN   - brush system failed to initialise
 *   CYAN    - canvas / layer textures failed to initialise
 *   BLUE    - colour picker failed to initialise
 *
 * (Font/text loading failing is NOT fatal - the app just runs without
 * on-screen labels, since text isn't required for drawing to work.)
 * ------------------------------------------------------------------- */

static void logMsg(const char *msg) {
    mkdir("sdmc:/switch", 0777);
    mkdir("sdmc:/switch/drawnro", 0777);
    FILE *f = fopen("sdmc:/switch/drawnro/debug.log", "a");
    if (!f) return;
    fprintf(f, "%s\n", msg);
    fclose(f);
}

static void holdColor(SDL_Renderer *r, Uint8 rr, Uint8 gg, Uint8 bb) {
    if (!r) return;
    for (int i = 0; i < 300 && appletMainLoop(); i++) { /* ~5s at 60fps-ish */
        SDL_SetRenderDrawColor(r, rr, gg, bb, 255);
        SDL_RenderClear(r);
        SDL_RenderPresent(r);
        SDL_Delay(16);
    }
}

typedef struct { SDL_Window *window; SDL_Renderer *renderer; } BootResult;

/* stage 1-3: SDL + window + renderer, with logging/colour feedback for each */
static bool bootSDL(BootResult *out) {
    logMsg("stage: SDL_Init");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        logMsg("FAILED: SDL_Init");
        logMsg(SDL_GetError());
        return false; /* no renderer exists yet, nothing to colour */
    }

    logMsg("stage: SDL_CreateWindow");
    out->window = SDL_CreateWindow("DrawNRO", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                    SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);
    if (!out->window) {
        logMsg("FAILED: SDL_CreateWindow");
        logMsg(SDL_GetError());
        return false;
    }

    logMsg("stage: SDL_CreateRenderer (accelerated)");
    out->renderer = SDL_CreateRenderer(out->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!out->renderer) {
        logMsg("accelerated renderer failed, trying software:");
        logMsg(SDL_GetError());
        out->renderer = SDL_CreateRenderer(out->window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!out->renderer) {
        logMsg("FAILED: SDL_CreateRenderer (both accelerated and software)");
        logMsg(SDL_GetError());
        return false; /* still nothing to draw with */
    }
    SDL_SetRenderDrawBlendMode(out->renderer, SDL_BLENDMODE_BLEND);
    logMsg("SDL boot OK");
    return true;
}

static bool initEverything(void) {
    logMsg("stage: plInitialize");
    if (R_FAILED(plInitialize(PlServiceType_User))) {
        logMsg("FAILED: plInitialize");
        holdColor(app.renderer, 255, 0, 255); /* MAGENTA */
        return false;
    }

    logMsg("stage: text_init (shared font, non-fatal)");
    if (!text_init()) {
        logMsg("WARNING: text_init failed, continuing without on-screen text");
    }

    logMsg("stage: brush_init");
    if (!brush_init(app.renderer)) {
        logMsg("FAILED: brush_init");
        holdColor(app.renderer, 0, 255, 0); /* GREEN */
        return false;
    }

    logMsg("stage: canvas_init");
    if (!canvas_init(&app.canvas, app.renderer, CANVAS_W, CANVAS_H)) {
        logMsg("FAILED: canvas_init");
        holdColor(app.renderer, 0, 255, 255); /* CYAN */
        return false;
    }

    logMsg("stage: colorpicker_init");
    if (!colorpicker_init(app.renderer, &app.colorPicker)) {
        logMsg("FAILED: colorpicker_init");
        holdColor(app.renderer, 0, 0, 255); /* BLUE */
        return false;
    }

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
    logMsg("init complete OK");
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

    logMsg("=== DrawNRO starting ===");

    BootResult boot = { NULL, NULL };
    if (!bootSDL(&boot)) {
        /* can't even get a renderer up - nothing to show on screen,
           the log file is the only record of what happened */
        return 1;
    }
    app.window = boot.window;
    app.renderer = boot.renderer;

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

    logMsg("=== DrawNRO exiting normally ===");
    shutdownEverything();
    plExit();
    return 0;
}
