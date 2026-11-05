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

## Using it from Ember

Ember has `i64`, `bool` and `[i64]` but no strings, function pointers or structs, so it
cannot call the C API directly. `include/ember_ui_em.h` is a layer shaped to fit, in the same
style as ember-gfx: every function takes and returns `int64_t`, and you declare the ones you
use with `extern def`.

- **Widgets are integer ids.** 0 is never valid, and ids are never reused, so a stale id is
  ignored rather than crashing.
- **Text goes through one shared buffer.** Fill it with `emui_text_put`, `emui_text_int`
  (so no `itoa` is needed) or `emui_text_from_array`, then call `emui_label`, `emui_button`
  or `emui_set_text`.
- **Clicks are polled, not delivered.** `emui_poll_event()` takes the oldest click and
  `emui_event_widget()` says which button it was.
- **You own the loop.** Call `emui_step()` until it returns 0.

```python
extern def emui_open(w: i64, h: i64) -> i64
extern def emui_step() -> i64
# ... see demo/counter.em for a complete program

def main():
    emui_open(240, 150)
    while emui_step() == 1:
        ...
```

`make demo-ember` builds and runs `demo/counter.em`. `make test-ember` runs
`demo/selftest.em` headless and compares its output with `demo/selftest.expect`.

Limits: Ember functions take at most four parameters, so the layer has no function that
needs more. The Ember layer runs on the custom backend only, because the GTK and Win32
backends cannot be stepped one frame at a time. The built-in font has capitals, digits and
a little punctuation, so keep text to those.

## Widget ownership

A widget has one owner. `ui_add` returns 0 if the child already has a parent or if the add
would make a cycle, `ui_remove` takes a child out without destroying it, and `ui_destroy`
detaches a widget from its parent before freeing it. Changing the tree after `ui_set_root`
is only reflected by the custom backend.
