/* Custom backend: draws widgets into an ember-gfx canvas with a built-in bitmap font.
 * It runs anywhere ember-gfx does (X11, Win32, or headless). */
#include "ember_gfx.h"
#include "ui_internal.h"

#include <ctype.h>
#include <stdint.h>
#include <string.h>

/* ---- font: 5x7 glyphs, '#' is a lit pixel ----------------------------------- */

typedef struct {
    char c;
    const char *rows[7];
} glyph_t;

static const glyph_t glyphs[] = {
    {' ', {".....", ".....", ".....", ".....", ".....", ".....", "....."}},
    {'!', {"..#..", "..#..", "..#..", "..#..", "..#..", ".....", "..#.."}},
    {'+', {".....", "..#..", "..#..", "#####", "..#..", "..#..", "....."}},
    {'-', {".....", ".....", ".....", "#####", ".....", ".....", "....."}},
    {'.', {".....", ".....", ".....", ".....", ".....", ".##..", ".##.."}},
    {':', {".....", ".##..", ".##..", ".....", ".##..", ".##..", "....."}},
    {'/', {"....#", "...#.", "...#.", "..#..", ".#...", ".#...", "#...."}},
    {'?', {".###.", "#...#", "....#", "...#.", "..#..", ".....", "..#.."}},
    {'0', {".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###."}},
    {'1', {"..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."}},
    {'2', {".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####"}},
    {'3', {".###.", "#...#", "....#", "..##.", "....#", "#...#", ".###."}},
    {'4', {"...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#."}},
    {'5', {"#####", "#....", "####.", "....#", "....#", "#...#", ".###."}},
    {'6', {"..##.", ".#...", "#....", "####.", "#...#", "#...#", ".###."}},
    {'7', {"#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..."}},
    {'8', {".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."}},
    {'9', {".###.", "#...#", "#...#", ".####", "....#", "...#.", ".##.."}},
    {'A', {".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}},
    {'B', {"####.", "#...#", "#...#", "####.", "#...#", "#...#", "####."}},
    {'C', {".###.", "#...#", "#....", "#....", "#....", "#...#", ".###."}},
    {'D', {"###..", "#..#.", "#...#", "#...#", "#...#", "#..#.", "###.."}},
    {'E', {"#####", "#....", "#....", "####.", "#....", "#....", "#####"}},
    {'F', {"#####", "#....", "#....", "####.", "#....", "#....", "#...."}},
    {'G', {".###.", "#...#", "#....", "#.###", "#...#", "#...#", ".####"}},
    {'H', {"#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}},
    {'I', {".###.", "..#..", "..#..", "..#..", "..#..", "..#..", ".###."}},
    {'J', {"..###", "...#.", "...#.", "...#.", "...#.", "#..#.", ".##.."}},
    {'K', {"#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#"}},
    {'L', {"#....", "#....", "#....", "#....", "#....", "#....", "#####"}},
    {'M', {"#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#"}},
    {'N', {"#...#", "##..#", "#.#.#", "#..##", "#...#", "#...#", "#...#"}},
    {'O', {".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}},
    {'P', {"####.", "#...#", "#...#", "####.", "#....", "#....", "#...."}},
    {'Q', {".###.", "#...#", "#...#", "#...#", "#.#.#", "#..#.", ".##.#"}},
    {'R', {"####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#"}},
    {'S', {".####", "#....", "#....", ".###.", "....#", "....#", "####."}},
    {'T', {"#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.."}},
    {'U', {"#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}},
    {'V', {"#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", "..#.."}},
    {'W', {"#...#", "#...#", "#...#", "#.#.#", "#.#.#", "##.##", "#...#"}},
    {'X', {"#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#"}},
    {'Y', {"#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.."}},
    {'Z', {"#####", "....#", "...#.", "..#..", ".#...", "#....", "#####"}},
};

static const glyph_t *find_glyph(char c) {
    c = (char)toupper((unsigned char)c);
    for (size_t i = 0; i < sizeof glyphs / sizeof glyphs[0]; i++) {
        if (glyphs[i].c == c) return &glyphs[i];
    }
    return NULL; /* unknown characters take up space but draw nothing */
}

/* ---- drawing ---------------------------------------------------------------- */

static int clamp255(int v) {
    return v < 0 ? 0 : v > 255 ? 255 : v;
}

static uint32_t shade(uint32_t c, int delta) {
    int r = clamp255((int)((c >> 16) & 0xFF) + delta);
    int g = clamp255((int)((c >> 8) & 0xFF) + delta);
    int b = clamp255((int)(c & 0xFF) + delta);
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

static void fill_rect(int x, int y, int w, int h, uint32_t rgb) {
    if (w <= 0 || h <= 0) return;
    gfx_color((int64_t)rgb);
    gfx_rect(x, y, w, h);
}

static void draw_text(int x, int y, const char *t, uint32_t fg) {
    gfx_color((int64_t)fg);
    for (; *t; t++, x += UI_ADVANCE) {
        const glyph_t *g = find_glyph(*t);
        if (!g) continue;
        for (int r = 0; r < 7; r++) {
            for (int c = 0; c < 5; c++) {
                if (g->rows[r][c] == '#') {
                    gfx_rect(x + c * UI_SCALE, y + r * UI_SCALE, UI_SCALE, UI_SCALE);
                }
            }
        }
    }
}

static int bitmap_text_width(const char *t) {
    size_t n = strlen(t);
    return n ? (int)n * UI_ADVANCE - UI_SCALE : 0;
}

void ui_custom_draw(const ui_widget *w) {
    switch (w->kind) {
    case UI_STACK:
        if (w->bg) fill_rect(w->x, w->y, w->w, w->h, w->bg);
        for (int i = 0; i < w->nchildren; i++) ui_custom_draw(w->children[i]);
        return;

    case UI_LABEL:
        if (w->bg) fill_rect(w->x, w->y, w->w, w->h, w->bg);
        draw_text(w->x, w->y + (w->h - UI_GLYPH_H) / 2, ui_text_of(w), w->fg);
        return;

    case UI_BUTTON: {
        uint32_t bg = w->bg;
        if (w->pressed) bg = shade(bg, -UI_SHADE_STEP);
        else if (w->hovered) bg = shade(bg, UI_SHADE_STEP);
        fill_rect(w->x, w->y, w->w, w->h, bg);

        fill_rect(w->x, w->y, w->w, 1, UI_BORDER);
        fill_rect(w->x, w->y + w->h - 1, w->w, 1, UI_BORDER);
        fill_rect(w->x, w->y, 1, w->h, UI_BORDER);
        fill_rect(w->x + w->w - 1, w->y, 1, w->h, UI_BORDER);

        const char *t = ui_text_of(w);
        int tx = w->x + (w->w - bitmap_text_width(t)) / 2;
        int ty = w->y + (w->h - UI_GLYPH_H) / 2;
        draw_text(tx, ty, t, w->fg);
        return;
    }
    }
}

/* ---- frame loop ------------------------------------------------------------- */

static ui_widget *g_root;

static int inside(const ui_widget *w, int x, int y) {
    return x >= w->x && x < w->x + w->w && y >= w->y && y < w->y + w->h;
}

static void update_pointer(ui_widget *w, int mx, int my, int down) {
    if (w->kind == UI_BUTTON) {
        w->hovered = inside(w, mx, my);
        w->pressed = w->hovered && down;
    }
    for (int i = 0; i < w->nchildren; i++) update_pointer(w->children[i], mx, my, down);
}

static void custom_frame(ui_widget *root) {
    ui_layout(root, 0, 0, (int)gfx_width(), (int)gfx_height());
    update_pointer(root, (int)gfx_mouse_x(), (int)gfx_mouse_y(), (int)gfx_mouse_down(0));

    /* Clicks are queued by ember-gfx during the previous present. */
    while (gfx_poll_click()) {
        if (gfx_click_button() == 0) ui_click(root, (int)gfx_click_x(), (int)gfx_click_y());
        if (gfx_closed()) return;
    }

    gfx_clear((int64_t)root->bg);
    ui_custom_draw(root);
    gfx_present();
}

static int custom_open(const char *title, int width, int height) {
    (void)title; /* ember-gfx windows have no title */
    return gfx_open(width, height) == 1;
}

static void custom_set_root(ui_widget *root) {
    g_root = root;
}

static int custom_step(void) {
    if (gfx_closed()) return 0;
    if (g_root) custom_frame(g_root);
    else gfx_present();
    gfx_sleep_ms(16);
    return !gfx_closed();
}

static int custom_run(void) {
    while (custom_step()) {
    }
    return 0;
}

static void custom_quit(void) {
    gfx_close();
}

const ui_backend ui_backend_custom = {
    .name = "custom",
    .open = custom_open,
    .set_root = custom_set_root,
    .run = custom_run,
    .step = custom_step,
    .quit = custom_quit,
};
