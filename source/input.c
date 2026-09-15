#include "input.h"

static SDL_GameController *s_pad = NULL;
static SDL_FingerID s_fingers[2];
static int s_fingerCount = 0;
static float s_lastFingerX[2], s_lastFingerY[2];
static float s_pinchStartDist = 0.0f;

static float s_virtX = SCREEN_W / 2.0f, s_virtY = SCREEN_H / 2.0f;

void input_init(void) {
    SDL_GameControllerEventState(SDL_ENABLE);
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) { s_pad = SDL_GameControllerOpen(i); break; }
    }
}

void input_beginFrame(InputState *s) {
    bool keepPointer = s->havePointer;
    float px = s->x, py = s->y;
    memset(s, 0, sizeof(*s));
    s->havePointer = keepPointer;
    s->x = px; s->y = py;
    s->zoomFactor = 1.0f;
    s->controllerConnected = (s_pad != NULL);
}

static int findFingerSlot(SDL_FingerID id) {
    for (int i = 0; i < s_fingerCount; i++) if (s_fingers[i] == id) return i;
    return -1;
}

void input_handleEvent(InputState *s, const SDL_Event *ev, float sw, float sh) {
    switch (ev->type) {
        case SDL_CONTROLLERDEVICEADDED:
            if (!s_pad) s_pad = SDL_GameControllerOpen(ev->cdevice.which);
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            if (s_pad) { SDL_GameControllerClose(s_pad); s_pad = NULL; }
            break;

        case SDL_FINGERDOWN: {
            if (s_fingerCount < 2) {
                int slot = s_fingerCount++;
                s_fingers[slot] = ev->tfinger.fingerId;
                s_lastFingerX[slot] = ev->tfinger.x * sw;
                s_lastFingerY[slot] = ev->tfinger.y * sh;
            }
            if (s_fingerCount == 1) {
                s->x = ev->tfinger.x * sw;
                s->y = ev->tfinger.y * sh;
                s->havePointer = true;
                s->drawDown = true;
            } else if (s_fingerCount == 2) {
                s->drawDown = false; /* two fingers => gesture, not drawing */
                float dx = s_lastFingerX[0] - s_lastFingerX[1];
                float dy = s_lastFingerY[0] - s_lastFingerY[1];
                s_pinchStartDist = sqrtf(dx * dx + dy * dy);
            }
            break;
        }
        case SDL_FINGERUP: {
            int slot = findFingerSlot(ev->tfinger.fingerId);
            if (slot >= 0) {
                for (int i = slot; i < s_fingerCount - 1; i++) {
                    s_fingers[i] = s_fingers[i + 1];
                    s_lastFingerX[i] = s_lastFingerX[i + 1];
                    s_lastFingerY[i] = s_lastFingerY[i + 1];
                }
                s_fingerCount--;
            }
            if (s_fingerCount == 0) s->havePointer = false;
            break;
        }
        case SDL_FINGERMOTION: {
            int slot = findFingerSlot(ev->tfinger.fingerId);
            float nx = ev->tfinger.x * sw, ny = ev->tfinger.y * sh;
            if (s_fingerCount == 1 && slot == 0) {
                s->x = nx; s->y = ny;
                s->havePointer = true;
                s->drawDown = true;
            } else if (s_fingerCount == 2) {
                /* two-finger pan + pinch zoom */
                float ocx = (s_lastFingerX[0] + s_lastFingerX[1]) / 2.0f;
                float ocy = (s_lastFingerY[0] + s_lastFingerY[1]) / 2.0f;
                if (slot >= 0) { s_lastFingerX[slot] = nx; s_lastFingerY[slot] = ny; }
                float ncx = (s_lastFingerX[0] + s_lastFingerX[1]) / 2.0f;
                float ncy = (s_lastFingerY[0] + s_lastFingerY[1]) / 2.0f;
                s->panning = true;
                s->panDX += (ncx - ocx);
                s->panDY += (ncy - ocy);

                float dx = s_lastFingerX[0] - s_lastFingerX[1];
                float dy = s_lastFingerY[0] - s_lastFingerY[1];
                float dist = sqrtf(dx * dx + dy * dy);
                if (s_pinchStartDist > 1.0f) {
                    s->zoomFactor *= (dist / s_pinchStartDist);
                    s->zoomAtX = ncx; s->zoomAtY = ncy;
                }
                s_pinchStartDist = dist;
            }
            if (slot >= 0 && s_fingerCount != 2) { s_lastFingerX[slot] = nx; s_lastFingerY[slot] = ny; }
            break;
        }

        case SDL_CONTROLLERBUTTONDOWN: {
            switch (ev->cbutton.button) {
                case SDL_CONTROLLER_BUTTON_START: s->actToggleMenu = true; break;
                case SDL_CONTROLLER_BUTTON_BACK:  s->actToggleMenu = true; break;
                case SDL_CONTROLLER_BUTTON_X:     s->actToggleColorPanel = true; break;
                case SDL_CONTROLLER_BUTTON_Y:     s->actCycleBrush = true; break;
                case SDL_CONTROLLER_BUTTON_B:     s->actCyclePrevBrush = true; break;
                case SDL_CONTROLLER_BUTTON_A:     s->actToggleLayerPanel = true; break;
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  s->actSizeDown = true; break;
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: s->actSizeUp = true; break;
                case SDL_CONTROLLER_BUTTON_DPAD_UP:    s->actOpacityUp = true; break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  s->actOpacityDown = true; break;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  s->actPrevLayer = true; break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: s->actNextLayer = true; break;
                case SDL_CONTROLLER_BUTTON_LEFTSTICK:  s->actResetView = true; break;
                case SDL_CONTROLLER_BUTTON_RIGHTSTICK: s->actToggleSymmetry = true; break;
                default: break;
            }
            break;
        }
        default: break;
    }
}

#define STICK_DEAD 8000
static float axisNorm(Sint16 v) {
    if (v > -STICK_DEAD && v < STICK_DEAD) return 0.0f;
    return v / 32767.0f;
}

void input_endFrame(InputState *s) {
    if (!s_pad) return;

    float lx = axisNorm(SDL_GameControllerGetAxis(s_pad, SDL_CONTROLLER_AXIS_LEFTX));
    float ly = axisNorm(SDL_GameControllerGetAxis(s_pad, SDL_CONTROLLER_AXIS_LEFTY));
    float rx = axisNorm(SDL_GameControllerGetAxis(s_pad, SDL_CONTROLLER_AXIS_RIGHTX));
    float ry = axisNorm(SDL_GameControllerGetAxis(s_pad, SDL_CONTROLLER_AXIS_RIGHTY));
    Sint16 zl = SDL_GameControllerGetAxis(s_pad, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    Sint16 zr = SDL_GameControllerGetAxis(s_pad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);

    if (fabsf(lx) > 0.001f || fabsf(ly) > 0.001f) {
        s_virtX = clampf(s_virtX + lx * 14.0f, 0, SCREEN_W);
        s_virtY = clampf(s_virtY + ly * 14.0f, 0, SCREEN_H);
        s->x = s_virtX; s->y = s_virtY; s->havePointer = true;
    } else if (!s->havePointer) {
        s->x = s_virtX; s->y = s_virtY;
    }

    if (zr > 4000) { s->drawDown = true; s->havePointer = true; s->x = s_virtX; s->y = s_virtY; }
    if (zl > 4000) { s->eraseDown = true; s->havePointer = true; s->x = s_virtX; s->y = s_virtY; }

    if (fabsf(rx) > 0.001f || fabsf(ry) > 0.001f) {
        s->panning = true;
        s->panDX += -rx * 10.0f;
        s->panDY += -ry * 10.0f;
    }
}
