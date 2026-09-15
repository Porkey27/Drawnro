#include "undo.h"

void undo_init(UndoSystem *u) { memset(u, 0, sizeof(*u)); }

static void clearRedo(UndoSystem *u) {
    for (int i = 0; i < u->redoCount; i++) SDL_DestroyTexture(u->redoStack[i].tex);
    u->redoCount = 0;
}

void undo_destroy(UndoSystem *u) {
    for (int i = 0; i < u->undoCount; i++) SDL_DestroyTexture(u->undoStack[i].tex);
    clearRedo(u);
}

void undo_pushBeforeEdit(UndoSystem *u, Canvas *c, SDL_Renderer *r, int layerIdx) {
    SDL_Texture *snap = canvas_snapshotLayer(c, r, layerIdx);
    if (u->undoCount == MAX_UNDO) {
        SDL_DestroyTexture(u->undoStack[0].tex);
        memmove(&u->undoStack[0], &u->undoStack[1], sizeof(UndoEntry) * (MAX_UNDO - 1));
        u->undoCount--;
    }
    u->undoStack[u->undoCount].tex = snap;
    u->undoStack[u->undoCount].layer = layerIdx;
    u->undoCount++;
    clearRedo(u);
}

void undo_perform(UndoSystem *u, Canvas *c, SDL_Renderer *r) {
    if (u->undoCount == 0) return;
    UndoEntry e = u->undoStack[--u->undoCount];

    /* stash current state for redo before restoring */
    SDL_Texture *cur = canvas_snapshotLayer(c, r, e.layer);
    if (u->redoCount == MAX_UNDO) {
        SDL_DestroyTexture(u->redoStack[0].tex);
        memmove(&u->redoStack[0], &u->redoStack[1], sizeof(UndoEntry) * (MAX_UNDO - 1));
        u->redoCount--;
    }
    u->redoStack[u->redoCount].tex = cur;
    u->redoStack[u->redoCount].layer = e.layer;
    u->redoCount++;

    canvas_restoreLayer(c, r, e.layer, e.tex);
    SDL_DestroyTexture(e.tex);
}

void undo_redoPerform(UndoSystem *u, Canvas *c, SDL_Renderer *r) {
    if (u->redoCount == 0) return;
    UndoEntry e = u->redoStack[--u->redoCount];

    SDL_Texture *cur = canvas_snapshotLayer(c, r, e.layer);
    if (u->undoCount == MAX_UNDO) {
        SDL_DestroyTexture(u->undoStack[0].tex);
        memmove(&u->undoStack[0], &u->undoStack[1], sizeof(UndoEntry) * (MAX_UNDO - 1));
        u->undoCount--;
    }
    u->undoStack[u->undoCount].tex = cur;
    u->undoStack[u->undoCount].layer = e.layer;
    u->undoCount++;

    canvas_restoreLayer(c, r, e.layer, e.tex);
    SDL_DestroyTexture(e.tex);
}
