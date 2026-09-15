#include "app.h"

void app_pushToast(App *a, const char *msg) {
    snprintf(a->toast, sizeof(a->toast), "%s", msg);
    a->toastFrames = 120;
}
