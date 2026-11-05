/* Ember bindings for ember-ui: handles, a text buffer and a click queue over the C API.
 * See include/ember_ui_em.h. */
#include "ember_ui_em.h"
#include "ui_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEXT_MAX 255
#define QUEUE_MAX 64

/* Ids are indexes into this table plus one. Slots are never reused, so an id that was
 * destroyed can never name a different widget later. */
static ui_widget **g_table;
static int64_t g_count, g_cap;

static ui_widget *g_root;
static int64_t g_root_id;

static char g_text[TEXT_MAX + 1];
static size_t g_text_len;

static int64_t g_queue[QUEUE_MAX];
static int g_qhead, g_qlen;
static int64_t g_event;

static ui_widget *lookup(int64_t id) {
    if (id < 1 || id > g_count) return NULL;
    return g_table[id - 1];
}

static int64_t track(ui_widget *w) {
    if (!w) return 0;
    if (g_count == g_cap) {
        int64_t ncap = g_cap ? g_cap * 2 : 32;
        ui_widget **nt = realloc(g_table, (size_t)ncap * sizeof *nt);
        if (!nt) {
            ui_destroy(w);
            return 0;
        }
        g_table = nt;
        g_cap = ncap;
    }
    g_table[g_count++] = w;
    w->em_id = g_count;
    return w->em_id;
}

/* ---- text buffer ------------------------------------------------------------ */

void emui_text_clear(void) {
    g_text_len = 0;
    g_text[0] = '\0';
}

static void put_byte(unsigned char c) {
    if (c == 0 || g_text_len >= TEXT_MAX) return;
    g_text[g_text_len++] = (char)c;
    g_text[g_text_len] = '\0';
}

void emui_text_put(int64_t c) {
    put_byte((unsigned char)(c & 0xFF));
}

void emui_text_int(int64_t n) {
    char digits[32];
    snprintf(digits, sizeof digits, "%lld", (long long)n);
    for (const char *p = digits; *p; p++) put_byte((unsigned char)*p);
}

int64_t emui_text_from_array(int64_t array) {
    emui_text_clear();
    const int64_t *block = (const int64_t *)(intptr_t)array;
    if (!block) return 0;
    int64_t n = block[0];
    for (int64_t i = 0; i < n && g_text_len < TEXT_MAX; i++) {
        put_byte((unsigned char)(block[1 + i] & 0xFF));
    }
    return (int64_t)g_text_len;
}

int64_t emui_text_len(void) {
    return (int64_t)g_text_len;
}

/* ---- window and loop -------------------------------------------------------- */

int64_t emui_open(int64_t width, int64_t height) {
    if (width < 1 || height < 1) return 0;
    return ui_open_polled(g_text_len ? g_text : "Ember UI", (int)width, (int)height);
}

void emui_close(void) {
    ui_quit();
}

void emui_set_root(int64_t id) {
    ui_widget *w = lookup(id);
    if (id != 0 && !w) return;
    g_root = w;
    g_root_id = w ? id : 0;
    ui_set_root(w);
}

int64_t emui_step(void) {
    return ui_step();
}

/* ---- building the tree ------------------------------------------------------ */

int64_t emui_stack(int64_t vertical, int64_t padding, int64_t spacing) {
    return track(ui_stack(vertical != 0, (int)padding, (int)spacing));
}

int64_t emui_label(int64_t fg) {
    return track(ui_label(g_text, (uint32_t)fg & 0xFFFFFF));
}

/* Clicks go to a queue because Ember has no way to pass a function. */
static void queue_click(ui_widget *self, void *user) {
    (void)user;
    if (g_qlen == QUEUE_MAX) return; /* full: drop, as ember-gfx does */
    g_queue[(g_qhead + g_qlen) % QUEUE_MAX] = self->em_id;
    g_qlen++;
}

int64_t emui_button(void) {
    return track(ui_button(g_text, queue_click, NULL));
}

int64_t emui_add(int64_t parent, int64_t child) {
    return ui_add(lookup(parent), lookup(child));
}

int64_t emui_remove(int64_t parent, int64_t child) {
    return ui_remove(lookup(parent), lookup(child));
}

/* Retire the ids of w and everything below it, and drop the root if it is among them. */
static void forget(ui_widget *w) {
    if (w->em_id) g_table[w->em_id - 1] = NULL;
    if (w == g_root) {
        g_root = NULL;
        g_root_id = 0;
        ui_set_root(NULL);
    }
    for (int i = 0; i < w->nchildren; i++) forget(w->children[i]);
}

void emui_destroy(int64_t id) {
    ui_widget *w = lookup(id);
    if (!w) return;
    forget(w);
    ui_destroy(w);
}

void emui_set_text(int64_t id) {
    ui_widget *w = lookup(id);
    if (w && w->kind != UI_STACK) ui_set_text(w, g_text);
}

void emui_set_colors(int64_t id, int64_t fg, int64_t bg) {
    ui_widget *w = lookup(id);
    if (w) ui_set_colors(w, (uint32_t)fg & 0xFFFFFF, (uint32_t)bg & 0xFFFFFF);
}

/* ---- input ------------------------------------------------------------------ */

int64_t emui_poll_event(void) {
    if (g_qlen == 0) return 0;
    g_event = g_queue[g_qhead];
    g_qhead = (g_qhead + 1) % QUEUE_MAX;
    g_qlen--;
    return 1;
}

int64_t emui_event_widget(void) {
    return g_event;
}

int64_t emui_click(int64_t x, int64_t y) {
    if (!g_root) return 0;
    return ui_click(g_root, (int)x, (int)y);
}

int64_t emui_hit(int64_t x, int64_t y) {
    if (!g_root) return 0;
    ui_widget *w = ui_hit_test(g_root, (int)x, (int)y);
    return w ? w->em_id : 0;
}

static int64_t geometry(int64_t id, int which) {
    const ui_widget *w = lookup(id);
    if (!w) return -1;
    int x, y, width, height;
    ui_get_rect(w, &x, &y, &width, &height);
    return which == 0 ? x : which == 1 ? y : which == 2 ? width : height;
}

int64_t emui_x(int64_t id) { return geometry(id, 0); }
int64_t emui_y(int64_t id) { return geometry(id, 1); }
int64_t emui_w(int64_t id) { return geometry(id, 2); }
int64_t emui_h(int64_t id) { return geometry(id, 3); }
