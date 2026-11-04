# ember-ui

A widget toolkit with native backends. Build a widget tree once, and it is shown with the
platform's own controls where possible:

- **GTK 3** on Linux (GtkButton, GtkLabel, GtkBox)
- **Win32** on Windows (BUTTON, STATIC)
- **Custom** anywhere [ember-gfx](https://github.com/enderman-claude/ember-gfx) works (framebuffer drawing)

```c
ui_open("Hello", 320, 240);
ui_set_root(root);
ui_run();
```

Set `EMBER_UI_BACKEND=gtk|win32|custom` to force a backend.

## Build

    make                    # Linux: custom, plus GTK if installed
    make test               # headless tests
    make demo               # build/ui_demo
    tests/e2e_ui.sh         # end-to-end clicks on Xvfb (needs xdotool, ImageMagick)
    make WIN32=1 demo       # Windows build with MinGW

The custom backend builds ember-gfx from `../ember-gfx` (override with `GFX=`).
