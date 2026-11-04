#define _POSIX_C_SOURCE 200809L
#include "ember_gfx.h"
#include "ember_ui.h"

#include <stdio.h>
#include <stdlib.h>

static int failures;
#define CHECK(cond) do { \
    if (!(cond)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); failures++; } \
} while (0)

static int clicks_a, clicks_b;
static void on_a(ui_widget *self, void *user) { (void)self; (void)user; clicks_a++; }
static void on_b(ui_widget *self, void *user) { (void)self; (void)user; clicks_b++; }

static void rect_is(const ui_widget *w, int x, int y, int width, int height, const char *name) {
    int rx, ry, rw, rh;
    ui_get_rect(w, &rx, &ry, &rw, &rh);
    if (rx != x || ry != y || rw != width || rh != height) {
        fprintf(stderr, "FAIL %s: got (%d,%d %dx%d) want (%d,%d %dx%d)\n",
                name, rx, ry, rw, rh, x, y, width, height);
        failures++;
    }
}

/* Layout, hit testing and clicks are backend-independent. */
static void test_layout_and_input(void) {
    ui_widget *root = ui_stack(1, 8, 6);
    ui_widget *lbl = ui_label("HI", 0xFFFFFF);
    ui_widget *btn_a = ui_button("A", on_a, NULL);
    ui_widget *btn_b = ui_button("BB", on_b, NULL);
    CHECK(ui_add(root, lbl));
    CHECK(ui_add(root, btn_a));
    CHECK(ui_add(root, btn_b));
    CHECK(!ui_add(lbl, btn_a)); /* labels are not stacks */

    /* Text: 2 chars = 22 wide, 14 tall. Button: text + 20 wide, text + 12 tall. */
    int mw, mh;
    ui_measure(root, &mw, &mh);
    CHECK(mh == 8 + 14 + 6 + 26 + 6 + 26 + 8);

    ui_layout(root, 0, 0, 200, 120);
    rect_is(root, 0, 0, 200, 120, "root");
    rect_is(lbl, 8, 8, 184, 14, "label");
    rect_is(btn_a, 8, 28, 184, 26, "button a");
    rect_is(btn_b, 8, 60, 184, 26, "button b");

    CHECK(ui_hit_test(root, 50, 40) == btn_a);
    CHECK(ui_hit_test(root, 50, 70) == btn_b);
    CHECK(ui_hit_test(root, 50, 10) == lbl);
    CHECK(ui_hit_test(root, 50, 3) == root);
    CHECK(ui_hit_test(root, 300, 5) == NULL);

    CHECK(ui_click(root, 50, 40) == 1 && clicks_a == 1 && clicks_b == 0);
    CHECK(ui_click(root, 50, 70) == 1 && clicks_b == 1);
    CHECK(ui_click(root, 50, 10) == 0);
    CHECK(ui_click(root, 300, 5) == 0);
    CHECK(clicks_a == 1 && clicks_b == 1);

    /* A horizontal stack lays children out left to right, stretched to the row height. */
    ui_widget *row = ui_stack(0, 0, 10);
    ui_widget *r1 = ui_button("ONE", NULL, NULL);
    ui_widget *r2 = ui_button("TWO", NULL, NULL);
    ui_add(row, r1);
    ui_add(row, r2);
    ui_layout(row, 0, 0, 300, 30);
    rect_is(r1, 0, 0, 54, 30, "row button 1");
    rect_is(r2, 64, 0, 54, 30, "row button 2");

    /* A changed text resizes the label on the next layout. */
    ui_set_text(lbl, "HELLO");
    ui_measure(lbl, &mw, &mh);
    CHECK(mw == 5 * 12 - 2);
    ui_layout(root, 0, 0, 200, 120);
    rect_is(lbl, 8, 8, 184, 14, "label after set_text");

    ui_destroy(row);
    ui_destroy(root);
}

#ifdef UI_HAVE_CUSTOM
/* Pixel-level checks for the custom backend. */
static void test_custom_drawing(void) {
    setenv("GFX_BACKEND", "null", 1);
    CHECK(gfx_open(200, 120) == 1);

    ui_widget *root = ui_stack(1, 8, 6);
    ui_widget *lbl = ui_label("HI", 0xFFFFFF);
    ui_widget *btn = ui_button("A", on_a, NULL);
    ui_set_colors(root, 0xFFFFFF, 0x101010);
    ui_add(root, lbl);
    ui_add(root, btn);
    ui_layout(root, 0, 0, 200, 120);

    gfx_clear(0);
    ui_custom_draw(root);
    CHECK(gfx_get_pixel(0, 0) == 0x101010);      /* root background */
    CHECK(gfx_get_pixel(8, 8) == 0xFFFFFF);      /* top-left pixel of 'H' */
    CHECK(gfx_get_pixel(12, 33) == 0x3A3A6A);    /* button background, away from text */
    CHECK(gfx_get_pixel(8, 28) == 0x8888CC);     /* button border */

    /* A label with background 0 is transparent. */
    gfx_clear(0x123456);
    ui_custom_draw(lbl);
    CHECK(gfx_get_pixel(150, 10) == 0x123456);

    ui_destroy(root);
    gfx_close();
}
#endif

int main(void) {
    test_layout_and_input();
#ifdef UI_HAVE_CUSTOM
    test_custom_drawing();
#endif

    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("ember-ui: all tests passed\n");
    return 0;
}
