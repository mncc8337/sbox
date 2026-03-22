#include "esp_system.h"
#include <ui_layout.h>
#include <U8g2lib.h>
#include <screen.h>
#include <action.h>
#include <connectivity.h>
#include <sensors.h>
#include <Adafruit_Sensor.h>

extern bool broadcasting;
extern U8G2 u8g2;

// connectivity menu
DummyAction ble_server_option("BLE Server");
DummyAction ble_beacon_option("BLE Beacon");
DummyAction wifi_option("WiFi");
std::vector<DummyAction*> connectivity_menu_items = {
    &ble_server_option,
    &ble_beacon_option,
    &wifi_option
};
std::vector<int> connectivity_menu_item_map = {
    BLE_SERVER,
    BLE_BEACON,
    WIFI,
};
extern BroadcastType new_broadcast_type;
RadioMenu connectivity_menu(
    connectivity_menu_items,
    (int&)new_broadcast_type,
    connectivity_menu_item_map,
    0
);
OpenScreenAction open_connectivity_menu("Connectivity", &connectivity_menu);

void broadcast_func() {
    extern FunctionAction broadcast;
    extern BroadcastType broadcast_type;

    if(broadcasting) {
        switch(broadcast_type) {
            case BLE_BEACON: {
                ble_beacon_stop();
                break;
            }
            case BLE_SERVER: {
                ble_server_stop();
                break;
            }
            default: return; // not gonna happened
        };

        broadcast.set_name("Broadcast");
        open_notification("Stopped\nbroadcasting");
        broadcasting = false;
        broadcast_type = new_broadcast_type;
    } else {
        broadcast_type = new_broadcast_type;
        switch(broadcast_type) {
            case BLE_BEACON: {
                ble_beacon_start();
                break;
            }
            case BLE_SERVER: {
                ble_server_start();
                break;
            }
            default: {
                open_notification("Unsupported\nconnectivity");
                return;
            }
        }
        broadcast.set_name("Broadcasting");
        open_notification("Started\nbroadcasting");
        broadcasting = true;
    }
}
FunctionAction broadcast("Broadcast", broadcast_func);

DummyAction open_sensors_menu("Sensors");

extern unsigned long screen_brightness;
DummyAction screen_brightness_menu_item_1("1");
DummyAction screen_brightness_menu_item_10("10");
DummyAction screen_brightness_menu_item_25("25");
DummyAction screen_brightness_menu_item_50("50");
DummyAction screen_brightness_menu_item_75("75");
DummyAction screen_brightness_menu_item_100("100");
std::vector<DummyAction*> screen_brightness_menu_items = {
    &screen_brightness_menu_item_1,
    &screen_brightness_menu_item_10,
    &screen_brightness_menu_item_25,
    &screen_brightness_menu_item_50,
    &screen_brightness_menu_item_75,
    &screen_brightness_menu_item_100,
};
std::vector<int> screen_brightness_menu_item_map = {1, 8, 26, 64, 128, 255};
RadioMenu screen_brightness_menu(
    screen_brightness_menu_items,
    (int&)screen_brightness,
    screen_brightness_menu_item_map,
    4
);
OpenScreenAction open_screen_brightness_menu("Brightness", &screen_brightness_menu);

extern unsigned long screen_timeout;
DummyAction screen_timeout_menu_item_30s("30s");
DummyAction screen_timeout_menu_item_3m("3m");
DummyAction screen_timeout_menu_item_5m("5m");
DummyAction screen_timeout_menu_item_10m("10m");
std::vector<DummyAction*> screen_timeout_menu_items = {
    &screen_timeout_menu_item_30s,
    &screen_timeout_menu_item_3m,
    &screen_timeout_menu_item_5m,
    &screen_timeout_menu_item_10m,
};
std::vector<int> screen_timeout_menu_item_map = {
    30000,
    60 * 3 * 60000,
    60 * 5 * 60000,
    60 * 10 * 60000,
};
RadioMenu screen_timeout_menu(
    screen_timeout_menu_items,
    (int&)screen_timeout,
    screen_timeout_menu_item_map,
    1
);
OpenScreenAction open_screen_timeout_menu("Screen Timeout", &screen_timeout_menu);

void invert_screen_color() {
    static bool screen_inverted = false;
    if(!screen_inverted)
        u8g2.sendF("c", 0xA7);
    else
        u8g2.sendF("c", 0xA6);

    screen_inverted = !screen_inverted;
}
FunctionAction screen_invert("Invert Color", invert_screen_color);

std::vector<Action*> screen_menu_items = {
    &open_screen_brightness_menu,
    &open_screen_timeout_menu,
    &screen_invert,
};
Menu screen_menu(screen_menu_items, 0);
OpenScreenAction open_screen_menu("Screen", &screen_menu);

InfoScreen info_screen_instance;
OpenScreenAction open_info_menu("Info", &info_screen_instance);

FunctionAction reboot("Reboot", esp_restart);

std::vector<Action*> settings_menu_items = {
    &open_connectivity_menu,
    &open_sensors_menu,
    &open_screen_menu,
    &open_info_menu,
    &reboot,
};
Menu settings_menu(settings_menu_items, 0);
OpenScreenAction open_settings_menu("Settings", &settings_menu);

std::vector<Action*> sensor_data_menu_items;
Menu sensor_data_menu(sensor_data_menu_items, 0);
OpenScreenAction open_sensor_data_menu("Sensor Data", &sensor_data_menu);

SplashScreen splash_screen;
OpenScreenAction open_splash_screen("Splash GIF", &splash_screen);

std::vector<Action*> main_menu_items = {
    &open_sensor_data_menu,
    &open_settings_menu,
    &broadcast,
    &open_splash_screen
};
Menu main_menu(main_menu_items, 0);

void ui_init() {
    sensor_data_menu_items.reserve(SENS_COUNT);

    extern sensors_data_t sensors_data;
    for(unsigned i = 0; i < SENS_COUNT; i++) {
        if(!SENSOR_ALIVE(i)) continue;

        sensor_t sensor_info;
        sensors[i]->getSensor(&sensor_info);

        SensorView *sensor_view = new SensorView(*sensors[i]);
        OpenScreenAction *open_sensor_view = new OpenScreenAction(
            std::string(sensor_info.name),
            sensor_view
        );

        sensor_data_menu_items.push_back(open_sensor_view);
    }
}
