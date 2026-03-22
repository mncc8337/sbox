#ifndef SCREEN_H
#define SCREEN_H

#include <U8g2lib.h>
#include <string>
#include <vector>

class Action;
class DummyAction;
class Adafruit_Sensor;

extern void open_notification(std::string message);

class Screen {
public:
    bool redraw_request = false;

    virtual void process_navigation(
        unsigned long button_select_press_duration,
        bool button_up_clicked,
        bool button_down_clicked
    ) = 0;
    virtual void draw(U8G2 &u8g2) = 0;
    virtual bool is_overlay();
    virtual bool prevent_sleep();
    void request_redraw();
};

class SplashScreen: public Screen {
public:
    void process_navigation(
        unsigned long button_select_press_duration,
        bool button_up_clicked,
        bool button_down_clicked
    );
    void draw(U8G2 &u8g2);
};

class Notification: public Screen {
private:
    std::string message;
public:
    Notification(std::string message);
    void process_navigation(
        unsigned long button_select_press_duration,
        bool button_up_clicked,
        bool button_down_clicked
    ) override;
    void draw(U8G2 &u8g2) override;
    bool is_overlay() override;

    void set_message(std::string new_message);
};

class SensorView: public Screen {
private:
    Adafruit_Sensor &sensor;

    char sensor_name[32];
    int32_t sensor_type;
    float max_sensor_value;
    float min_sensor_value;
    bool multi_axis;

    bool graph_screen = false;
    float graph_data[127];
    unsigned graph_data_pos = 0;
    unsigned axis = 0;
    bool auto_scaling = false;

    unsigned long sample_interval_ms = 50;
    unsigned long last_sampling_ts = 0;
public:
    SensorView(Adafruit_Sensor &sensor);
    void process_navigation(
        unsigned long button_select_press_duration,
        bool button_up_clicked,
        bool button_down_clicked
    ) override;
    void draw(U8G2 &u8g2) override;

    bool prevent_sleep() override;
};

class Menu: public Screen {
protected:
    std::vector<Action*> &items;
    int item_selected = 0;
    int item_sel_previous = -1;
    int item_sel_next = 1;

public:
    Menu(std::vector<Action*> &items, unsigned default_select);
    void process_navigation(
        unsigned long button_select_press_duration,
        bool button_up_clicked,
        bool button_down_clicked
    ) override;
    void draw(U8G2 &u8g2) override;
};

class RadioMenu: public Menu {
private:
    int radio_state = 0;
    int &bound_target;
    std::vector<int> &value_map;

public:
    RadioMenu(
        std::vector<DummyAction*> &items,
        int &bound_target,
        std::vector<int> &value_map,
        unsigned default_state
    );
    void process_navigation(
        unsigned long button_select_press_duration,
        bool button_up_clicked,
        bool button_down_clicked
    ) override;
    void draw(U8G2 &u8g2) override;
};

class InfoScreen : public Screen {
private:
    unsigned long last_update_ts = 0;
    int current_y = 6;
public:
    void process_navigation(
        unsigned long button_select_press_duration,
        bool button_up_clicked,
        bool button_down_clicked
    ) override;
    void draw(U8G2 &u8g2) override;
};

#endif
