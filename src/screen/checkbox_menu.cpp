#include <screen.h>
#include <action.h>
#include <bitmap.h>

#define GET_MASK(mask, item) (((mask) >>(item)) & 1)

CheckBoxMenu::CheckBoxMenu(
    std::vector<DummyAction*> &items,
    CheckBoxMask &item_mask,
    std::vector<unsigned> &bit_map,
    void (*callback)(CheckBoxMask new_value),
    bool *block_flag
)
    : Menu((std::vector<Action*> &)items, block_flag),
      item_mask(item_mask),
      item_mask_buffer(item_mask),
      bit_map(bit_map),
      callback(callback) {
}

void CheckBoxMenu::process_navigation(
    unsigned long button_select_press_duration,
    bool button_up_clicked,
    bool button_down_clicked
) {
    Menu::process_navigation(
        button_select_press_duration,
        button_up_clicked,
        button_down_clicked
    );

    if(button_select_press_duration > 50) {
        item_mask_buffer ^= 1 << item_selected;
        redraw_request = true;
    }
}

void CheckBoxMenu::draw(U8G2 &u8g2) {
    Menu::draw(u8g2);

    for(unsigned i = 0; i < 3; i++) {
        int item = i + item_sel_previous;
        if(item < 0) continue;
        if(item >= items.size()) break;

        if(GET_MASK(item_mask, bit_map[item]) == 1) {
            u8g2.drawXBM(
                4,
                2 + 22 * i,
                16,
                16,
                BITMAP_CHECK
            );
        }
    }
}

void CheckBoxMenu::open_callback() {
    item_selected = 0;
    item_sel_previous = -1;
    item_sel_next = 1;

    for(unsigned i = 0; i < items.size(); i++) {
        items[i]->icon = BITMAP_CHECKBOX;
    }
}

void CheckBoxMenu::close_callback() {
    item_mask = item_mask_buffer;
    if(callback) callback(item_mask);
}
