#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <preference_keys.h>

#include <esp_sleep.h>
#include <esp_log.h>

#include <bitmap.h>
#include <sensors.h>
#include <ui_layout.h>
#include <connectivity.h>
#include <data_logger.h>

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

volatile bool is_session_running = false;

volatile bool is_datalogger_enabled;
volatile unsigned long datalogger_interval;

volatile bool is_telemetry_enabled;
volatile TelemetryType telemetry_type;

File logfile;
bool force_file_flush = false;

extern bool is_webserver_running;
extern void loop_webserver();

bool screen_off = false;
unsigned long screen_timeout;
uint8_t screen_brightness;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

const unsigned SCREEN_STACK_SIZE = 8;
Screen *current_screen;
Screen *screen_stack[SCREEN_STACK_SIZE];
unsigned screen_stack_ptr = 0;

void draw_frame(Screen *screen) {
    u8g2.clearBuffer();
    if(screen->is_overlay() && screen_stack_ptr > 0) {
        Screen *prev_screen = screen_stack[screen_stack_ptr - 1];
        prev_screen->draw(u8g2);
    }
    screen->draw(u8g2);
    u8g2.sendBuffer();
}

void open_screen(Screen *screen, bool forced=false) {
    if(screen_stack_ptr >= SCREEN_STACK_SIZE || current_screen == screen) {
        screen->request_redraw();
        return;
    }
    if(screen->is_blocked()) {
        open_notification("This menu is\nnot available\nright now");
        return;
    }

    current_screen->close_callback();
    screen_stack[screen_stack_ptr++] = current_screen;

    current_screen = screen;
    current_screen->open_callback();
    current_screen->request_redraw();

    if(forced)
        draw_frame(current_screen);

    ESP_LOGD("UI", "Screen 0x%X opened", current_screen);
}
void open_prev_screen() {
    if(screen_stack_ptr == 0) return;

    current_screen->close_callback();
    current_screen = screen_stack[--screen_stack_ptr];

    current_screen->open_callback();
    current_screen->request_redraw();
    ESP_LOGD("UI", "Screen 0x%X opened", current_screen);
}

void shutdown() {
    ESP_LOGI("SYSTEM", "Shutting down");

    u8g2.setPowerSave(1);

    Wire.end();

    pinMode(SDA_PIN, INPUT);
    pinMode(SCL_PIN, INPUT);

    // config deep sleep wake pin
    pinMode(BUTTON_SELECT_PIN, INPUT_PULLDOWN);
    esp_deep_sleep_enable_gpio_wakeup(1ULL << BUTTON_SELECT_PIN, ESP_GPIO_WAKEUP_GPIO_HIGH);
    esp_deep_sleep_start();
}

void turn_off_screen() {
    if(screen_off || current_screen->prevent_sleep()) return;

    u8g2.setPowerSave(1);

    if(!is_session_running && !is_webserver_running) {
        // wait for button release
        while(digitalRead(BUTTON_SELECT_PIN) == HIGH) delay(10);

        if(sensors_task_handle)
            vTaskSuspend(sensors_task_handle);

        sleep_sensors();

        gpio_wakeup_enable((gpio_num_t)BUTTON_SELECT_PIN, GPIO_INTR_HIGH_LEVEL);
        esp_sleep_enable_gpio_wakeup();

        ESP_LOGI("SYSTEM", "Starting light sleep");
        esp_light_sleep_start();

        // the esp wakes up right there
        // also wait for button release
        while(digitalRead(BUTTON_SELECT_PIN) == HIGH) delay(10);
        ESP_LOGI("SYSTEM", "Waken up from light sleep");

        wake_sensors();

        if(sensors_task_handle)
            vTaskResume(sensors_task_handle);

        u8g2.setPowerSave(0);

        return; // no need to set flag
    }

    set_low_power_sensor_mode();
    setCpuFrequencyMhz(80);
    ESP_LOGI("SYSTEM", "Entered low power mode");

    screen_off = true;
}

void turn_on_screen() {
    if(!screen_off || current_screen->prevent_sleep()) return;

    // if this func get called
    // that mean beacon mode is not running

    setCpuFrequencyMhz(160);
    unset_low_power_sensor_mode();
    u8g2.setPowerSave(0);

    ESP_LOGI("SYSTEM", "Screen turned on");

    screen_off = false;
}

const unsigned VOLTAGE_HISTORY_SIZE = 16;
uint32_t psu_voltage_history[VOLTAGE_HISTORY_SIZE] = {0};
uint32_t psu_voltage_sum = 0;
uint16_t psu_voltage_avg = 0;
uint8_t battery_percentage = 0;

void battery_readings_init() {
    uint32_t raw = analogReadMilliVolts(BATTERY_PIN) * 2;
    for(unsigned i = 0; i < VOLTAGE_HISTORY_SIZE; i++) {
        psu_voltage_history[i] = raw;
    }
    psu_voltage_avg = raw;
    psu_voltage_sum = raw * VOLTAGE_HISTORY_SIZE;
}

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
        shutdown();
    }
}

void setup() {
    pinMode(BUTTON_UP_PIN, INPUT);
    pinMode(BUTTON_DOWN_PIN, INPUT);
    pinMode(BUTTON_SELECT_PIN, INPUT);

    pinMode(BATTERY_PIN, INPUT);

    setenv("TZ", TZ_INFO, 1);
    tzset();

    battery_readings_init();

    // load saved settings
    Preferences pref;
    pref.begin(PREFERENCES, true);
    screen_timeout = pref.getULong(KEY_SCREEN_TIMEOUT, 3 * 60000);
    screen_brightness = pref.getUChar(KEY_SCREEN_BRIGHTNESS, 128);
    is_datalogger_enabled = pref.getBool(KEY_DATALOGGER_ENABLE, false);
    datalogger_interval = pref.getULong(KEY_DATALOGGER_INTERVAL, 3 * 60 * 1000);
    is_telemetry_enabled = pref.getBool(KEY_TELEMETRY_ENABLE, false);
    telemetry_type = (TelemetryType)pref.getUChar(KEY_TELEMETRY_TYPE, BLE_SERVER);
    pref.end();
    ESP_LOGI("SYSTEM", "Loaded saved settings");

    Wire.begin(SDA_PIN, SCL_PIN);

    // check if the screen exists or not
    // because no matter how the screen is
    // u8g2.begin() always return true
    Wire.beginTransmission(0x3c);
    if(Wire.endTransmission() != 0) {
        delay(1000);
        ESP_LOGE("SYSTEM", "screen is failing, please check connection");
        while(1) delay(1000);
    }

    u8g2.begin();
    u8g2.setContrast(screen_brightness);

    if(sensors_init() != (1 << SENS_COUNT) - 1) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_4x6_tf);
        u8g2.setCursor(0, 5); u8g2.printf("failed to init some sensors");
        u8g2.setCursor(0, 5 * 2 + 2 * 1); u8g2.printf("sensor mask: 0x%X", sensor_mask);
        u8g2.setCursor(0, 5 * 3 + 2 * 2); u8g2.printf("press SELECT to continue");
        u8g2.sendBuffer();

        ESP_LOGE("SYSTEM", "Failed to initialize some sensor, mask : 0x%X", sensor_mask);

        while(digitalRead(BUTTON_SELECT_PIN) == LOW)
            delay(10);
    }

    if(!LittleFS.begin(FORMAT_IF_FAILED)) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_4x6_tf);
        u8g2.setCursor(0, 5); u8g2.printf("failed to mount LittleFS");
        u8g2.setCursor(0, 5 * 2 + 2 * 1); u8g2.printf("press SELECT to continue");
        u8g2.sendBuffer();

        ESP_LOGE("SYSTEM", "Failed to mount LittleFS");

        while(digitalRead(BUTTON_SELECT_PIN) == LOW)
            delay(10);
    } else {
        ESP_LOGI("SYSTEM", "LittleFS mounted");
    }

    ui_init();
    ESP_LOGI("SYSTEM", "UI initialized");

    // now start all system tasks
    xTaskCreate(sensors_task, "SensorPollingTask", 4096, NULL, 1, &sensors_task_handle);

    // load splash gif for fun
    u8g2.setBitmapMode(0);
    for(unsigned i = 0; i < BITMAP_SPLASH_LEN * 5; i++) {
        if(digitalRead(BUTTON_SELECT_PIN) == HIGH) break;
        u8g2.drawXBMP(0, 0, 128, 64, BITMAP_SPLASH[i % BITMAP_SPLASH_LEN]);
        u8g2.sendBuffer();
        delay(10);
    }
    u8g2.setBitmapMode(1);

    while(digitalRead(BUTTON_SELECT_PIN) == HIGH) delay(10);

    current_screen = (Screen*)&main_menu;
    current_screen->request_redraw();
}

void loop() {
    static long unsigned last_battery_update_ts = 0;
    static long unsigned last_telemetry_packet_ts = 0;
    static long unsigned last_data_log_ts = 0;
    static long unsigned last_flush_ts = 0;
    static bool unflushed_data = false;

    const long unsigned current_ts = millis();

    if(current_ts - last_battery_update_ts > 3000) {
        update_battery_readings();
        if(psu_voltage_avg < 3400) {
            ESP_LOGW("SYSTEM", "Shutting down due to low battery voltage");
            shutdown();
        }
        last_battery_update_ts = current_ts;
    }

    sensors_data_t sensors_data;
    bool sensors_data_retrieved = false;

    if(current_ts - last_telemetry_packet_ts > 100 && is_session_running && is_telemetry_enabled) {
        if(is_session_data_ready(sensors_data)) {
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

            sensors_data_retrieved = true;
            last_telemetry_packet_ts = current_ts;
            ESP_LOGD("TELEMETRY", "New telemetry packet sent");
        }
    }

    if(
        (
            current_ts - last_data_log_ts > datalogger_interval
            || force_file_flush
        )
        && is_session_running
        && is_datalogger_enabled
        && (
            sensors_data_retrieved
            || is_session_data_ready(sensors_data)
        )
    ) {
        write_log_packet(logfile, sensors_data);

        unflushed_data = true;
        sensors_data_retrieved = true;
        last_data_log_ts = current_ts;
        ESP_LOGD("DATALOGGER", "Data read");
    }

    if(
        unflushed_data
        && is_session_running
        && is_datalogger_enabled
        && (
            force_file_flush
            || datalogger_interval > 5 * 60 * 1000
            || current_ts - last_flush_ts >= 5 * 60 * 1000
        )
    ) {
        unflushed_data = false;
        last_flush_ts = current_ts;
        force_file_flush = false;
        logfile.flush();
        ESP_LOGD("DATALOGGER", "Data flushed");
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
            ESP_LOGW("SYSTEM", "Shutting down due to button presses");
            shutdown();
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
            draw_frame(current_screen);
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
    } else if(current_ts - idle_ts > screen_timeout && !current_screen->prevent_sleep()) {
        ESP_LOGW("SYSTEM", "Turning off due to no activity");
        idle_ts = current_ts; // prevent sleeping again after waking up
        turn_off_screen();
    }

    if(is_webserver_running)
        loop_webserver();

    delay(1);
}
