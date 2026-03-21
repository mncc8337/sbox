#include "esp32-hal-gpio.h"
#include <Arduino.h>
#include <Wire.h>
#include <esp_sleep.h>

#include <sensors.h>
#include <ui_layout.h>
#include <connectivity.h>

#define SDA_PIN 8
#define SCL_PIN 9

#define BUTTON_UP_PIN 3
#define BUTTON_DOWN_PIN 1
#define BUTTON_SELECT_PIN 4

#define BATTERY_PIN 0

bool button_up_clicked = false;
bool button_select_clicked = false;
bool button_down_clicked = false;

bool broadcasting = false;
BroadcastType broadcast_type = BLE_SERVER;
BroadcastType new_broadcast_type = BLE_SERVER;

bool sleep_lock = false;

bool screen_off = false;
uint32_t program_tick = 0;
unsigned long idle_ts = 0;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

Screen *current_screen;
Screen *screen_stack[8];
unsigned screen_stack_ptr = 0;
void open_screen(Screen *screen) {
    if(screen_stack_ptr >= 8 || screen_stack[screen_stack_ptr - 1] == current_screen)
        return;
    screen_stack[screen_stack_ptr++] = current_screen;
    current_screen = screen;
    current_screen->request_redraw();

    sleep_lock = current_screen->prevent_sleep();
}
void open_prev_screen() {
    if(!screen_stack_ptr) return;
    current_screen = screen_stack[--screen_stack_ptr];
    current_screen->request_redraw();
}

void turn_off_screen() {
    if(screen_off || sleep_lock) return;

    u8g2.setPowerSave(1);

    if(!broadcasting) {
        sleep_sensors();
        gpio_wakeup_enable((gpio_num_t)BUTTON_SELECT_PIN, GPIO_INTR_HIGH_LEVEL);
        esp_sleep_enable_gpio_wakeup();

        // wait for button release
        while(digitalRead(BUTTON_SELECT_PIN) != LOW) delay(10);

        esp_light_sleep_start();

        // the esp wakes up right there
        // also wait for button release
        while(digitalRead(BUTTON_SELECT_PIN) != LOW) delay(10);

        wake_sensors();
        u8g2.setPowerSave(0);
        return; // no need to set flag
    } else {
        set_low_power_sensor_mode();
        setCpuFrequencyMhz(80);
    }

    screen_off = true;
}

void turn_on_screen() {
    if(!screen_off || sleep_lock) return;

    // if this func get called
    // that mean beacom mode is not running

    setCpuFrequencyMhz(160);
    unset_low_power_sensor_mode();
    u8g2.setPowerSave(0);

    screen_off = false;
}

const unsigned VOLTAGE_HISTORY_SIZE = 16;
uint32_t psu_voltage_history[VOLTAGE_HISTORY_SIZE];
uint32_t psu_voltage_sum = 0;
uint8_t battery_percentage = 0;
#define PSU_VOLTAGE (psu_voltage_sum / VOLTAGE_HISTORY_SIZE)

void update_battery_readings() {
    uint32_t raw = analogReadMilliVolts(BATTERY_PIN) * 2;

    static uint32_t history_ptr = 0;
    psu_voltage_sum += - psu_voltage_history[history_ptr] + raw;
    psu_voltage_history[history_ptr++] = raw;
    history_ptr %= VOLTAGE_HISTORY_SIZE;

    uint16_t mvolts = PSU_VOLTAGE;
    if(mvolts >= 4150) 
        battery_percentage = 100;
    else if(mvolts > 3800)
        battery_percentage = (uint8_t)(70 + (((uint32_t)(mvolts - 3800) * 19) >> 8));
    else if(mvolts > 3600)
        battery_percentage = (uint8_t)(20 + ((mvolts - 3600) >> 2));
    else if(mvolts > 3400)
        battery_percentage = (uint8_t)(((uint32_t)(mvolts - 3400) * 26) >> 8);
}

void update_state() {
    program_tick++;
    if(!(program_tick % (1 << 10))) {

        update_battery_readings();
        if(PSU_VOLTAGE < 3400)
            esp_deep_sleep_start();
    }
    if(!(program_tick % (1 << 4)) && broadcasting) {
        sensors_data_t data;
        get_sensors_data(data);
        
        switch(broadcast_type) {
            case BLE_BEACON: {
                ble_beacon_set_data(data);
                break;
            }
            case BLE_SERVER: {
                ble_server_update(data, battery_percentage);
                break;
            }
            default:;
        }
    }

    program_tick %= (1<<10);
}

void draw_frame() {
}

void setup() {
    pinMode(BUTTON_UP_PIN, INPUT);
    pinMode(BUTTON_DOWN_PIN, INPUT);
    pinMode(BUTTON_SELECT_PIN, INPUT);

    pinMode(BATTERY_PIN, INPUT);

    for(unsigned i = 0; i < VOLTAGE_HISTORY_SIZE; i++)
        update_battery_readings();

    Wire.begin(SDA_PIN, SCL_PIN);

    u8g2.setColorIndex(1);
    u8g2.begin();
    u8g2.setBitmapMode(1);
    u8g2.clear();

    if(init_sensors() != (1 << SENS_COUNT) - 1) {
        u8g2.setFont(u8g2_font_4x6_tf);
        u8g2.setCursor(0, 5); u8g2.printf("failed to init some sensors");
        u8g2.setCursor(0, 5 * 2 + 2 * 1); u8g2.printf("sensor mask: 0x%X", sensor_mask);
        u8g2.setCursor(0, 5 * 3 + 2 * 2); u8g2.printf("press SELECT to continue");
        u8g2.sendBuffer();

        while(digitalRead(BUTTON_SELECT_PIN) == LOW)
            delay(10);
    }

    while(digitalRead(BUTTON_SELECT_PIN) == HIGH)
        delay(10);

    ble_init();

    ui_init();

    current_screen = &main_menu;
    current_screen->request_redraw();
}

void loop() {
    update_state();

    int button_select_state = digitalRead(BUTTON_SELECT_PIN);
    int button_up_state = digitalRead(BUTTON_UP_PIN);
    int button_down_state = digitalRead(BUTTON_DOWN_PIN);

    static unsigned long first_select_press_ts = 0;

    unsigned long button_select_press_time = 0;
    bool button_up_action = false;
    bool button_down_action = false;

    if(button_select_state == HIGH && !button_select_clicked) {
        button_select_clicked = true;
        first_select_press_ts = millis();
    }
    if(button_select_state == LOW && button_select_clicked) { 
        button_select_press_time = millis() - first_select_press_ts;
        button_select_clicked = false;

        bool event_consumed = true;

        // global event
        if(button_select_press_time >= 5000) {
            turn_off_screen();
        } else if(button_select_press_time >= 300) {
            open_prev_screen();
        } else {
            event_consumed = false;
        }

        if(event_consumed) return;
    }

    if(button_up_state == HIGH && !button_up_clicked) {
        button_up_clicked = true;
    }
    if(button_up_state == LOW && button_up_clicked) {
        button_up_clicked = false;
        button_up_action = true;
    }

    if(button_down_state == HIGH && !button_down_clicked) {
        button_down_clicked = true;
    } 
    if(button_down_state == LOW && button_down_clicked) {
        button_down_clicked = false;
        button_down_action = true;
    }

    if(!screen_off) {
        current_screen->process_navigation(
            button_select_press_time,
            button_up_action,
            button_down_action
        );

        if(current_screen->redraw_request) {
            if(!current_screen->is_overlay())
                u8g2.clearBuffer();
            current_screen->draw(u8g2);
            u8g2.sendBuffer();
        }
    } else {
        if(button_select_press_time > 0) {
            turn_on_screen();
            current_screen->request_redraw();
        }
    }

    // activity check
    if(button_down_action || button_up_action || button_select_press_time) {
        idle_ts = millis();
        turn_on_screen();
    } else if(millis() - idle_ts > 3 * 60000) {
        idle_ts = millis(); // prevent sleeping again after waking up
        turn_off_screen();
    }

    delay(10);
}
