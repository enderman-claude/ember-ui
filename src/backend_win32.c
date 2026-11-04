/* Win32 backend: each widget is a real system control (STATIC, BUTTON), so it is
 * drawn by Windows and follows the system theme. Layout comes from ui_layout, and
 * native control sizes come from the system font. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "ui_internal.h"

#include <string.h>

#define UI_WIN_CLASS "EmberUIWindow"

static HWND g_hwnd;
static HINSTANCE g_inst;
static ui_widget *g_root;

static HFONT ui_font(void) {
    return (HFONT)GetStockObject(DEFAULT_GUI_FONT);
}

static void apply_layout(const ui_widget *w) {
    if (w->native) MoveWindow((HWND)w->native, w->x, w->y, w->w, w->h, TRUE);
    for (int i = 0; i < w->nchildren; i++) apply_layout(w->children[i]);
}

static void relayout(void) {
    if (!g_root || !g_hwnd) return;
    RECT rc;
    GetClientRect(g_hwnd, &rc);
    ui_layout(g_root, 0, 0, (int)(rc.right - rc.left), (int)(rc.bottom - rc.top));
    apply_layout(g_root);
}

static void build_native(ui_widget *w) {
    switch (w->kind) {
    case UI_LABEL:
        w->native = CreateWindowExA(0, "STATIC", ui_text_of(w), WS_CHILD | WS_VISIBLE | SS_LEFT,
                                    0, 0, 0, 0, g_hwnd, NULL, g_inst, NULL);
        break;
    case UI_BUTTON:
        w->native = CreateWindowExA(0, "BUTTON", ui_text_of(w),
                                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                    0, 0, 0, 0, g_hwnd, NULL, g_inst, NULL);
        break;
    case UI_STACK:
        break;
    }
    if (w->native) {
        SetWindowLongPtrA((HWND)w->native, GWLP_USERDATA, (LONG_PTR)w);
        SendMessageA((HWND)w->native, WM_SETFONT, (WPARAM)ui_font(), TRUE);
    }
    for (int i = 0; i < w->nchildren; i++) build_native(w->children[i]);
}

static void destroy_native(ui_widget *w) {
    if (w->native) {
        DestroyWindow((HWND)w->native);
        w->native = NULL;
    }
    for (int i = 0; i < w->nchildren; i++) destroy_native(w->children[i]);
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_SIZE:
        relayout();
        return 0;
    case WM_COMMAND:
        if (HIWORD(wp) == BN_CLICKED) {
            ui_widget *w = (ui_widget *)GetWindowLongPtrA((HWND)lp, GWLP_USERDATA);
            if (w && w->on_click) w->on_click(w, w->user);
        }
        return 0;
    case WM_DESTROY:
        if (hwnd == g_hwnd) {
            g_hwnd = NULL;
            PostQuitMessage(0);
        }
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static int win_open(const char *title, int width, int height) {
    WNDCLASSA wc;
    memset(&wc, 0, sizeof wc);
    g_inst = GetModuleHandleA(NULL);
    wc.lpfnWndProc = wndproc;
    wc.hInstance = g_inst;
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = UI_WIN_CLASS;
    if (!RegisterClassA(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return 0;

    RECT r = {0, 0, width, height};
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    g_hwnd = CreateWindowExA(0, UI_WIN_CLASS, title, WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
                             NULL, NULL, g_inst, NULL);
    if (!g_hwnd) return 0;
    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);
    return 1;
}

static void win_set_root(ui_widget *root) {
    if (!g_hwnd) return;
    if (g_root) destroy_native(g_root);
    g_root = root;
    build_native(root);
    relayout();
}

static void win_text_changed(ui_widget *w) {
    if (!w->native) return;
    SetWindowTextA((HWND)w->native, ui_text_of(w));
    relayout(); /* the new text may change the size */
}

static int win_run(void) {
    MSG m;
    while (GetMessageA(&m, NULL, 0, 0) > 0) {
        TranslateMessage(&m);
        DispatchMessageA(&m);
    }
    return 0;
}

static void win_quit(void) {
    PostQuitMessage(0);
}

static int win_measure_text(const char *text, int *width, int *height) {
    HDC dc = GetDC(NULL);
    if (!dc) return 0;
    HGDIOBJ old = SelectObject(dc, ui_font());
    SIZE sz = {0, 0};
    GetTextExtentPoint32A(dc, text, (int)strlen(text), &sz);
    SelectObject(dc, old);
    ReleaseDC(NULL, dc);
    *width = (int)sz.cx;
    *height = (int)sz.cy;
    return 1;
}

const ui_backend ui_backend_win32 = {
    .name = "win32",
    .open = win_open,
    .set_root = win_set_root,
    .text_changed = win_text_changed,
    .run = win_run,
    .quit = win_quit,
    .measure_text = win_measure_text,
};
