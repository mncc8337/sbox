#include <screen.h>
#include <bitmap.h>

void Screen::open_callback() {}

bool Screen::is_blocked() {
    return false;
}

bool Screen::is_overlay() {
    return false;
}

void Screen::close_callback() {}

bool Screen::prevent_sleep() {
    return false;
}

void Screen::request_redraw() {
    redraw_request = true;
}
