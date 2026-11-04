/* ui_demo: a window with a title, a counter, and buttons.
 * Uses the native backend when one is available, and the custom backend otherwise.
 *
 *   build/ui_demo                     run until the window is closed
 *   EMBER_UI_BACKEND=custom build/ui_demo   force the custom backend
 */
#include "ember_ui.h"

#include <stdio.h>

typedef struct {
    int clicks;
    ui_widget *counter;
    char text[32];
} app_t;

static void show_count(app_t *app) {
    snprintf(app->text, sizeof app->text, "CLICKS: %d", app->clicks);
    ui_set_text(app->counter, app->text);
}

static void on_click(ui_widget *self, void *user) {
    (void)self;
    app_t *app = user;
    app->clicks++;
    show_count(app);
}

static void on_reset(ui_widget *self, void *user) {
    (void)self;
    app_t *app = user;
    app->clicks = 0;
    show_count(app);
}

static void on_quit(ui_widget *self, void *user) {
    (void)self;
    (void)user;
    ui_quit();
}

int main(void) {
    if (!ui_open("EMBER UI", 320, 240)) {
        fprintf(stderr, "ui_demo: no backend could open a window\n");
        return 1;
    }

    app_t app = {0};
    app.counter = ui_label("CLICKS: 0", 0xAAFFAA);
    ui_widget *title = ui_label("EMBER UI", 0xFFFFFF);
    ui_widget *click = ui_button("CLICK ME", on_click, &app);
    ui_widget *reset = ui_button("RESET", on_reset, &app);
    ui_widget *quit = ui_button("QUIT", on_quit, NULL);
    ui_widget *row = ui_stack(0, 0, 10);
    ui_widget *root = ui_stack(1, 16, 12);
    if (!app.counter || !title || !click || !reset || !quit || !row || !root) {
        fprintf(stderr, "ui_demo: out of memory\n");
        return 1;
    }

    ui_set_colors(root, 0xFFFFFF, 0x202030);
    ui_set_colors(click, 0xFFFFFF, 0x3A6A3A);
    ui_set_colors(quit, 0xFFFFFF, 0x6A3A3A);

    ui_add(row, click);
    ui_add(row, reset);
    ui_add(row, quit);
    ui_add(root, title);
    ui_add(root, app.counter);
    ui_add(root, row);

    ui_set_root(root);
    ui_run();
    ui_destroy(root);
    return 0;
}
