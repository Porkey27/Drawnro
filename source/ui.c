#include "ui.h"
#include "save.h"
#include "text.h"
#include <sys/stat.h>

/* ---------- small drawing helpers ---------- */

static void fillRoundRect(SDL_Renderer *r, SDL_Rect rc, int rad, SDL_Color col) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
    SDL_Rect a = { rc.x + rad, rc.y, rc.w - rad * 2, rc.h };
    SDL_Rect b = { rc.x, rc.y + rad, rc.w, rc.h - rad * 2 };
    SDL_RenderFillRect(r, &a);
    SDL_RenderFillRect(r, &b);
    for (int i = 0; i < rad; i++) {
        float ang = asinf(1.0f - (float)i / rad);
        int dx = (int)(cosf(ang) * rad);
        SDL_RenderDrawLine(r, rc.x + rad - dx, rc.y + rad - i, rc.x + rc.w - rad + dx, rc.y + rad - i);
        SDL_RenderDrawLine(r, rc.x + rad - dx, rc.y + rc.h - rad + i, rc.x + rc.w - rad + dx, rc.y + rc.h - rad + i);
    }
}

static void drawCircle(SDL_Renderer *r, int cx, int cy, int rad, bool filled) {
    for (int y = -rad; y <= rad; y++) {
        int dx = (int)sqrtf((float)(rad * rad - y * y));
        if (filled) SDL_RenderDrawLine(r, cx - dx, cy + y, cx + dx, cy + y);
        else { SDL_RenderDrawPoint(r, cx - dx, cy + y); SDL_RenderDrawPoint(r, cx + dx, cy + y); }
    }
}

static bool inRect(SDL_Rect rc, float x, float y) {
    return x >= rc.x && x <= rc.x + rc.w && y >= rc.y && y <= rc.y + rc.h;
}

/* ---------- toolbar layout ---------- */

typedef enum {
    BTN_NONE = 0,
    BTN_BRUSH_BASE, /* + BrushType for BRUSH_HARD..BRUSH_ERASER */
    BTN_UNDO = BTN_BRUSH_BASE + BRUSH_COUNT,
    BTN_REDO, BTN_GRID, BTN_SYMX, BTN_SYMY, BTN_LAYERS, BTN_COLOR, BTN_SETTINGS,
    BTN_SAVE, BTN_CLEAR, BTN_HOME,
    BTN_LAYER_ROW_BASE = 100,     /* + layer index, up to MAX_LAYERS */
    BTN_LAYER_EYE_BASE = 100 + MAX_LAYERS,
    BTN_LAYER_ADD = 100 + MAX_LAYERS * 2,
    BTN_LAYER_DEL,
    BTN_LAYER_OPACITY_BASE = 200 /* + layer index */
} BtnId;

typedef struct { SDL_Rect rc; int id; } Btn;
static Btn s_btns[64];
static int s_btnCount;

static void addBtn(SDL_Rect rc, int id) {
    if (s_btnCount < 64) s_btns[s_btnCount++] = (Btn){ rc, id };
}

void ui_init(void) { s_btnCount = 0; }

static void iconFor(SDL_Renderer *r, int id, SDL_Rect rc, SDL_Color col) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
    int cx = rc.x + rc.w / 2, cy = rc.y + rc.h / 2;
    int s2 = rc.w / 2 - 6;

    if (id >= BTN_BRUSH_BASE && id < BTN_BRUSH_BASE + BRUSH_COUNT) {
        BrushType t = (BrushType)(id - BTN_BRUSH_BASE);
        switch (t) {
            case BRUSH_HARD: drawCircle(r, cx, cy, s2 - 2, true); break;
            case BRUSH_SOFT:
                for (int i = 0; i < 4; i++) {
                    SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a / (i + 1));
                    drawCircle(r, cx, cy, s2 - i * 3, true);
                }
                break;
            case BRUSH_AIRBRUSH:
                for (int i = 0; i < 10; i++) {
                    float ang = (i * 0.61803f) * 2 * (float)M_PI;
                    float rr = (i / 10.0f) * s2;
                    SDL_RenderDrawPoint(r, cx + (int)(cosf(ang) * rr), cy + (int)(sinf(ang) * rr));
                    SDL_RenderDrawPoint(r, cx + (int)(cosf(ang) * rr) + 1, cy + (int)(sinf(ang) * rr));
                }
                drawCircle(r, cx, cy, 3, true);
                break;
            case BRUSH_CALLIGRAPHY:
                SDL_RenderDrawLine(r, cx - s2, cy + s2 / 2, cx + s2, cy - s2 / 2);
                SDL_RenderDrawLine(r, cx - s2, cy + s2 / 2 + 1, cx + s2, cy - s2 / 2 + 1);
                SDL_RenderDrawLine(r, cx - s2, cy + s2 / 2 - 1, cx + s2, cy - s2 / 2 - 1);
                break;
            case BRUSH_PENCIL:
                SDL_RenderDrawLine(r, cx - s2, cy + s2, cx + s2, cy - s2);
                SDL_RenderDrawLine(r, cx - s2 + 4, cy + s2, cx - s2, cy + s2 - 4);
                break;
            case BRUSH_ERASER: {
                SDL_Rect er = { cx - s2, cy - s2 / 2, s2 * 2, s2 };
                SDL_RenderFillRect(r, &er);
                SDL_SetRenderDrawColor(r, COL_BG.r, COL_BG.g, COL_BG.b, 255);
                SDL_RenderDrawRect(r, &er);
                break;
            }
            default: break;
        }
        return;
    }

    switch (id) {
        case BTN_UNDO:
            SDL_RenderDrawLine(r, cx + s2, cy - s2, cx - s2, cy);
            SDL_RenderDrawLine(r, cx - s2, cy, cx + s2, cy + s2);
            SDL_RenderDrawLine(r, cx - s2, cy, cx - s2 + 8, cy - 6);
            SDL_RenderDrawLine(r, cx - s2, cy, cx - s2 + 8, cy + 6);
            break;
        case BTN_REDO:
            SDL_RenderDrawLine(r, cx - s2, cy - s2, cx + s2, cy);
            SDL_RenderDrawLine(r, cx + s2, cy, cx - s2, cy + s2);
            SDL_RenderDrawLine(r, cx + s2, cy, cx + s2 - 8, cy - 6);
            SDL_RenderDrawLine(r, cx + s2, cy, cx + s2 - 8, cy + 6);
            break;
        case BTN_GRID:
            for (int i = -1; i <= 1; i++) {
                SDL_RenderDrawLine(r, cx + i * (s2 / 2), cy - s2, cx + i * (s2 / 2), cy + s2);
                SDL_RenderDrawLine(r, cx - s2, cy + i * (s2 / 2), cx + s2, cy + i * (s2 / 2));
            }
            break;
        case BTN_SYMX:
            SDL_RenderDrawLine(r, cx, cy - s2, cx, cy + s2);
            drawCircle(r, cx - s2 / 2, cy, 4, true);
            drawCircle(r, cx + s2 / 2, cy, 4, true);
            break;
        case BTN_SYMY:
            SDL_RenderDrawLine(r, cx - s2, cy, cx + s2, cy);
            drawCircle(r, cx, cy - s2 / 2, 4, true);
            drawCircle(r, cx, cy + s2 / 2, 4, true);
            break;
        case BTN_LAYERS:
            for (int i = -1; i <= 1; i++) {
                SDL_Rect lr = { cx - s2, cy + i * 8 - 6, s2 * 2, 12 };
                SDL_RenderDrawRect(r, &lr);
            }
            break;
        case BTN_COLOR:
            drawCircle(r, cx, cy, s2, true);
            break;
        case BTN_SETTINGS:
            drawCircle(r, cx, cy, s2 - 4, false);
            for (int i = 0; i < 8; i++) {
                float ang = i * (float)M_PI / 4.0f;
                SDL_RenderDrawLine(r, cx + (int)(cosf(ang) * (s2 - 4)), cy + (int)(sinf(ang) * (s2 - 4)),
                                    cx + (int)(cosf(ang) * (s2 + 3)), cy + (int)(sinf(ang) * (s2 + 3)));
            }
            break;
        case BTN_SAVE: {
            SDL_Rect fr = { cx - s2, cy - s2, s2 * 2, s2 * 2 };
            SDL_RenderDrawRect(r, &fr);
            SDL_Rect tab = { cx - s2 / 2, cy - s2, s2, s2 / 2 };
            SDL_RenderFillRect(r, &tab);
            break;
        }
        case BTN_CLEAR: {
            SDL_Rect tr = { cx - s2 + 3, cy - s2 / 2, (s2 - 3) * 2 - 6, s2 * 3 / 2 };
            SDL_RenderDrawRect(r, &tr);
            SDL_RenderDrawLine(r, cx - s2, cy - s2 / 2, cx + s2, cy - s2 / 2);
            SDL_RenderDrawLine(r, cx - s2 / 2, cy - s2 / 2, cx - s2 / 2, cy - s2);
            SDL_RenderDrawLine(r, cx + s2 / 2, cy - s2 / 2, cx + s2 / 2, cy - s2);
            break;
        }
        case BTN_HOME:
            SDL_RenderDrawLine(r, cx - s2, cy, cx, cy - s2);
            SDL_RenderDrawLine(r, cx, cy - s2, cx + s2, cy);
            SDL_RenderDrawLine(r, cx - s2 + 3, cy, cx - s2 + 3, cy + s2 - 3);
            SDL_RenderDrawLine(r, cx + s2 - 3, cy, cx + s2 - 3, cy + s2 - 3);
            SDL_RenderDrawLine(r, cx - s2 + 3, cy + s2 - 3, cx + s2 - 3, cy + s2 - 3);
            break;
        default: break;
    }
}

static void drawButton(SDL_Renderer *r, SDL_Rect rc, int id, bool active, SDL_Color tint) {
    SDL_Color bg = active ? COL_ACCENT : COL_PANEL_ALT;
    if (active) bg.a = 60; else bg.a = 200;
    fillRoundRect(r, rc, 10, bg);
    if (active) {
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, COL_ACCENT.r, COL_ACCENT.g, COL_ACCENT.b, 255);
        SDL_Rect border = rc;
        SDL_RenderDrawRect(r, &border);
    }
    iconFor(r, id, rc, active ? COL_ACCENT : tint);
}

/* ---------- slider helper ---------- */

static void drawSlider(SDL_Renderer *r, SDL_Rect rc, float v01, const char *label, SDL_Color accent) {
    fillRoundRect(r, rc, 6, COL_PANEL_ALT);
    SDL_Rect fill = { rc.x, rc.y, (int)(rc.w * clampf(v01, 0, 1)), rc.h };
    fillRoundRect(r, fill, 6, accent);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, COL_BORDER.r, COL_BORDER.g, COL_BORDER.b, 255);
    SDL_RenderDrawRect(r, &rc);
    char buf[48];
    snprintf(buf, sizeof(buf), "%s %d%%", label, (int)(v01 * 100));
    text_draw(r, rc.x + 8, rc.y + rc.h / 2 - 8, buf, 16, COL_TEXT);
}

static bool hitSlider(SDL_Rect rc, float x, float y, float *outV) {
    if (!inRect(rc, x, y)) return false;
    *outV = clampf((x - rc.x) / (float)rc.w, 0, 1);
    return true;
}

/* ---------- main entry points ---------- */

void ui_handleActions(App *a) {
    InputState *in = &a->input;
    if (in->actUndo) undo_perform(&a->undo, &a->canvas, a->renderer);
    if (in->actRedo) undo_redoPerform(&a->undo, &a->canvas, a->renderer);
    if (in->actCycleBrush) a->brush.type = (BrushType)((a->brush.type + 1) % BRUSH_COUNT);
    if (in->actCyclePrevBrush) a->brush.type = (BrushType)((a->brush.type + BRUSH_COUNT - 1) % BRUSH_COUNT);
    if (in->actToggleColorPanel) { a->colorPanelOpen = !a->colorPanelOpen; a->layerPanelOpen = false; }
    if (in->actToggleLayerPanel) { a->layerPanelOpen = !a->layerPanelOpen; a->colorPanelOpen = false; }
    if (in->actSizeUp) a->brush.size = clampf(a->brush.size + 3, 1, 300);
    if (in->actSizeDown) a->brush.size = clampf(a->brush.size - 3, 1, 300);
    if (in->actOpacityUp) a->brush.opacity = clampf(a->brush.opacity + 0.05f, 0.02f, 1.0f);
    if (in->actOpacityDown) a->brush.opacity = clampf(a->brush.opacity - 0.05f, 0.02f, 1.0f);
    if (in->actResetView) canvas_resetView(&a->canvas, SCREEN_W, SCREEN_H - TOPBAR_H);
    if (in->actToggleSymmetry) { a->brush.symmetryX = !a->brush.symmetryX; }
    if (in->actNextLayer) a->canvas.active = (a->canvas.active + 1) % a->canvas.layerCount;
    if (in->actPrevLayer) a->canvas.active = (a->canvas.active + a->canvas.layerCount - 1) % a->canvas.layerCount;
    if (in->actSave) {
        char path[256];
        if (save_canvasToPng(&a->canvas, a->renderer, path, sizeof(path))) app_pushToast(a, "Saved to sdmc:/switch/drawnro/");
        else app_pushToast(a, "Save failed");
    }
}

bool ui_handleTap(App *a, float x, float y) {
    /* colour wheel popup takes priority while open */
    if (a->colorPanelOpen) {
        int cx = SCREEN_W / 2, cy = SCREEN_H / 2, rad = 130;
        if (colorpicker_handleTap(&a->colorPicker, cx, cy, rad, x, y)) {
            a->brush.color = a->colorPicker.color;
            return true;
        }
        SDL_Rect closeArea = { cx - rad - 40, cy - rad - 60, rad * 2 + 220, rad * 2 + 140 };
        if (!inRect(closeArea, x, y)) { a->colorPanelOpen = false; return true; }
        return true;
    }

    for (int i = 0; i < s_btnCount; i++) {
        if (!inRect(s_btns[i].rc, x, y)) continue;
        int id = s_btns[i].id;

        if (id >= BTN_BRUSH_BASE && id < BTN_BRUSH_BASE + BRUSH_COUNT) {
            a->brush.type = (BrushType)(id - BTN_BRUSH_BASE);
            return true;
        }
        switch (id) {
            case BTN_UNDO: undo_perform(&a->undo, &a->canvas, a->renderer); return true;
            case BTN_REDO: undo_redoPerform(&a->undo, &a->canvas, a->renderer); return true;
            case BTN_GRID: a->showGrid = !a->showGrid; return true;
            case BTN_SYMX: a->brush.symmetryX = !a->brush.symmetryX; return true;
            case BTN_SYMY: a->brush.symmetryY = !a->brush.symmetryY; return true;
            case BTN_LAYERS: a->layerPanelOpen = !a->layerPanelOpen; a->colorPanelOpen = false; return true;
            case BTN_COLOR: a->colorPanelOpen = !a->colorPanelOpen; a->layerPanelOpen = false; return true;
            case BTN_SETTINGS: a->brushPanelOpen = !a->brushPanelOpen; return true;
            case BTN_SAVE: {
                char path[256];
                if (save_canvasToPng(&a->canvas, a->renderer, path, sizeof(path))) app_pushToast(a, "Saved!");
                else app_pushToast(a, "Save failed");
                return true;
            }
            case BTN_CLEAR:
                undo_pushBeforeEdit(&a->undo, &a->canvas, a->renderer, a->canvas.active);
                canvas_clearLayer(&a->canvas, a->renderer, a->canvas.active);
                return true;
            case BTN_HOME: canvas_resetView(&a->canvas, SCREEN_W, SCREEN_H - TOPBAR_H); return true;
            case BTN_LAYER_ADD:
                if (a->canvas.layerCount < MAX_LAYERS) canvas_addLayer(&a->canvas, a->renderer);
                return true;
            case BTN_LAYER_DEL:
                canvas_removeLayer(&a->canvas, a->canvas.active);
                return true;
            default: break;
        }
        if (id >= BTN_LAYER_ROW_BASE && id < BTN_LAYER_ROW_BASE + MAX_LAYERS) {
            a->canvas.active = id - BTN_LAYER_ROW_BASE;
            return true;
        }
        if (id >= BTN_LAYER_EYE_BASE && id < BTN_LAYER_EYE_BASE + MAX_LAYERS) {
            int li = id - BTN_LAYER_EYE_BASE;
            a->canvas.visible[li] = !a->canvas.visible[li];
            return true;
        }
    }

    /* brush settings sliders */
    if (a->brushPanelOpen) {
        SDL_Rect sizeR = { 16, TOPBAR_H + 16, 260, 30 };
        SDL_Rect opacR = { 16, TOPBAR_H + 56, 260, 30 };
        SDL_Rect spacR = { 16, TOPBAR_H + 96, 260, 30 };
        SDL_Rect hardR = { 16, TOPBAR_H + 136, 260, 30 };
        float v;
        if (hitSlider(sizeR, x, y, &v)) { a->brush.size = lerpf(1, 300, v); return true; }
        if (hitSlider(opacR, x, y, &v)) { a->brush.opacity = clampf(v, 0.02f, 1.0f); return true; }
        if (hitSlider(spacR, x, y, &v)) { a->brush.spacing = clampf(v, 0.02f, 1.0f); return true; }
        if (hitSlider(hardR, x, y, &v)) { a->brush.hardness = v; return true; }
        SDL_Rect panelRect = { 8, TOPBAR_H + 8, 280, 176 };
        if (inRect(panelRect, x, y)) return true;
    }

    /* layer panel sliders (opacity per row) + palette rect check handled above via buttons */
    if (a->layerPanelOpen) {
        int px = SCREEN_W - PANEL_W - 8;
        for (int i = 0; i < a->canvas.layerCount; i++) {
            SDL_Rect opacR = { px + 74, TOPBAR_H + 16 + i * 54 + 26, PANEL_W - 90, 14 };
            float v;
            if (hitSlider(opacR, x, y, &v)) { a->canvas.opacity[i] = v; return true; }
        }
        SDL_Rect panelRect = { px - 8, TOPBAR_H + 8, PANEL_W + 16, SCREEN_H - TOPBAR_H - 16 };
        if (inRect(panelRect, x, y)) return true;
    }

    if (y <= TOPBAR_H) return true; /* clicked empty toolbar space */
    return false;
}

/* ---------- rendering ---------- */

static void renderTopbar(SDL_Renderer *r, App *a, float sw) {
    SDL_Rect bar = { 0, 0, (int)sw, TOPBAR_H };
    fillRoundRect(r, bar, 0, COL_PANEL);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, COL_BORDER.r, COL_BORDER.g, COL_BORDER.b, 255);
    SDL_RenderDrawLine(r, 0, TOPBAR_H, (int)sw, TOPBAR_H);

    s_btnCount = 0;
    int bs = 52, gap = 6, x = 10, y = (TOPBAR_H - bs) / 2;

    for (int t = 0; t < BRUSH_COUNT; t++) {
        SDL_Rect rc = { x, y, bs, bs };
        drawButton(r, rc, BTN_BRUSH_BASE + t, a->brush.type == t, COL_TEXT);
        addBtn(rc, BTN_BRUSH_BASE + t);
        x += bs + gap;
    }

    x += 14;
    SDL_Rect settingsR = { x, y, bs, bs };
    drawButton(r, settingsR, BTN_SETTINGS, a->brushPanelOpen, COL_TEXT);
    addBtn(settingsR, BTN_SETTINGS); x += bs + gap;

    SDL_Rect colorR = { x, y, bs, bs };
    fillRoundRect(r, colorR, 10, COL_PANEL_ALT);
    drawCircle(r, colorR.x + bs / 2, colorR.y + bs / 2, bs / 2 - 8, false);
    SDL_SetRenderDrawColor(r, a->brush.color.r, a->brush.color.g, a->brush.color.b, 255);
    drawCircle(r, colorR.x + bs / 2, colorR.y + bs / 2, bs / 2 - 10, true);
    addBtn(colorR, BTN_COLOR); x += bs + gap;

    SDL_Rect layersR = { x, y, bs, bs };
    drawButton(r, layersR, BTN_LAYERS, a->layerPanelOpen, COL_TEXT);
    addBtn(layersR, BTN_LAYERS); x += bs + gap;

    SDL_Rect gridR = { x, y, bs, bs };
    drawButton(r, gridR, BTN_GRID, a->showGrid, COL_TEXT);
    addBtn(gridR, BTN_GRID); x += bs + gap;

    SDL_Rect symxR = { x, y, bs, bs };
    drawButton(r, symxR, BTN_SYMX, a->brush.symmetryX, COL_TEXT);
    addBtn(symxR, BTN_SYMX); x += bs + gap;

    SDL_Rect symyR = { x, y, bs, bs };
    drawButton(r, symyR, BTN_SYMY, a->brush.symmetryY, COL_TEXT);
    addBtn(symyR, BTN_SYMY); x += bs + gap;

    SDL_Rect homeR = { x, y, bs, bs };
    drawButton(r, homeR, BTN_HOME, false, COL_TEXT);
    addBtn(homeR, BTN_HOME); x += bs + gap;

    /* right-aligned: undo/redo/clear/save */
    int rx = (int)sw - 10 - bs;
    SDL_Rect saveR = { rx, y, bs, bs }; drawButton(r, saveR, BTN_SAVE, false, COL_OK); addBtn(saveR, BTN_SAVE); rx -= bs + gap;
    SDL_Rect clearR = { rx, y, bs, bs }; drawButton(r, clearR, BTN_CLEAR, false, COL_DANGER); addBtn(clearR, BTN_CLEAR); rx -= bs + gap;
    SDL_Rect redoR = { rx, y, bs, bs }; drawButton(r, redoR, BTN_REDO, false, COL_TEXT); addBtn(redoR, BTN_REDO); rx -= bs + gap;
    SDL_Rect undoR = { rx, y, bs, bs }; drawButton(r, undoR, BTN_UNDO, false, COL_TEXT); addBtn(undoR, BTN_UNDO);

    char buf[64];
    snprintf(buf, sizeof(buf), "%s", BRUSH_NAMES[a->brush.type]);
    text_draw(r, x + 20, TOPBAR_H / 2 - 10, buf, 20, COL_TEXT);
}

static void renderBrushPanel(SDL_Renderer *r, App *a) {
    if (!a->brushPanelOpen) return;
    SDL_Rect panel = { 8, TOPBAR_H + 8, 280, 176 };
    fillRoundRect(r, panel, 12, COL_PANEL);
    text_draw(r, panel.x + 14, panel.y + 6, "Brush Settings", 18, COL_TEXT_DIM);

    SDL_Rect sizeR = { 16, TOPBAR_H + 16, 260, 30 };
    SDL_Rect opacR = { 16, TOPBAR_H + 56, 260, 30 };
    SDL_Rect spacR = { 16, TOPBAR_H + 96, 260, 30 };
    SDL_Rect hardR = { 16, TOPBAR_H + 136, 260, 30 };
    drawSlider(r, sizeR, a->brush.size / 300.0f, "Size", COL_ACCENT);
    drawSlider(r, opacR, a->brush.opacity, "Opacity", COL_ACCENT2);
    drawSlider(r, spacR, a->brush.spacing, "Spacing", COL_ACCENT);
    drawSlider(r, hardR, a->brush.hardness, "Hardness", COL_ACCENT2);
}

static void renderLayerPanel(SDL_Renderer *r, App *a) {
    if (!a->layerPanelOpen) return;
    int px = SCREEN_W - PANEL_W - 8;
    SDL_Rect panel = { px - 8, TOPBAR_H + 8, PANEL_W + 16, SCREEN_H - TOPBAR_H - 16 };
    fillRoundRect(r, panel, 12, COL_PANEL);
    text_draw(r, panel.x + 14, panel.y + 8, "Layers", 20, COL_TEXT);

    for (int i = a->canvas.layerCount - 1; i >= 0; i--) {
        int rowY = TOPBAR_H + 16 + (a->canvas.layerCount - 1 - i) * 54;
        SDL_Rect row = { px, rowY, PANEL_W, 46 };
        SDL_Color bg = (i == a->canvas.active) ? COL_ACCENT : COL_PANEL_ALT;
        SDL_Color bgFade = bg; bgFade.a = (i == a->canvas.active) ? 90 : 200;
        fillRoundRect(r, row, 8, bgFade);
        addBtn(row, BTN_LAYER_ROW_BASE + i);

        SDL_Rect eye = { px + 6, rowY + 8, 30, 30 };
        drawButton(r, eye, 0, false, a->canvas.visible[i] ? COL_OK : COL_TEXT_DIM);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, a->canvas.visible[i] ? COL_OK.r : COL_TEXT_DIM.r,
                                a->canvas.visible[i] ? COL_OK.g : COL_TEXT_DIM.g,
                                a->canvas.visible[i] ? COL_OK.b : COL_TEXT_DIM.b, 255);
        drawCircle(r, eye.x + 15, eye.y + 15, 8, a->canvas.visible[i]);
        addBtn(eye, BTN_LAYER_EYE_BASE + i);

        text_draw(r, px + 44, rowY + 4, a->canvas.name[i], 15, COL_TEXT);
        SDL_Rect opacR = { px + 74, rowY + 26, PANEL_W - 90, 14 };
        drawSlider(r, opacR, a->canvas.opacity[i], "", COL_ACCENT);
    }

    int by = TOPBAR_H + 16 + a->canvas.layerCount * 54 + 6;
    SDL_Rect addR = { px, by, PANEL_W / 2 - 4, 40 };
    SDL_Rect delR = { px + PANEL_W / 2 + 4, by, PANEL_W / 2 - 4, 40 };
    fillRoundRect(r, addR, 8, COL_OK);
    fillRoundRect(r, delR, 8, COL_DANGER);
    text_draw(r, addR.x + 14, addR.y + 10, "+ Add", 16, COL_BG);
    text_draw(r, delR.x + 10, delR.y + 10, "- Remove", 16, COL_BG);
    addBtn(addR, BTN_LAYER_ADD);
    addBtn(delR, BTN_LAYER_DEL);
}

static void renderColorPopup(SDL_Renderer *r, App *a) {
    if (!a->colorPanelOpen) return;
    int cx = SCREEN_W / 2, cy = SCREEN_H / 2, rad = 130;
    SDL_Rect bg = { cx - rad - 40, cy - rad - 60, rad * 2 + 220, rad * 2 + 140 };
    fillRoundRect(r, bg, 16, COL_PANEL);
    text_draw(r, bg.x + 16, bg.y + 10, "Colour Picker", 20, COL_TEXT);
    colorpicker_render(r, &a->colorPicker, cx - 60, cy, rad);
}

static void renderToast(SDL_Renderer *r, App *a, float sw, float sh) {
    if (a->toastFrames <= 0) return;
    a->toastFrames--;
    int w = text_width(a->toast, 18) + 40;
    SDL_Rect rc = { (int)(sw / 2 - w / 2), (int)(sh - 70), w, 44 };
    Uint8 alpha = (Uint8)clampf(a->toastFrames * 6, 0, 235);
    SDL_Color bg = COL_PANEL; bg.a = alpha;
    fillRoundRect(r, rc, 12, bg);
    SDL_Color tc = COL_OK; tc.a = alpha;
    text_draw(r, rc.x + 20, rc.y + 12, a->toast, 18, tc);
}

static void renderGrid(SDL_Renderer *r, App *a, SDL_Rect viewport) {
    if (!a->showGrid) return;
    SDL_RenderSetClipRect(r, &viewport);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 255, 255, 255, 28);
    float step = 100.0f * a->canvas.zoom;
    if (step > 6.0f) {
        float sx0, sy0;
        canvas_canvasToScreen(&a->canvas, 0, 0, viewport.w, viewport.h, &sx0, &sy0);
        for (float x = fmodf(sx0, step); x < viewport.w; x += step)
            SDL_RenderDrawLine(r, viewport.x + (int)x, viewport.y, viewport.x + (int)x, viewport.y + viewport.h);
        for (float y = fmodf(sy0, step); y < viewport.h; y += step)
            SDL_RenderDrawLine(r, viewport.x, viewport.y + (int)y, viewport.x + viewport.w, viewport.y + (int)y);
    }
    SDL_RenderSetClipRect(r, NULL);
}

static void renderCursor(SDL_Renderer *r, App *a) {
    if (!a->input.havePointer) return;
    float rad = a->brush.size * a->canvas.zoom;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_Color c = a->brush.type == BRUSH_ERASER ? COL_DANGER : COL_ACCENT;
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 200);
    drawCircle(r, (int)a->input.x, (int)a->input.y, (int)rad, false);
}

void ui_render(App *a, float sw, float sh) {
    SDL_Rect viewport = { 0, TOPBAR_H, (int)sw, (int)(sh - TOPBAR_H) };
    renderGrid(a->renderer, a, viewport);
    renderCursor(a->renderer, a);
    renderTopbar(a->renderer, a, sw);
    renderBrushPanel(a->renderer, a);
    renderLayerPanel(a->renderer, a);
    renderColorPopup(a->renderer, a);
    renderToast(a->renderer, a, sw, sh);
}
