#include <screen.h>
#include <bitmap.h>

static unsigned long last_draw_ts = 0;

void SplashScreen::process_navigation(
    unsigned long button_select_press_duration,
    bool button_up_clicked,
    bool button_down_clicked
) {};

void SplashScreen::draw(U8G2 &u8g2) {
    u8g2.setBitmapMode(0);
    for(unsigned i = 0; i < BITMAP_SPLASH_LEN; i++) {
        u8g2.drawXBMP(0, 0, 128, 64, BITMAP_SPLASH[i]);
        u8g2.sendBuffer();
        delay(10);
    }
    u8g2.setBitmapMode(1);
}

