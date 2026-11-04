/* ember-ui: a widget toolkit with native backends.
 *
 * Build a tree of widgets (stacks, labels, buttons), then:
 *
 *   ui_open(title, w, h);      pick a backend and open the window
 *   ui_set_root(root);         show the tree
 *   ui_run();                  run until the window closes
 *
 * Backends, in order of preference:
 *   gtk     Linux with GTK 3. Real GtkButton/GtkLabel/GtkBox controls.
 *   win32   Windows. Real BUTTON/STATIC controls, drawn by the system theme.
 *   custom  Anywhere ember-gfx works. Widgets are drawn into a framebuffer.
 * Set EMBER_UI_BACKEND=gtk|win32|custom to force one.
 *
 * Colours (ui_set_colors) and the bitmap font are used by the custom backend only.
 * Native backends draw with the system theme, so they ignore colours.
 * Hit testing and ui_layout use the custom layout model; native backends
 * place controls with their own layout, so use them with the custom backend or in tests.
 */
#ifndef EMBER_UI_H
#define EMBER_UI_H

#include <stdint.h>

typedef struct ui_widget ui_widget;

/* Called when a button is clicked. */
typedef void (*ui_click_fn)(ui_widget *self, void *user);

ui_widget *ui_stack(int vertical, int padding, int spacing);
ui_widget *ui_label(const char *text, uint32_t fg);
ui_widget *ui_button(const char *text, ui_click_fn on_click, void *user);

/* Adds child to a stack. Returns 1 on success, 0 if parent is not a stack or on
 * allocation failure. */
int ui_add(ui_widget *parent, ui_widget *child);

void ui_set_text(ui_widget *w, const char *text);
void ui_set_colors(ui_widget *w, uint32_t fg, uint32_t bg);

/* Frees w and everything below it. */
void ui_destroy(ui_widget *w);

/* Preferred size of a widget (uses the backend's text measurement when it has one). */
void ui_measure(const ui_widget *w, int *width, int *height);
/* Places w at (x, y), size width x height, and lays out its children. */
void ui_layout(ui_widget *w, int x, int y, int width, int height);
void ui_get_rect(const ui_widget *w, int *x, int *y, int *width, int *height);

/* Deepest widget containing (x, y), or NULL. Uses the last ui_layout. */
ui_widget *ui_hit_test(ui_widget *w, int x, int y);
/* Press at (x, y). Returns 1 if it landed on a button, and calls its handler. */
int ui_click(ui_widget *root, int x, int y);

/* Application */
int ui_open(const char *title, int width, int height);  /* 1 on success */
const char *ui_backend_name(void);                        /* "gtk", "win32", "custom" */
void ui_set_root(ui_widget *root);
int ui_run(void);                                         /* blocks until closed */
void ui_quit(void);

/* Custom backend: draws w into the current ember-gfx canvas (does not present). */
void ui_custom_draw(const ui_widget *w);

#endif
