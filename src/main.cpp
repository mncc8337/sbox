#include <Arduino.h>
#include <Wire.h>
#include <esp_sleep.h>

#include <sensors.h>
#include <ui_layout.h>

#define SDA_PIN 8
#define SCL_PIN 9

#define BUTTON_UP_PIN 3
#define BUTTON_DOWN_PIN 1
#define BUTTON_SELECT_PIN 4

#define BATTERY_PIN 0

enum BroascastType {
    NOT_BROASCASTING,
    BLE_BEACON,
    BLE_SERVER,
    WIFI,
};

bool button_up_clicked = false;
bool button_select_clicked = false;
bool button_down_clicked = false;

bool broadcasting = false;
BroascastType broascast_type = NOT_BROASCASTING;

bool screen_off = false;
bool statusbar_redraw = false;
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
}
void open_prev_screen() {
    if(!screen_stack_ptr) return;
    current_screen = screen_stack[--screen_stack_ptr];
    current_screen->request_redraw();
}

void turn_off_screen() {
    if(screen_off) return;

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
    if(!screen_off) return;

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
#define PSU_VOLTAGE (psu_voltage_sum / VOLTAGE_HISTORY_SIZE)

void update_psu_voltage() {
    uint32_t raw = analogReadMilliVolts(BATTERY_PIN) * 2;

    static uint32_t history_ptr = 0;
    psu_voltage_sum += - psu_voltage_history[history_ptr] + raw;
    psu_voltage_history[history_ptr++] = raw;
    history_ptr %= VOLTAGE_HISTORY_SIZE;
}

void update_state() {
    program_tick++;
    if(program_tick >= (1 << 10)) {
        program_tick = 0;

        update_psu_voltage();

        statusbar_redraw = true;
    }
}

void clear_statusbar() {
    u8g2.setDrawColor(0);
    u8g2.drawBox(0, 0, 128 - 8, 5);
    u8g2.setDrawColor(1);
}

void draw_statusbar() {
    u8g2.setFont(u8g2_font_4x6_mf);

    u8g2.setCursor(0, 5); u8g2.printf("%4dmV", PSU_VOLTAGE);
}

void draw_frame() {
    if(!current_screen->is_overlay())
        u8g2.clearBuffer();

    current_screen->draw(u8g2, 6);
    draw_statusbar();

    u8g2.sendBuffer();
}

void setup() {
    pinMode(BUTTON_UP_PIN, INPUT);
    pinMode(BUTTON_DOWN_PIN, INPUT);
    pinMode(BUTTON_SELECT_PIN, INPUT);

    pinMode(BATTERY_PIN, INPUT);

    for(unsigned i = 0; i < VOLTAGE_HISTORY_SIZE; i++)
        update_psu_voltage();

    Wire.begin(SDA_PIN, SCL_PIN);

    u8g2.setColorIndex(1);
    u8g2.begin();
    u8g2.setBitmapMode(1);

    if(!init_sensors()) {
        u8g2.printf("failed to init some sensors");
        while(1);
    }

    u8g2.clearBuffer();
    current_screen = &main_menu;
    current_screen->request_redraw();
}

void loop() {
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
        } else if(button_select_press_time >= 400) {
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

    // actibity check
    if(button_down_action || button_up_action || button_select_press_time) {
        idle_ts = millis();
        turn_on_screen();
    } else if(millis() - idle_ts > 3 * 60000) {
        idle_ts = millis(); // prevent sleeping again after waking up
        turn_off_screen();
    }

    update_state();

    if(!screen_off) {
        current_screen->process_navigation(
            button_select_press_time,
            button_up_action,
            button_down_action
        );

        if(current_screen->redraw_request) {
            draw_frame();
        } else if(statusbar_redraw) {
            clear_statusbar();
            draw_statusbar();
            u8g2.sendBuffer();
            statusbar_redraw = false;
        }
    } else {
        if(button_select_press_time > 0) {
            turn_on_screen();
            current_screen->request_redraw();
        }
    }

    delay(10);
}
