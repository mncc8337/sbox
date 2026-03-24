#include <screen.h>
#include <action.h>
#include <bitmap.h>

RadioMenu::RadioMenu(
    std::vector<DummyAction*> &items,
    int &bound_target,
    std::vector<int> &value_map,
    void (*callback)(int new_value)
)
    : Menu((std::vector<Action*> &)items),
      bound_target(bound_target),
      value_map(value_map),
      callback(callback) {
    item_selected = -1;
}

void RadioMenu::process_navigation(
    unsigned long button_select_press_duration,
    bool button_up_clicked,
    bool button_down_clicked
) {
    Menu::process_navigation(button_select_press_duration, button_up_clicked, button_down_clicked);

    if(button_select_press_duration > 50) {
        radio_state = item_selected;
        bound_target = value_map[radio_state];
        redraw_request = true;

        if(callback) callback(value_map[radio_state]);
    }
}

void RadioMenu::setup() {
    if(item_selected >= 0) return;

    for(radio_state = 0; radio_state < items.size(); radio_state++) {
        if(value_map[radio_state] == bound_target) break;
    }

    if(radio_state == items.size()) {
        radio_state = 0;
    }

    item_selected = radio_state;
    item_sel_previous = item_selected - 1;
    item_sel_next = item_selected + 1;
}

void RadioMenu::draw(U8G2 &u8g2) {
    Menu::draw(u8g2);

    if(item_sel_previous >= 0)
        u8g2.drawCircle(12, 10, 4);
    u8g2.drawCircle(12, 32, 4);
    if(item_sel_next < items.size())
        u8g2.drawCircle(12, 54, 4);

    if(radio_state >= item_sel_previous && radio_state <= item_sel_next) {
        u8g2.drawDisc(12, 10 + 22 * (radio_state - item_sel_previous), 2);
    }
}
