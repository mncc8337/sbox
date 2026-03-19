#include <screen.h>
#include <action.h>
#include <bitmap.h>

RadioMenu::RadioMenu(DummyAction **items, unsigned item_count)
    : Menu((Action**)items, item_count) {}

void RadioMenu::process_navigation(
    unsigned long button_select_press_duration,
    bool button_up_clicked,
    bool button_down_clicked
) {
    Menu::process_navigation(button_select_press_duration, button_up_clicked, button_down_clicked);

    if(button_select_press_duration > 50) {
        radio_state = item_selected;
        redraw_request = true;
    }
}

void RadioMenu::draw(U8G2 &u8g2, int offset_y) {
    Menu::draw(u8g2, offset_y);

    if(item_sel_previous >= 0)
        u8g2.drawCircle(12, 4 + offset_y, 4);
    u8g2.drawCircle(12, 26 + offset_y, 4);
    if(item_sel_next < item_count)
        u8g2.drawCircle(12, 48 + offset_y, 4);

    if(radio_state >= item_sel_previous && radio_state <= item_sel_next) {
        u8g2.drawDisc(12, 4 + 22 * (radio_state - item_sel_previous) + offset_y, 2);
    }
}
