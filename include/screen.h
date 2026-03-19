#ifndef SCREEN_H
#define SCREEN_H

#include <U8g2lib.h>

class Action;
class DummyAction;
class Adafruit_Sensor;

class Screen {
public:
    bool redraw_request = false;

    virtual void process_navigation(
        unsigned long button_select_press_duration,
        bool button_up_clicked,
        bool button_down_clicked
    ) = 0;
    virtual void draw(U8G2 &u8g2, int offset_y) = 0;
    virtual bool is_menu();
    void request_redraw();
};

class SensorView: public Screen {
private:
    Adafruit_Sensor **sensor_ptr;

    char sensor_name[32];
    int32_t sensor_type;
    bool initialized = false;
public:
    SensorView(Adafruit_Sensor **sensor_ptr);
    void process_navigation(
        unsigned long button_select_press_duration,
        bool button_up_clicked,
        bool button_down_clicked
    ) override;
    void draw(U8G2 &u8g2, int offset_y) override;
    bool is_menu() override;
};

class Menu: public Screen {
private:
    Action **items;

protected:
    unsigned item_count;
    int item_selected = 0;
    int item_sel_previous = -1;
    int item_sel_next = 1;

public:
    Menu(Action **items, unsigned item_count);
    void process_navigation(
        unsigned long button_select_press_duration,
        bool button_up_clicked,
        bool button_down_clicked
    ) override;
    void draw(U8G2 &u8g2, int offset_y) override;
    bool is_menu() override;
};

class RadioMenu: public Menu {
private:
    int radio_state = 0;

public:
    RadioMenu(DummyAction **items, unsigned item_count);
    void process_navigation(
        unsigned long button_select_press_duration,
        bool button_up_clicked,
        bool button_down_clicked
    ) override;
    void draw(U8G2 &u8g2, int offset_y) override;
    bool is_menu() override;
};

#endif
