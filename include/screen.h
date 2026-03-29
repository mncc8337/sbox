#ifndef SCREEN_H
#define SCREEN_H

#include <U8g2lib.h>
#include <string>
#include <vector>
#include <Adafruit_Sensor.h>

class Action;
class DummyAction;
class Adafruit_Sensor;

typedef uint16_t CheckBoxMask;

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
    virtual bool is_blocked();
    virtual bool is_overlay();
    virtual void open_callback();
    virtual void close_callback();
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
    int sensor_id;

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

    sensors_event_t sensor_event = {0};

    unsigned long sample_interval_ms = 40;
    unsigned long last_sampling_ts = 0;
public:
    SensorView(int sensor_id, sensor_t &sensor_info);
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
    bool *block_flag;

public:
    Menu(std::vector<Action*> &items, bool *block_flag=nullptr);
    void process_navigation(
        unsigned long button_select_press_duration,
        bool button_up_clicked,
        bool button_down_clicked
    ) override;
    void draw(U8G2 &u8g2) override;
    bool is_blocked() override;
};

class RadioMenu: public Menu {
private:
    int radio_state = 0;
    int &bound_target;
    std::vector<int> &value_map;
    void (*callback)(int new_val);

public:
    RadioMenu(
        std::vector<DummyAction*> &items,
        int &bound_target,
        std::vector<int> &value_map,
        void (*callback)(int new_val),
        bool *block_flag=nullptr
    );
    void process_navigation(
        unsigned long button_select_press_duration,
        bool button_up_clicked,
        bool button_down_clicked
    ) override;
    void draw(U8G2 &u8g2) override;
    void open_callback() override;
    void close_callback() override;
};

class CheckBoxMenu: public Menu {
private:
    CheckBoxMask item_mask_buffer;
    CheckBoxMask &item_mask;
    std::vector<unsigned> &bit_map;
    void (*callback)(CheckBoxMask new_val);

public:
    CheckBoxMenu(
        std::vector<DummyAction*> &items,
        CheckBoxMask &item_mask,
        std::vector<unsigned> &bit_map,
        void (*callback)(CheckBoxMask new_val),
        bool *block_flag=nullptr
    );
    void process_navigation(
        unsigned long button_select_press_duration,
        bool button_up_clicked,
        bool button_down_clicked
    ) override;
    void draw(U8G2 &u8g2) override;
    void open_callback() override;
    void close_callback() override;
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
