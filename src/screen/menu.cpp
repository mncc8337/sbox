#include <screen.h>
#include <action.h>
#include <bitmap.h>

Menu::Menu(std::vector<Action*> &items)
    : items((std::vector<Action*>&)items) {}

void Menu::process_navigation(
    unsigned long button_select_press_duration,
    bool button_up_clicked,
    bool button_down_clicked
) {
    if(button_up_clicked) {
        item_selected = item_selected - 1;
        if(item_selected < 0) {
            item_selected = items.size() - 1;
        }
    }

    if(button_down_clicked) {
        item_selected = item_selected + 1;
        if(item_selected >= items.size()) {
            item_selected = 0;
        }
    } 

    item_sel_previous = item_selected - 1;
    item_sel_next = item_selected + 1;  

    if(button_select_press_duration > 50) {
        items[item_selected]->execute();
    }

    if(button_up_clicked || button_down_clicked)
        redraw_request = true;
}

void Menu::draw(U8G2 &u8g2) {
    // selected item background
    u8g2.drawXBMP(0, 22, 128, 21, BITMAP_ITEM_SEL_OUTLINE);

    if(item_sel_previous >= 0) {
        // draw previous item as icon + label
        u8g2.setFont(u8g2_font_profont12_tf);
        u8g2.drawStr(25, 15, items[item_sel_previous]->name.c_str());
        // u8g2.drawXBMP(4, 2 + offset_y, 16, 16, bitmap_icons[item_sel_previous]);
    }

    // draw selected item as icon + label in bold font
    u8g2.setFont(u8g2_font_tenthinnerguys_tf); // u8g2_font_DigitalDisco_tu
    u8g2.drawStr(25, 15+20+2, items[item_selected]->name.c_str());
    // u8g2.drawXBMP(4, 24 + offset_y, 16, 16, bitmap_icons[item_selected]);

    if(item_sel_next < items.size()) {
        // draw next item as icon + label
        u8g2.setFont(u8g2_font_profont12_tf);
        u8g2.drawStr(25, 15+20+20+2+2, items[item_sel_next]->name.c_str());
        // u8g2.drawXBMP(4, 46 + offset_y, 16, 16, bitmap_icons[item_sel_next]);
    }

    // draw scrollbar background
    u8g2.drawXBMP(128-8, 0, 8, 64, BITMAP_SCROLLBAR_BACKGROUND);

    // draw scrollbar handle
    u8g2.drawBox(
        125,
        64/items.size() * item_selected,
        3,
        64/items.size()
    );

    redraw_request = false;
}
