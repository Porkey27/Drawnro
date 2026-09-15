#pragma once
#include "common.h"
#include "canvas.h"

typedef struct {
    SDL_Texture *tex;
    int layer;
} UndoEntry;

typedef struct {
    UndoEntry undoStack[MAX_UNDO];
    UndoEntry redoStack[MAX_UNDO];
    int undoCount, redoCount;
} UndoSystem;

void undo_init(UndoSystem *u);
void undo_destroy(UndoSystem *u);

/* call right before a stroke starts modifying a layer */
void undo_pushBeforeEdit(UndoSystem *u, Canvas *c, SDL_Renderer *r, int layerIdx);

void undo_perform(UndoSystem *u, Canvas *c, SDL_Renderer *r);
void undo_redoPerform(UndoSystem *u, Canvas *c, SDL_Renderer *r);
