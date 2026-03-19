#include <cstring>
#include <screen.h>
#include <action.h>
#include <bitmap.h>

Notification::Notification(std::string message) : message(message) {};

void Notification::set_message(std::string new_message) {
    message = new_message;
}

void Notification::process_navigation(
    unsigned long button_select_press_duration,
    bool button_up_clicked,
    bool button_down_clicked
) {
    if(button_select_press_duration > 50 || button_down_clicked || button_down_clicked) {
        extern void open_prev_screen();
        open_prev_screen();
    }
}

void Notification::draw(U8G2 &u8g2, int offset_y) {
    u8g2.drawRBox(10, 10, 128 - 20, 64 - 20, 2);
    u8g2.setDrawColor(0);
    u8g2.drawRBox(12, 12, 128 - 24, 64 - 24, 2);
    u8g2.setDrawColor(1);

    u8g2.setFont(u8g2_font_6x10_tf);

    int current_y = 15 + 7;
    int current_x = 15;
    char *buff = (char*)message.c_str();
    char *start_ptr = buff;
    for(unsigned i = 0; i <= message.size(); i++) {
        if(buff[i] == '\n') {
            buff[i] = 0;
            u8g2.setCursor(current_x, current_y);
            u8g2.print(start_ptr);
            current_x = 15;
            current_y += 7 + 2;
            start_ptr = buff + i + 1;
        } else if(buff[i] == 0) {
            u8g2.setCursor(current_x, current_y);
            u8g2.print(start_ptr);
        }
    }
    redraw_request = false;
}

bool Notification::is_overlay() {
    return true;
}
