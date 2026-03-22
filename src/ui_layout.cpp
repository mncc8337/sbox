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
    connectivity_menu_item_map
);
OpenScreenAction open_connectivity_menu("Connectivity", &connectivity_menu);

void functionaction_broadcast() {
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
FunctionAction broadcast("Broadcast", functionaction_broadcast);

DummyAction open_screen_menu("Screen");

InfoScreen info_screen_instance;
OpenScreenAction open_info_menu("Info", &info_screen_instance);

// system menu
std::vector<Action*> settings_menu_items = {
    &open_connectivity_menu,
    &open_screen_menu,
    &open_info_menu,
};
Menu settings_menu(settings_menu_items);
OpenScreenAction open_settings_menu("Settings", &settings_menu);

// sensor menu
std::vector<Action*> sensor_data_menu_items;
Menu sensor_data_menu(sensor_data_menu_items);
OpenScreenAction open_sensor_data_menu("Sensor Data", &sensor_data_menu);

// main menu
std::vector<Action*> main_menu_items = {
    &open_sensor_data_menu,
    &open_settings_menu,
    &broadcast,
};
Menu main_menu(main_menu_items);

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
