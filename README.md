# DrawNRO — Switch homebrew drawing app

A full-featured drawing/painting app for Nintendo Switch homebrew (.nro), built
with libnx + SDL2. Works with touch, Joy-Cons/Pro Controller, or both at once.

## Features
- Scalable canvas: pan with two-finger drag or right stick, pinch-to-zoom or
  zoom with the sticks, `L3` resets the view, 1920x1080 working resolution.
- 6 brush types: Hard Round, Soft Round, Airbrush, Calligraphy (angled nib),
  Pencil (textured noise), Eraser — all procedurally generated, no image
  assets required.
- Per-brush size / opacity / hardness / spacing sliders, plus optional
  speed-based fake pressure (thinner on fast strokes, fatter when slow).
- X/Y symmetry drawing (mandala/mirror mode).
- HSV colour wheel + value slider + a 10-swatch recent-colour palette.
- Up to 6 layers: add/remove, per-layer visibility toggle and opacity slider.
- Undo/redo (10 steps deep, per-layer snapshot based).
- Save the flattened composite straight to `sdmc:/switch/drawnro/*.png`.
- Grid overlay toggle.
- Dark, flat "cool" UI theme with procedurally drawn vector icons (nothing
  to import — the whole UI is generated at runtime), plus a toast/notification
  strip for feedback (e.g. "Saved!").
- Text is rendered using the **console's own shared system font** via
  `plGetSharedFontByType` + SDL2_ttf, so there's no font file to ship.

## Controls
| Action                     | Touch                        | Controller                     |
|-----------------------------|-------------------------------|---------------------------------|
| Draw                        | 1-finger drag                 | `ZR` held (+ left stick to move)|
| Erase (momentary)           | —                              | `ZL` held                       |
| Pan canvas                  | 2-finger drag                 | Right stick                     |
| Zoom                        | Pinch                         | Right stick zoom is disabled; use pinch on touch, or bind further if desired |
| Reset view                  | Tap the home icon             | Click left stick (`L3`)         |
| Cycle brush                 | Tap a brush icon              | `Y` (next) / `B` (previous)     |
| Adjust brush size           | Drag the Size slider          | `L`/`R` shoulder                |
| Adjust opacity              | Drag the Opacity slider       | D-Pad up/down                   |
| Switch active layer         | Tap a layer row               | D-Pad left/right                |
| Toggle colour picker        | Tap the colour swatch         | `X`                             |
| Toggle layer panel          | Tap the layers icon           | `A`                             |
| Toggle X symmetry           | Tap the symmetry-X icon       | Click right stick (`R3`)        |
| Undo / Redo                 | Tap the undo/redo icons       | (bind more buttons if you like) |
| Save PNG                    | Tap the save icon             | —                               |
| Clear active layer          | Tap the trash icon            | —                               |

## Building
Requires [devkitPro](https://devkitpro.org/wiki/Getting_Started) with the
Switch toolchain and these portlibs:

```
sudo dkp-pacman -S switch-dev switch-sdl2 switch-sdl2_ttf switch-sdl2_image \
                    switch-freetype switch-libpng switch-libjpeg-turbo switch-zlib switch-bzip2
```

Then, from this folder:

```
export DEVKITPRO=/opt/devkitpro   # adjust to your install
make
```

This produces `drawnro.nro`. Copy it to `switch/drawnro/drawnro.nro` on your
SD card (or `switch/` root) and launch it from the Homebrew Launcher / Album.

## Project layout
```
source/
  main.c         - init, main loop, ties everything together
  app.h/.c        - shared App state struct
  common.h        - shared includes, constants, UI colour palette
  canvas.h/.c     - scalable/pannable multi-layer canvas (render-target textures)
  brush.h/.c      - procedural brush stamps + stroke stamping/blending
  colorpicker.h/.c- HSV wheel + value slider + palette
  undo.h/.c       - per-layer snapshot undo/redo stack
  input.h/.c      - unifies touch gestures + controller into one input model
  ui.h/.c         - toolbar, panels, vector icons, hit-testing
  text.h/.c       - shared-font SDL2_ttf wrapper with a small glyph-texture cache
  save.h/.c       - flattens + exports the canvas to PNG via libpng
Makefile
```

## Notes / things to tune
- `CANVAS_W`/`CANVAS_H` in `common.h` control canvas resolution — bigger
  costs more VRAM per layer and per undo snapshot (`MAX_UNDO` in the same
  file). 1920x1080 x 6 layers x 10 undo steps is already fairly generous for
  a homebrew app; shrink either constant if you hit memory limits on original
  Switch hardware.
- Eraser uses `SDL_ComposeCustomBlendMode` to actually punch alpha rather
  than paint white — needs SDL2 2.0.6+, which devkitPro's `switch-sdl2`
  provides.
- Right-stick zoom isn't wired up (only pinch zooms) to avoid stick
  conflicts with panning — hook `SDL_CONTROLLER_AXIS_TRIGGERLEFT/RIGHT` or a
  spare button combo in `input.c` if you want stick-based zoom too.
- No `romfs`/bundled assets at all — every icon and the color wheel are
  drawn procedurally at startup, and text uses the system's own font, so the
  whole thing is a single self-contained `.nro`.
