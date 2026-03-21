#include <screen.h>
#include <bitmap.h>

bool Screen::is_overlay() {
    return false;
}

bool Screen::prevent_sleep() {
    return false;
}

void Screen::request_redraw() {
    redraw_request = true;
}
