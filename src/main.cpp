#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <preference_keys.h>

#include <esp_sleep.h>

#include <bitmap.h>
#include <sensors.h>
#include <ui_layout.h>
#include <connectivity.h>

// littlefs format
#define FORMAT_IF_FAILED true

#define SDA_PIN 8
#define SCL_PIN 9

// #define SCK_PIN 6
// #define MISO_PIN 2
// #define MOSI_PIN 7
// #define CS_PIN 10

#define BUTTON_UP_PIN 3
#define BUTTON_DOWN_PIN 1
#define BUTTON_SELECT_PIN 4

#define BATTERY_PIN 0

enum Feature {
    FEAT_BLE = 0b1,
    FEAT_LITTLEFS = 0b01,
};

Preferences preferences;

uint16_t feature_mask = 0;

bool is_session_running = false;

bool is_datalogger_enabled;

bool is_telemetry_enabled;
TelemetryType telemetry_type;

bool sleep_lock = false;
bool screen_off = false;
unsigned long screen_timeout;
uint8_t screen_brightness;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

Screen *current_screen;
Screen *screen_stack[8];
unsigned screen_stack_ptr = 0;

void draw_frame() {
    if(!current_screen->is_overlay())
        u8g2.clearBuffer();
    current_screen->draw(u8g2);
    u8g2.setContrast(screen_brightness);
    u8g2.sendBuffer();
}

void open_screen(Screen *screen, bool forced=false) {
    if(screen_stack_ptr >= 8 || screen_stack[screen_stack_ptr - 1] == current_screen)
        return;
    if(screen->is_blocked()) {
        open_notification("this menu is\nnot available\nright now");
        return;
    }
    screen_stack[screen_stack_ptr++] = current_screen;
    current_screen = screen;
    current_screen->setup();
    current_screen->request_redraw();

    sleep_lock = current_screen->prevent_sleep();

    if(forced) {
        draw_frame();
    }
}
void open_prev_screen() {
    if(!screen_stack_ptr) return;
    current_screen = screen_stack[--screen_stack_ptr];
    current_screen->request_redraw();
}

void turn_off_screen() {
    if(screen_off || sleep_lock) return;

    u8g2.setPowerSave(1);

    if(!is_session_running) {
        sleep_sensors();
        gpio_wakeup_enable((gpio_num_t)BUTTON_SELECT_PIN, GPIO_INTR_HIGH_LEVEL);
        esp_sleep_enable_gpio_wakeup();

        // wait for button release
        while(digitalRead(BUTTON_SELECT_PIN) != LOW) delay(10);

        Wire.end();
        pinMode(SDA_PIN, INPUT);
        pinMode(SCL_PIN, INPUT);

        esp_light_sleep_start();

        Wire.begin(SDA_PIN, SCL_PIN);

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
    // that mean beacon mode is not running

    setCpuFrequencyMhz(160);
    unset_low_power_sensor_mode();
    u8g2.setPowerSave(0);

    screen_off = false;
}

void shutdown() {
    sleep_sensors();
    u8g2.setPowerSave(1);
    Wire.end();
    pinMode(SDA_PIN, INPUT);
    pinMode(SCL_PIN, INPUT);

    // TODO:
    // do some hardware tricks to actually cut the power off all peripherals

    // config deep sleep wake pin
    pinMode(BUTTON_SELECT_PIN, INPUT_PULLUP);
    esp_deep_sleep_enable_gpio_wakeup(1ULL << BUTTON_SELECT_PIN, ESP_GPIO_WAKEUP_GPIO_HIGH);
    esp_deep_sleep_start();
}

const unsigned VOLTAGE_HISTORY_SIZE = 16;
uint32_t psu_voltage_history[VOLTAGE_HISTORY_SIZE] = {0};
uint32_t psu_voltage_sum = 0;
uint16_t psu_voltage_avg = 0;
uint8_t battery_percentage = 0;

void update_battery_readings() {
    uint32_t raw = analogReadMilliVolts(BATTERY_PIN) * 2;
    static uint32_t history_ptr = 0;

    psu_voltage_sum = psu_voltage_sum - psu_voltage_history[history_ptr] + raw;
    psu_voltage_history[history_ptr++] = raw;
    history_ptr %= VOLTAGE_HISTORY_SIZE;

    psu_voltage_avg = psu_voltage_sum / VOLTAGE_HISTORY_SIZE;

    if(psu_voltage_avg >= 4150) {
        battery_percentage = 100;
    } else if(psu_voltage_avg > 3750) {
        battery_percentage = 80 + (uint8_t)((psu_voltage_avg - 3750) / 20);
    } else if(psu_voltage_avg > 3650) {
        battery_percentage = 20 + (uint8_t)(((psu_voltage_avg - 3650) * 3) / 5);
    } else if(psu_voltage_avg > 3500) {
        battery_percentage = 10 + (uint8_t)((psu_voltage_avg - 3500) / 15);
    } else if(psu_voltage_avg > 3200) {
        battery_percentage = (uint8_t)((psu_voltage_avg - 3200) / 30);
    } else {
        battery_percentage = 0;
    }
}

void setup() {
    pinMode(BUTTON_UP_PIN, INPUT);
    pinMode(BUTTON_DOWN_PIN, INPUT);
    pinMode(BUTTON_SELECT_PIN, INPUT);

    setenv("TZ", TZ_INFO, 1);
    tzset();

    pinMode(BATTERY_PIN, INPUT);

    for(unsigned i = 0; i < VOLTAGE_HISTORY_SIZE; i++)
        update_battery_readings();

    Wire.begin(SDA_PIN, SCL_PIN);

    // check if the screen exists or not
    // because no matter how the screen is
    // u8g2.begin() always return true
    Wire.beginTransmission(0x3c);
    if(Wire.endTransmission() != 0) {
        delay(1000);
        puts("screen is failing, please check connection");
        while(1) delay(1);
    }

    u8g2.begin();

    if(init_sensors() != (1 << SENS_COUNT) - 1) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_4x6_tf);
        u8g2.setCursor(0, 5); u8g2.printf("failed to init some sensors");
        u8g2.setCursor(0, 5 * 2 + 2 * 1); u8g2.printf("sensor mask: 0x%X", sensor_mask);
        u8g2.setCursor(0, 5 * 3 + 2 * 2); u8g2.printf("press SELECT to continue");
        u8g2.sendBuffer();

        while(digitalRead(BUTTON_SELECT_PIN) == LOW)
            delay(10);
    }

    if(!LittleFS.begin(FORMAT_IF_FAILED)) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_4x6_tf);
        u8g2.setCursor(0, 5); u8g2.printf("failed to mount LittleFS");
        u8g2.setCursor(0, 5 * 2 + 2 * 1); u8g2.printf("press SELECT to continue");
        u8g2.sendBuffer();

        while(digitalRead(BUTTON_SELECT_PIN) == LOW)
            delay(10);
    } else {
        feature_mask |= FEAT_LITTLEFS;
    }

    if(!ble_init()) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_4x6_tf);
        u8g2.setCursor(0, 5); u8g2.printf("failed to init BLE");
        u8g2.setCursor(0, 5 * 2 + 2 * 1); u8g2.printf("BLE features may not be usable");
        u8g2.setCursor(0, 5 * 3 + 2 * 2); u8g2.printf("press SELECT to continue");
        u8g2.sendBuffer();

        while(digitalRead(BUTTON_SELECT_PIN) == LOW)
            delay(10);
    } else {
        feature_mask |= FEAT_BLE;
    }

    // load saved settings
    preferences.begin("settings", false);
    screen_timeout = preferences.getULong(KEY_SCREEN_TIMEOUT, 3 * 60000);
    screen_brightness = preferences.getUChar(KEY_SCREEN_BRIGHTNESS, 128);
    is_datalogger_enabled = preferences.getBool(KEY_DATALOGGER_ENABLE, false);
    is_telemetry_enabled = preferences.getBool(KEY_BROADCAST_ENABLE, false);
    telemetry_type = (TelemetryType)preferences.getUChar(KEY_TELEMETRY_TYPE, BLE_SERVER);

    ui_init();

    // load splash gif for fun
    u8g2.setBitmapMode(0);
    for(unsigned i = 0; i < BITMAP_SPLASH_LEN * 5; i++) {
        if(digitalRead(BUTTON_SELECT_PIN) == HIGH) break;
        u8g2.drawXBMP(0, 0, 128, 64, BITMAP_SPLASH[i % BITMAP_SPLASH_LEN]);
        u8g2.sendBuffer();
        delay(10);
    }
    u8g2.setBitmapMode(1);

    current_screen = (Screen*)&main_menu;
    current_screen->request_redraw();
}

void loop() {
    static long unsigned last_battery_update_ts = 0;
    static long unsigned last_sensor_update_ts = 0;

    const long unsigned current_ts = millis();

    if(current_ts - last_battery_update_ts > 3000) {
        update_battery_readings();
        if(psu_voltage_avg < 3400)
            shutdown();
        last_battery_update_ts = current_ts;
    }

    if(current_ts - last_sensor_update_ts > 10 && is_session_running) {
        sensors_data_t sensors_data;
        get_sensors_data(sensors_data);
        
        switch(telemetry_type) {
            case BLE_BEACON: {
                ble_beacon_set_data(sensors_data);
                break;
            }
            case BLE_SERVER: {
                ble_server_update(sensors_data, battery_percentage);
                break;
            }
            default:;
        }
    }

    static bool button_up_clicked = false;
    static bool button_select_clicked = false;
    static bool button_down_clicked = false;

    int button_select_state = digitalRead(BUTTON_SELECT_PIN);
    int button_up_state = digitalRead(BUTTON_UP_PIN);
    int button_down_state = digitalRead(BUTTON_DOWN_PIN);

    static unsigned long first_select_press_ts = 0;

    unsigned long button_select_press_time = 0;
    bool button_up_action = false;
    bool button_down_action = false;

    if(button_select_state == HIGH && !button_select_clicked) {
        button_select_clicked = true;
        first_select_press_ts = current_ts;
    }
    if(button_select_state == LOW && button_select_clicked) { 
        button_select_press_time = current_ts - first_select_press_ts;
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
            draw_frame();
        }
    } else {
        if(button_select_press_time > 0) {
            turn_on_screen();
            current_screen->request_redraw();
        }
    }

    // activity check
    static long unsigned idle_ts = 0;
    if(button_down_action || button_up_action || button_select_press_time) {
        idle_ts = current_ts;
        turn_on_screen();
    } else if(current_ts - idle_ts > screen_timeout) {
        idle_ts = current_ts; // prevent sleeping again after waking up
        turn_off_screen();
    }

    delay(1);
}
