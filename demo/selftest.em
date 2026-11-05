# Deterministic check of the Ember bindings, run headless (GFX_BACKEND=null).
# Builds a small tree, then prints geometry, hits, queued clicks and pixel readbacks.
extern def gfx_get_pixel(x: i64, y: i64) -> i64

extern def emui_open(w: i64, h: i64) -> i64
extern def emui_close()
extern def emui_step() -> i64
extern def emui_set_root(id: i64)
extern def emui_stack(vertical: i64, padding: i64, spacing: i64) -> i64
extern def emui_label(fg: i64) -> i64
extern def emui_button() -> i64
extern def emui_add(parent: i64, child: i64) -> i64
extern def emui_remove(parent: i64, child: i64) -> i64
extern def emui_destroy(id: i64)
extern def emui_set_text(id: i64)
extern def emui_set_colors(id: i64, fg: i64, bg: i64)
extern def emui_text_clear()
extern def emui_text_put(c: i64)
extern def emui_text_int(n: i64)
extern def emui_text_from_array(a: [i64]) -> i64
extern def emui_poll_event() -> i64
extern def emui_event_widget() -> i64
extern def emui_click(x: i64, y: i64) -> i64
extern def emui_hit(x: i64, y: i64) -> i64
extern def emui_x(id: i64) -> i64
extern def emui_y(id: i64) -> i64
extern def emui_w(id: i64) -> i64
extern def emui_h(id: i64) -> i64

def main():
    if emui_open(200, 120) == 0:
        print(-1)
        return

    root = emui_stack(1, 8, 6)
    emui_text_clear()
    emui_text_put(72)                 # H
    emui_text_put(73)                 # I
    title = emui_label(0xFFFFFF)
    emui_text_clear()
    emui_text_put(65)                 # A
    a = emui_button()
    emui_text_clear()
    emui_text_put(66)                 # B
    emui_text_put(66)                 # B
    b = emui_button()

    print(emui_add(root, title))      # 1
    print(emui_add(root, a))          # 1
    print(emui_add(root, b))          # 1
    print(emui_add(root, a))          # 0: a already has a parent
    print(emui_add(title, root))      # 0: a label is not a stack

    emui_set_colors(root, 0xFFFFFF, 0x101010)
    emui_set_root(root)
    print(emui_step())                # 1: the window is open

    print(emui_y(a))                  # 28
    print(emui_w(a))                  # 184
    print(emui_h(a))                  # 26
    print(emui_y(b))                  # 60
    print(gfx_get_pixel(0, 0))        # 1052688: the root background, 0x101010
    print(gfx_get_pixel(8, 8))        # 16777215: top-left pixel of the H

    print(emui_hit(50, 40) == a)      # true
    print(emui_hit(50, 70) == b)      # true
    print(emui_hit(50, 3) == root)    # true
    print(emui_hit(500, 3))           # 0

    # Clicks queue up in order; only buttons make them.
    print(emui_click(50, 40))         # 1
    print(emui_click(50, 70))         # 1
    print(emui_click(50, 10))         # 0: the label
    while emui_poll_event() == 1:
        w = emui_event_widget()
        if w == a:
            print(1)                  # 1
        elif w == b:
            print(2)                  # 2

    # A counter driven by Ember control flow. emui_text_int means no itoa is needed.
    emui_text_clear()
    emui_text_put(78)                 # N
    emui_text_put(58)                 # :
    emui_text_int(0)
    counter = emui_label(0xAAFFAA)
    print(emui_add(root, counter))    # 1
    mut presses = 0
    for i in range(5):
        emui_step()
        emui_click(50, 40)
        while emui_poll_event() == 1:
            if emui_event_widget() == a:
                presses = presses + 1
        emui_text_clear()
        emui_text_put(78)
        emui_text_put(58)
        emui_text_int(presses)
        emui_set_text(counter)
    print(presses)                    # 5

    # Text from an Ember array of character codes.
    word = alloc(2)
    word[0] = 79                      # O
    word[1] = 75                      # K
    print(emui_text_from_array(word)) # 2
    free(word)
    emui_set_text(b)

    # Removing keeps a widget alive; destroying retires its id.
    print(emui_remove(root, a))       # 1
    print(emui_remove(root, a))       # 0
    emui_step()
    print(emui_y(b))                  # 28: moved up into a's place
    print(emui_w(a) >= 0)             # true: still valid
    emui_destroy(a)
    print(emui_w(a))                  # -1
    print(emui_add(root, a))          # 0: a stale id does nothing

    emui_close()
    print(emui_step())                # 0: the window is closed
