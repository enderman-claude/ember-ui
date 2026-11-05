#include "ember_ui.h"
#include "ui_internal.h"

#include <stdlib.h>
#include <string.h>

static const ui_backend *g_backend;

/* Backends in order of preference. The first one that opens the window is used. */
static const ui_backend *const backends[] = {
#ifdef UI_HAVE_GTK
    &ui_backend_gtk,
#endif
#ifdef UI_HAVE_WIN32
    &ui_backend_win32,
#endif
#ifdef UI_HAVE_CUSTOM
    &ui_backend_custom,
#endif
    NULL,
};

static char *dup_str(const char *s) {
    if (!s) s = "";
    size_t n = strlen(s) + 1;
    char *d = malloc(n);
    if (d) memcpy(d, s, n);
    return d;
}

static void text_size(const char *t, int *w, int *h) {
    if (g_backend && g_backend->measure_text && g_backend->measure_text(t, w, h)) return;
    size_t n = strlen(t);
    *w = n ? (int)n * UI_ADVANCE - UI_SCALE : 0;
    *h = UI_GLYPH_H;
}

static int inside(const ui_widget *w, int x, int y) {
    return x >= w->x && x < w->x + w->w && y >= w->y && y < w->y + w->h;
}

/* ---- construction ----------------------------------------------------------- */

ui_widget *ui_stack(int vertical, int padding, int spacing) {
    ui_widget *w = calloc(1, sizeof *w);
    if (!w) return NULL;
    w->kind = UI_STACK;
    w->vertical = vertical ? 1 : 0;
    w->padding = padding;
    w->spacing = spacing;
    return w;
}

ui_widget *ui_label(const char *text, uint32_t fg) {
    ui_widget *w = calloc(1, sizeof *w);
    if (!w) return NULL;
    w->kind = UI_LABEL;
    w->text = dup_str(text);
    w->fg = fg;
    return w;
}

ui_widget *ui_button(const char *text, ui_click_fn on_click, void *user) {
    ui_widget *w = calloc(1, sizeof *w);
    if (!w) return NULL;
    w->kind = UI_BUTTON;
    w->text = dup_str(text);
    w->fg = UI_BUTTON_FG;
    w->bg = UI_BUTTON_BG;
    w->on_click = on_click;
    w->user = user;
    return w;
}

int ui_add(ui_widget *parent, ui_widget *child) {
    if (!parent || !child || parent->kind != UI_STACK) return 0;
    if (child->parent) return 0; /* a widget has one owner */
    for (const ui_widget *a = parent; a; a = a->parent) {
        if (a == child) return 0; /* would make a cycle */
    }
    if (parent->nchildren == parent->cap) {
        int ncap = parent->cap ? parent->cap * 2 : 4;
        ui_widget **nc = realloc(parent->children, (size_t)ncap * sizeof *nc);
        if (!nc) return 0;
        parent->children = nc;
        parent->cap = ncap;
    }
    parent->children[parent->nchildren++] = child;
    child->parent = parent;
    return 1;
}

int ui_remove(ui_widget *parent, ui_widget *child) {
    if (!parent || !child || child->parent != parent) return 0;
    for (int i = 0; i < parent->nchildren; i++) {
        if (parent->children[i] != child) continue;
        memmove(&parent->children[i], &parent->children[i + 1],
                (size_t)(parent->nchildren - i - 1) * sizeof *parent->children);
        parent->nchildren--;
        child->parent = NULL;
        return 1;
    }
    return 0;
}

void ui_set_text(ui_widget *w, const char *text) {
    if (!w) return;
    char *n = dup_str(text);
    if (!n) return;
    free(w->text);
    w->text = n;
    if (g_backend && g_backend->text_changed) g_backend->text_changed(w);
}

void ui_set_colors(ui_widget *w, uint32_t fg, uint32_t bg) {
    if (!w) return;
    w->fg = fg;
    w->bg = bg;
}

static void destroy_tree(ui_widget *w) {
    for (int i = 0; i < w->nchildren; i++) destroy_tree(w->children[i]);
    free(w->children);
    free(w->text);
    free(w);
}

void ui_destroy(ui_widget *w) {
    if (!w) return;
    if (w->parent) ui_remove(w->parent, w);
    destroy_tree(w);
}

/* ---- layout ----------------------------------------------------------------- */

void ui_measure(const ui_widget *w, int *width, int *height) {
    switch (w->kind) {
    case UI_LABEL:
        text_size(ui_text_of(w), width, height);
        return;
    case UI_BUTTON: {
        int tw, th;
        text_size(ui_text_of(w), &tw, &th);
        *width = tw + 2 * UI_BTN_PADX;
        *height = th + 2 * UI_BTN_PADY;
        return;
    }
    case UI_STACK:
        break;
    }

    int main_len = 0, cross_len = 0;
    for (int i = 0; i < w->nchildren; i++) {
        int cw, ch;
        ui_measure(w->children[i], &cw, &ch);
        if (w->vertical) {
            main_len += ch;
            if (cw > cross_len) cross_len = cw;
        } else {
            main_len += cw;
            if (ch > cross_len) cross_len = ch;
        }
    }
    if (w->nchildren > 1) main_len += w->spacing * (w->nchildren - 1);

    if (w->vertical) {
        *width = cross_len + 2 * w->padding;
        *height = main_len + 2 * w->padding;
    } else {
        *width = main_len + 2 * w->padding;
        *height = cross_len + 2 * w->padding;
    }
}

void ui_layout(ui_widget *w, int x, int y, int width, int height) {
    w->x = x;
    w->y = y;
    w->w = width;
    w->h = height;
    if (w->kind != UI_STACK) return;

    int ix = x + w->padding, iy = y + w->padding;
    int iw = width - 2 * w->padding, ih = height - 2 * w->padding;
    if (iw < 0) iw = 0;
    if (ih < 0) ih = 0;

    int cursor = w->vertical ? iy : ix;
    for (int i = 0; i < w->nchildren; i++) {
        ui_widget *c = w->children[i];
        int cw, ch;
        ui_measure(c, &cw, &ch);
        if (w->vertical) {
            ui_layout(c, ix, cursor, iw, ch);
            cursor += ch + w->spacing;
        } else {
            ui_layout(c, cursor, iy, cw, ih);
            cursor += cw + w->spacing;
        }
    }
}

void ui_get_rect(const ui_widget *w, int *x, int *y, int *width, int *height) {
    *x = w->x;
    *y = w->y;
    *width = w->w;
    *height = w->h;
}

/* ---- input ------------------------------------------------------------------ */

ui_widget *ui_hit_test(ui_widget *w, int x, int y) {
    if (!w || !inside(w, x, y)) return NULL;
    for (int i = w->nchildren - 1; i >= 0; i--) {
        ui_widget *hit = ui_hit_test(w->children[i], x, y);
        if (hit) return hit;
    }
    return w;
}

int ui_click(ui_widget *root, int x, int y) {
    ui_widget *hit = ui_hit_test(root, x, y);
    if (!hit || hit->kind != UI_BUTTON) return 0;
    if (hit->on_click) hit->on_click(hit, hit->user);
    return 1;
}

/* ---- application ------------------------------------------------------------ */

static int open_backend(const char *title, int width, int height, int polled) {
    const char *want = getenv("EMBER_UI_BACKEND");
    for (int i = 0; backends[i]; i++) {
        const ui_backend *b = backends[i];
        if (want && *want && strcmp(want, b->name) != 0) continue;
        if (polled && !b->step) continue;
        if (b->open(title, width, height)) {
            g_backend = b;
            return 1;
        }
    }
    return 0;
}

int ui_open(const char *title, int width, int height) {
    return open_backend(title, width, height, 0);
}

int ui_open_polled(const char *title, int width, int height) {
    return open_backend(title, width, height, 1);
}

const char *ui_backend_name(void) {
    return g_backend ? g_backend->name : "none";
}

void ui_set_root(ui_widget *root) {
    if (g_backend) g_backend->set_root(root);
}

int ui_run(void) {
    return g_backend ? g_backend->run() : 0;
}

int ui_step(void) {
    return g_backend && g_backend->step ? g_backend->step() : 0;
}

void ui_quit(void) {
    if (g_backend) g_backend->quit();
}
