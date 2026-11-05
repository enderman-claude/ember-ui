# A counter, written in Ember. Click the button to count; click RESET to start again.
# Build and run:  make demo-ember
extern def emui_open(w: i64, h: i64) -> i64
extern def emui_step() -> i64
extern def emui_set_root(id: i64)
extern def emui_stack(vertical: i64, padding: i64, spacing: i64) -> i64
extern def emui_label(fg: i64) -> i64
extern def emui_button() -> i64
extern def emui_add(parent: i64, child: i64) -> i64
extern def emui_set_text(id: i64)
extern def emui_set_colors(id: i64, fg: i64, bg: i64)
extern def emui_text_clear()
extern def emui_text_put(c: i64)
extern def emui_text_int(n: i64)
extern def emui_poll_event() -> i64
extern def emui_event_widget() -> i64

def show_count(label: i64, n: i64):
    emui_text_clear()
    emui_text_put(67)                 # C
    emui_text_put(79)                 # O
    emui_text_put(85)                 # U
    emui_text_put(78)                 # N
    emui_text_put(84)                 # T
    emui_text_put(58)                 # :
    emui_text_put(32)
    emui_text_int(n)
    emui_set_text(label)

def main():
    if emui_open(240, 150) == 0:
        print(0)
        return
    root = emui_stack(1, 12, 10)
    emui_set_colors(root, 0xFFFFFF, 0x202838)

    emui_text_clear()
    emui_text_put(67)                 # C
    counter = emui_label(0xFFFFFF)
    show_count(counter, 0)

    emui_text_clear()
    emui_text_put(43)                 # +
    emui_text_put(49)                 # 1
    plus = emui_button()

    emui_text_clear()
    emui_text_put(82)                 # R
    emui_text_put(69)                 # E
    emui_text_put(83)                 # S
    emui_text_put(69)                 # E
    emui_text_put(84)                 # T
    reset = emui_button()

    emui_add(root, counter)
    emui_add(root, plus)
    emui_add(root, reset)
    emui_set_root(root)

    mut n = 0
    while emui_step() == 1:
        while emui_poll_event() == 1:
            w = emui_event_widget()
            if w == plus:
                n = n + 1
            elif w == reset:
                n = 0
            show_count(counter, n)
