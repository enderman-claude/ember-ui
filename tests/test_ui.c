#define _POSIX_C_SOURCE 200809L
#include "ember_gfx.h"
#include "ember_ui.h"
#include "ember_ui_em.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* A widget has one owner: no double adds, no cycles, and removing or destroying a child
 * takes it out of its parent so nothing is left dangling. */
static void test_ownership(void) {
    ui_widget *outer = ui_stack(1, 0, 0);
    ui_widget *inner = ui_stack(1, 0, 0);
    ui_widget *a = ui_button("A", NULL, NULL);
    ui_widget *b = ui_button("B", NULL, NULL);
    CHECK(ui_add(outer, inner));
    CHECK(ui_add(inner, a));
    CHECK(!ui_add(outer, a));      /* already owned by inner */
    CHECK(!ui_add(inner, inner));  /* itself */
    CHECK(!ui_add(inner, outer));  /* its own ancestor */

    CHECK(!ui_remove(outer, a));   /* not a child of outer */
    CHECK(ui_remove(inner, a));
    CHECK(!ui_remove(inner, a));   /* already gone */
    CHECK(ui_add(outer, a));       /* free to move */
    CHECK(ui_add(outer, b));

    ui_destroy(a);                 /* detaches itself from outer first */
    ui_layout(outer, 0, 0, 100, 100);
    CHECK(ui_hit_test(outer, 5, 5) != NULL);
    int w, h;
    ui_measure(outer, &w, &h);     /* walks the children: must not touch freed memory */
    CHECK(h == 26);                /* only b is left */

    ui_destroy(outer);
}

/* The polled loop used by Ember programs, on the headless backend. */
static void test_polled_loop(void) {
    setenv("GFX_BACKEND", "null", 1);
    setenv("EMBER_UI_BACKEND", "custom", 1);
    CHECK(ui_step() == 0);  /* nothing is open yet */
    CHECK(ui_open_polled("T", 200, 120) == 1);
    CHECK(strcmp(ui_backend_name(), "custom") == 0);

    ui_widget *root = ui_stack(1, 8, 6);
    ui_widget *btn = ui_button("A", on_a, NULL);
    ui_add(root, btn);
    ui_set_colors(root, 0xFFFFFF, 0x101010);
    ui_set_root(root);

    CHECK(ui_step() == 1);
    CHECK(gfx_get_pixel(0, 0) == 0x101010);
    int x, y, bw, bh;
    ui_get_rect(btn, &x, &y, &bw, &bh);
    CHECK(bw == 184 && bh == 26);

    ui_quit();
    CHECK(ui_step() == 0);
    ui_set_root(NULL);
    ui_destroy(root);
    unsetenv("EMBER_UI_BACKEND");
}

/* The Ember-facing layer: ids, the text buffer, queued clicks. */
static void test_ember_bindings(void) {
    setenv("GFX_BACKEND", "null", 1);
    setenv("EMBER_UI_BACKEND", "custom", 1);

    /* text buffer */
    emui_text_clear();
    emui_text_put('H');
    emui_text_put('I');
    emui_text_put(0);              /* ignored */
    emui_text_put(' ' + 0x100);    /* only the low 8 bits count */
    emui_text_int(-42);
    CHECK(emui_text_len() == 6);   /* "HI -42" */
    int64_t arr[4] = {3, 'O', 'K', '!'};  /* an Ember array: length, then elements */
    CHECK(emui_text_from_array((int64_t)(intptr_t)arr) == 3);
    CHECK(emui_text_from_array(0) == 0);
    for (int i = 0; i < 400; i++) emui_text_put('X');
    CHECK(emui_text_len() == 255); /* capped, not overflowed */

    CHECK(emui_open(0, 100) == 0);
    CHECK(emui_open(200, 120) == 1);

    /* a tree built from ids */
    int64_t root = emui_stack(1, 8, 6);
    emui_text_clear(); emui_text_put('H'); emui_text_put('I');
    int64_t lbl = emui_label(0xFFFFFF);
    emui_text_clear(); emui_text_put('A');
    int64_t ba = emui_button();
    emui_text_clear(); emui_text_put('B'); emui_text_put('B');
    int64_t bb = emui_button();
    CHECK(root && lbl && ba && bb && root != lbl && lbl != ba && ba != bb);
    CHECK(emui_add(root, lbl) == 1);
    CHECK(emui_add(root, ba) == 1);
    CHECK(emui_add(root, bb) == 1);
    CHECK(emui_add(root, ba) == 0);      /* already added */
    CHECK(emui_add(lbl, root) == 0);     /* a label is not a stack */
    CHECK(emui_add(root, 9999) == 0);    /* not an id */
    CHECK(emui_add(0, root) == 0);

    emui_set_colors(root, 0xFFFFFF, 0x101010);
    emui_set_root(root);
    CHECK(emui_step() == 1);
    CHECK(emui_w(root) == 200 && emui_h(root) == 120);
    CHECK(emui_x(ba) == 8 && emui_y(ba) == 28 && emui_w(ba) == 184 && emui_h(ba) == 26);
    CHECK(emui_x(12345) == -1);
    CHECK(emui_hit(50, 40) == ba && emui_hit(50, 70) == bb && emui_hit(50, 10) == lbl);
    CHECK(emui_hit(50, 3) == root && emui_hit(500, 3) == 0);
    CHECK(gfx_get_pixel(8, 8) == 0xFFFFFF);   /* top-left pixel of 'H' */

    /* clicks are queued in order, one event each, and only buttons make them */
    CHECK(emui_poll_event() == 0);
    CHECK(emui_click(50, 40) == 1);
    CHECK(emui_click(50, 70) == 1);
    CHECK(emui_click(50, 10) == 0);          /* the label */
    CHECK(emui_poll_event() == 1 && emui_event_widget() == ba);
    CHECK(emui_poll_event() == 1 && emui_event_widget() == bb);
    CHECK(emui_poll_event() == 0);
    for (int i = 0; i < 100; i++) emui_click(50, 40);
    int queued = 0;
    while (emui_poll_event()) queued++;
    CHECK(queued == 64);                     /* the queue drops the overflow */

    /* changing text resizes on the next frame */
    emui_text_clear(); emui_text_put('H'); emui_text_put('E'); emui_text_put('L');
    emui_set_text(bb);
    emui_set_text(root);                     /* stacks have no text: ignored */
    CHECK(emui_step() == 1);
    CHECK(emui_w(bb) == 184);

    /* removing keeps a widget alive; destroying retires every id below it */
    CHECK(emui_remove(root, ba) == 1);
    CHECK(emui_remove(root, ba) == 0);
    CHECK(emui_step() == 1);
    CHECK(emui_y(bb) == 28);                 /* moved up into ba's place */
    CHECK(emui_w(ba) >= 0);                  /* still valid */
    emui_destroy(ba);
    CHECK(emui_w(ba) == -1);
    CHECK(emui_add(root, ba) == 0);          /* a stale id does nothing */
    emui_destroy(ba);                        /* twice is harmless */

    emui_destroy(root);                      /* the root: takes its children with it */
    CHECK(emui_w(root) == -1 && emui_w(lbl) == -1 && emui_w(bb) == -1);
    CHECK(emui_step() == 1);                 /* still running, showing nothing */
    CHECK(emui_click(50, 40) == 0);

    int64_t fresh = emui_stack(0, 0, 0);
    CHECK(fresh != root && fresh != lbl && fresh != ba && fresh != bb); /* ids not reused */
    emui_destroy(fresh);

    emui_close();
    CHECK(emui_step() == 0);
    unsetenv("EMBER_UI_BACKEND");
}

int main(void) {
    test_layout_and_input();
    test_ownership();
#ifdef UI_HAVE_CUSTOM
    test_custom_drawing();
    test_polled_loop();
    test_ember_bindings();
#endif

    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("ember-ui: all tests passed\n");
    return 0;
}
