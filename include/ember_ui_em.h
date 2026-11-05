/* ember-ui for Ember programs.
 *
 * Ember has i64, bool and [i64] but no strings, function pointers or structs, so the C
 * API in ember_ui.h cannot be called from it directly. This layer reshapes it to fit,
 * the same way ember-gfx does: every function takes and returns int64_t, so Ember's
 * i64 values pass straight through `extern def`.
 *
 *   Widgets    are integer handles ("ids"). 0 is never a valid id, and ids are not
 *              reused, so a stale id is safe: every function ignores it (returning 0, or
 *              -1 for the geometry getters). A widget created here must be destroyed
 *              with emui_destroy, not ui_destroy.
 *   Text       there are no strings, so text goes through one shared text buffer. Fill
 *              it with emui_text_clear, emui_text_put, emui_text_int or
 *              emui_text_from_array, then call a function that uses it (emui_label,
 *              emui_button, emui_set_text, emui_open). The buffer holds 255 bytes; more is
 *              dropped. The built-in font has capitals, digits and  ! + - . : / ?  only.
 *   Clicks     there are no callbacks, so button clicks are queued and polled, like
 *              gfx_poll_click: emui_poll_event() takes the oldest click, and
 *              emui_event_widget() is the button it landed on. The queue holds 64 clicks.
 *   Main loop  there is no blocking ui_run: call emui_step() in a loop. It handles input,
 *              draws, presents and sleeps ~16 ms, and returns 0 once the window is closed.
 *
 * It runs on the custom backend (ember-gfx), so it works wherever ember-gfx does.
 * Native backends (GTK, Win32) cannot be stepped, and emui_open does not use them.
 *
 *   extern def emui_open(w: i64, h: i64) -> i64
 *   extern def emui_stack(vertical: i64, padding: i64, spacing: i64) -> i64
 *   extern def emui_label(fg: i64) -> i64
 *   extern def emui_button() -> i64
 *   extern def emui_add(parent: i64, child: i64) -> i64
 *   extern def emui_set_root(id: i64)
 *   extern def emui_step() -> i64
 *   extern def emui_poll_event() -> i64
 *   extern def emui_event_widget() -> i64
 */
#ifndef EMBER_UI_EM_H
#define EMBER_UI_EM_H

#include <stdint.h>

/* ---- text buffer ------------------------------------------------------------ */

void emui_text_clear(void);
/* Appends one byte (the low 8 bits of c, as put_byte does). Zero bytes are ignored. */
void emui_text_put(int64_t c);
/* Appends n in decimal, so a program can show a number without writing itoa. */
void emui_text_int(int64_t n);
/* Replaces the buffer with the contents of an [i64] array of character codes (the low
 * 8 bits of each element). Returns the number of bytes now in the buffer. Reads the
 * array the way the Ember runtime lays it out: element count in slot 0, then elements. */
int64_t emui_text_from_array(int64_t array);
int64_t emui_text_len(void);

/* ---- window and loop -------------------------------------------------------- */

/* Opens a w x h window; the text buffer is the title (ignored by the custom backend).
 * Returns 1 on success, 0 if no backend could be opened. */
int64_t emui_open(int64_t width, int64_t height);
/* Closes the window. emui_step then returns 0. */
void emui_close(void);
/* Shows the tree under id. Pass 0 to show nothing. */
void emui_set_root(int64_t id);
/* One frame. Returns 1 while the window is open, 0 once it has closed. */
int64_t emui_step(void);

/* ---- building the tree ------------------------------------------------------ */

/* Each returns an id, or 0 if out of memory. emui_label and emui_button take their text
 * from the text buffer; fg is 0xRRGGBB. Buttons start with the default button colours. */
int64_t emui_stack(int64_t vertical, int64_t padding, int64_t spacing);
int64_t emui_label(int64_t fg);
int64_t emui_button(void);

/* Adds child to a stack. 1 on success; 0 if parent is not a stack, if child already has a
 * parent, or if the ids are invalid. */
int64_t emui_add(int64_t parent, int64_t child);
/* Takes child out of parent without destroying it. 1 if it was a child of parent. */
int64_t emui_remove(int64_t parent, int64_t child);
/* Destroys id and everything below it. Their ids become invalid. */
void emui_destroy(int64_t id);

/* Sets the widget's text from the text buffer. */
void emui_set_text(int64_t id);
void emui_set_colors(int64_t id, int64_t fg, int64_t bg);

/* ---- input ------------------------------------------------------------------ */

/* 1 if a click was taken from the queue, 0 if it was empty. */
int64_t emui_poll_event(void);
/* The button of the click taken by the last successful emui_poll_event. */
int64_t emui_event_widget(void);

/* Injects a press at (x, y) as if the pointer had clicked there, for tests. 1 if it landed
 * on a button (and so queued a click). Uses the layout from the last emui_step. */
int64_t emui_click(int64_t x, int64_t y);
/* Deepest widget at (x, y) after the last emui_step, or 0. */
int64_t emui_hit(int64_t x, int64_t y);

/* Geometry from the last emui_step, in window pixels. -1 for an invalid id. */
int64_t emui_x(int64_t id);
int64_t emui_y(int64_t id);
int64_t emui_w(int64_t id);
int64_t emui_h(int64_t id);

#endif
