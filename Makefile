# ember-ui build.
#
#   make                 Linux: custom backend, plus GTK if GTK 3 is installed
#   make test            headless tests (layout, hit testing, custom drawing)
#   make demo            build build/ui_demo
#   make WIN32=1 demo    Windows: build build/ui_demo.exe (Win32 backend, needs MinGW)
#
# GFX points at the ember-gfx checkout (default ../ember-gfx).

GFX ?= ../ember-gfx
BUILD := build
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror
CFLAGS += -Iinclude -Isrc

ifeq ($(WIN32),1)
  CC := x86_64-w64-mingw32-gcc
  RC := x86_64-w64-mingw32-windres
  DEFS := -DUI_HAVE_WIN32
  BACKEND_SRC := src/backend_win32.c
  PLATFORM_LIBS := -lgdi32 -luser32 -lkernel32
  EXE := .exe
  DEMO_RES := $(BUILD)/ui_demo.rc.o
  DEMO_LDFLAGS := -mwindows
else
  CC ?= cc
  GFX_LIB := $(GFX)/build/libember_gfx.a
  X11_LIBS := $(shell pkg-config --libs x11 xext xtst 2>/dev/null)
  GTK_CFLAGS := $(shell pkg-config --cflags gtk+-3.0 2>/dev/null)
  GTK_LIBS := $(shell pkg-config --libs gtk+-3.0 2>/dev/null)
  DEFS := -DUI_HAVE_CUSTOM
  BACKEND_SRC := src/backend_custom.c
  CFLAGS += -I$(GFX)/include
  PLATFORM_LIBS := $(GFX_LIB) $(X11_LIBS) -lm
  ifneq ($(GTK_LIBS),)
    DEFS += -DUI_HAVE_GTK
    BACKEND_SRC += src/backend_gtk.c
    CFLAGS += $(GTK_CFLAGS)
    PLATFORM_LIBS += $(GTK_LIBS)
  endif
  EXE :=
  DEMO_RES :=
  DEMO_LDFLAGS :=
endif

UI_SRC := src/ember_ui.c $(BACKEND_SRC)
UI_OBJ := $(patsubst src/%.c,$(BUILD)/%.o,$(UI_SRC))
UI_LIB := $(BUILD)/libember_ui.a

all: $(UI_LIB)

$(BUILD):
	mkdir -p $(BUILD)

$(GFX)/build/libember_gfx.a:
	$(MAKE) -C $(GFX)

$(BUILD)/%.o: src/%.c src/ui_internal.h include/ember_ui.h | $(BUILD)
	$(CC) $(CFLAGS) $(DEFS) -c $< -o $@

$(UI_LIB): $(UI_OBJ)
	ar rcs $@ $^

$(BUILD)/ui_demo.rc.o: demo/ui_demo.rc demo/ui_demo.manifest | $(BUILD)
	$(RC) --include-dir demo -O coff $< -o $@

$(BUILD)/ui_demo$(EXE): demo/ui_demo.c $(UI_LIB) $(DEMO_RES)
	$(CC) $(CFLAGS) $(DEFS) demo/ui_demo.c $(UI_LIB) $(DEMO_RES) $(PLATFORM_LIBS) $(DEMO_LDFLAGS) -o $@

demo: $(BUILD)/ui_demo$(EXE)

# The tests use the custom backend, so they run only on the Linux build.
ifneq ($(WIN32),1)
$(BUILD)/test_ui: tests/test_ui.c $(UI_LIB) $(GFX_LIB)
	$(CC) $(CFLAGS) $(DEFS) tests/test_ui.c $(UI_LIB) $(PLATFORM_LIBS) -o $@

test: $(BUILD)/test_ui
	GFX_BACKEND=null ./$(BUILD)/test_ui
endif

clean:
	rm -rf $(BUILD)

.PHONY: all demo test clean
