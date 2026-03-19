#include <screen.h>
#include <bitmap.h>

bool Screen::is_overlay() {
    return false;
}

void Screen::request_redraw() {
    redraw_request = true;
}
