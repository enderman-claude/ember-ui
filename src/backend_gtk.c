/* GTK 3 backend: each widget is a real GTK control, laid out by GtkBox. */
#include "ui_internal.h"

#include <gtk/gtk.h>

static GtkWidget *g_window;
static GtkWidget *g_content;

static void on_clicked(GtkButton *button, gpointer data) {
    (void)button;
    ui_widget *w = data;
    if (w->on_click) w->on_click(w, w->user);
}

static void on_destroy(GtkWidget *window, gpointer data) {
    (void)window;
    (void)data;
    g_window = NULL;
    gtk_main_quit();
}

static GtkWidget *build(ui_widget *w) {
    GtkWidget *gw = NULL;
    switch (w->kind) {
    case UI_LABEL:
        gw = gtk_label_new(ui_text_of(w));
        gtk_widget_set_halign(gw, GTK_ALIGN_START);
        break;
    case UI_BUTTON:
        gw = gtk_button_new_with_label(ui_text_of(w));
        g_signal_connect(gw, "clicked", G_CALLBACK(on_clicked), w);
        break;
    case UI_STACK:
        gw = gtk_box_new(w->vertical ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL,
                         w->spacing);
        gtk_container_set_border_width(GTK_CONTAINER(gw), (guint)w->padding);
        for (int i = 0; i < w->nchildren; i++) {
            GtkWidget *cw = build(w->children[i]);
            if (cw) gtk_box_pack_start(GTK_BOX(gw), cw, FALSE, TRUE, 0);
        }
        break;
    }
    w->native = gw;
    return gw;
}

static int gtk_open(const char *title, int width, int height) {
    int argc = 1;
    char arg0[] = "ember-ui";
    char *argv_storage[] = {arg0, NULL};
    char **argv = argv_storage;
    if (!gtk_init_check(&argc, &argv)) return 0; /* no display */

    g_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(g_window), title);
    gtk_window_set_default_size(GTK_WINDOW(g_window), width, height);
    g_signal_connect(g_window, "destroy", G_CALLBACK(on_destroy), NULL);
    return 1;
}

static void gtk_set_root(ui_widget *root) {
    if (!g_window) return;
    if (g_content) gtk_widget_destroy(g_content);
    g_content = build(root);
    if (!g_content) return;
    gtk_container_add(GTK_CONTAINER(g_window), g_content);
    gtk_widget_show_all(g_window);
}

static void gtk_text_changed(ui_widget *w) {
    if (!w->native) return;
    if (w->kind == UI_LABEL) {
        gtk_label_set_text(GTK_LABEL(w->native), ui_text_of(w));
    } else if (w->kind == UI_BUTTON) {
        gtk_button_set_label(GTK_BUTTON(w->native), ui_text_of(w));
    }
}

static int gtk_run(void) {
    gtk_main();
    return 0;
}

static void gtk_quit(void) {
    gtk_main_quit();
}

const ui_backend ui_backend_gtk = {
    .name = "gtk",
    .open = gtk_open,
    .set_root = gtk_set_root,
    .text_changed = gtk_text_changed,
    .run = gtk_run,
    .quit = gtk_quit,
};
