#!/bin/sh
# End-to-end test of the demo on a real X server: clicks buttons with xdotool,
# saves a screenshot, and checks that QUIT exits with status 0.
#
#   tests/e2e_ui.sh [backend ...]     default: gtk custom
#
# Needs a running X server ($DISPLAY), xdotool, and ImageMagick's `import`.
# Screenshots are written to build/e2e_<backend>.png.
BIN=${BIN:-build/ui_demo}
[ -x "$BIN" ] || { echo "missing $BIN (run make demo)"; exit 1; }
[ -n "$DISPLAY" ] || { echo "DISPLAY is not set"; exit 1; }

# Button centres in window coordinates (the window is at 0,0 without a window manager).
CLICK_X=65; RESET_X=163; QUIT_X=245; Y=85

run_case() {
    backend=$1
    EMBER_UI_BACKEND=$backend "$BIN" > "build/e2e_$backend.log" 2>&1 &
    pid=$!
    sleep 2
    for i in 1 2 3; do xdotool mousemove $CLICK_X $Y click 1; sleep 0.3; done
    sleep 0.5
    import -window root -crop 320x240+0+0 "build/e2e_$backend.png" || return 1
    xdotool mousemove $RESET_X $Y click 1; sleep 0.5
    xdotool mousemove $QUIT_X $Y click 1
    wait $pid
    code=$?
    if [ $code -eq 0 ]; then
        echo "$backend: ok (quit with status 0)"
    else
        echo "$backend: FAILED (exit status $code)"
        return 1
    fi
}

mkdir -p build
status=0
if [ $# -eq 0 ]; then set -- gtk custom; fi
for b in "$@"; do run_case "$b" || status=1; done
exit $status
