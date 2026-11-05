#ifndef EMBER_UI_INTERNAL_H
#define EMBER_UI_INTERNAL_H

#include "ember_ui.h"

/* Bitmap font metrics. Used for measuring text when a backend has no measurement of its own. */
#define UI_SCALE 2
#define UI_ADVANCE (6 * UI_SCALE) /* 5 columns of glyph, 1 column of gap */
#define UI_GLYPH_H (7 * UI_SCALE)

#define UI_BTN_PADX 10
#define UI_BTN_PADY 6
#define UI_BUTTON_BG 0x3A3A6A
#define UI_BUTTON_FG 0xFFFFFF
#define UI_BORDER 0x8888CC
#define UI_SHADE_STEP 24

typedef enum { UI_STACK, UI_LABEL, UI_BUTTON } ui_kind;

struct ui_widget {
    ui_kind kind;
    int vertical;
    int padding, spacing;
    char *text;
    uint32_t fg, bg;
    ui_click_fn on_click;
    void *user;
    ui_widget *parent;  /* the stack that owns this widget, or NULL */
    ui_widget **children;
    int nchildren, cap;
    int x, y, w, h;
    int hovered, pressed;
    void *native; /* the backend's control for this widget, if it has one */
    int64_t em_id; /* handle given out by the Ember bindings (ember_ui_em.c), or 0 */
};

typedef struct ui_backend {
    const char *name;
    int (*open)(const char *title, int width, int height); /* 1 on success */
    void (*set_root)(ui_widget *root);
    void (*text_changed)(ui_widget *w);                     /* may be NULL */
    int (*run)(void);                                       /* blocks until closed */
    int (*step)(void);                                      /* one frame; 0 once closed; may be NULL */
    void (*quit)(void);
    int (*measure_text)(const char *text, int *width, int *height); /* may be NULL */
} ui_backend;

#ifdef UI_HAVE_CUSTOM
extern const ui_backend ui_backend_custom;
#endif
#ifdef UI_HAVE_GTK
extern const ui_backend ui_backend_gtk;
#endif
#ifdef UI_HAVE_WIN32
extern const ui_backend ui_backend_win32;
#endif

static inline const char *ui_text_of(const ui_widget *w) {
    return w->text ? w->text : "";
}

#endif
