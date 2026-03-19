#include <ui_layout.h>
#include <screen.h>
#include <action.h>
#include <U8g2lib.h>
#include <sensors.h>
#include <ble.h>

extern bool broadcasting;
extern U8G2 u8g2;

// connectivity menu
DummyAction ble_beacon_option("BLE Beacon");
DummyAction ble_server_option("BLE Server");
DummyAction wifi_option("WiFi");
DummyAction *CONNECTIVITY_MENU_ITEMS[] = {
    &ble_beacon_option,
    &ble_server_option,
    &wifi_option
};
RadioMenu connectivity_menu(
    CONNECTIVITY_MENU_ITEMS,
    sizeof(CONNECTIVITY_MENU_ITEMS) / sizeof(Action*)
);
OpenScreenAction open_connectivity_menu("Connectivity", &connectivity_menu);

Notification broadcast_notification("Started\nbroadcasting");

void itemaction_broadcast() {
    extern FunctionAction broadcast;
    extern void open_screen(Screen *screen);

    if(broadcasting) {
        ble_stop();
        broadcast.set_name("Broadcast");
        broadcast_notification.set_message("Stopped\nbroadcasting");
        broadcasting = false;
    } else {
        ble_beacon_start();
        broadcast.set_name("Broadcasting");
        broadcast_notification.set_message("Started\nbroadcasting");
        broadcasting = true;
    }

    open_screen(&broadcast_notification);
}
FunctionAction broadcast("Broadcast", itemaction_broadcast);

DummyAction open_battery_menu("Battery");

DummyAction open_screen_menu("Screen");

DummyAction open_info_menu("Info");

// system menu
Action *SETTINGS_MENU_ITEMS[] = {
    &open_connectivity_menu,
    &open_battery_menu,
    &open_screen_menu,
    &open_info_menu,
};
Menu settings_menu(
    SETTINGS_MENU_ITEMS,
    sizeof(SETTINGS_MENU_ITEMS) / sizeof(Action*)
);
OpenScreenAction open_settings_menu("Settings", &settings_menu);

// bmp280 view
SensorView bmp280_temperature_view(&sensors[SENS_BMP280_TEMPERATURE]);
OpenScreenAction open_bmp280_temperature_view("BMP280 Temp", &bmp280_temperature_view);
SensorView bmp280_pressure_view(&sensors[SENS_BMP280_PRESSURE]);
OpenScreenAction open_bmp280_pressure_view("BMP280 Press", &bmp280_pressure_view);

// ahtx0 view
SensorView ahtx0_temperature_view(&sensors[SENS_AHTX0_TEMPERATURE]);
OpenScreenAction open_ahtx0_temperature_view("AHTx0 Temp", &ahtx0_temperature_view);
SensorView ahtx0_humidity_view(&sensors[SENS_AHTX0_HUMIDITY]);
OpenScreenAction open_ahtx0_humidity_view("AHTx0 Humid", &ahtx0_humidity_view);

// bh1750 view
SensorView bh1750_view(&sensors[SENS_BH1750]);
OpenScreenAction open_bh1750_view("BH1750", &bh1750_view);

// bmi160 view
SensorView bmi160_acceleration_view(&sensors[SENS_BMI160_ACCELERATION]);
OpenScreenAction open_bmi160_acceleration_view("BMI160 Accel", &bmi160_acceleration_view);
SensorView bmi160_gyroscope_view(&sensors[SENS_BMI160_GYROSCOPE]);
OpenScreenAction open_bmi160_gyroscope_view("BMI160 Gyro", &bmi160_gyroscope_view);

// sensor menu
Action *SENSOR_DATA_MENU_ITEMS[] = {
    &open_bmp280_temperature_view,
    &open_bmp280_pressure_view,
    &open_ahtx0_temperature_view,
    &open_ahtx0_humidity_view,
    &open_bh1750_view,
    &open_bmi160_acceleration_view,
    &open_bmi160_gyroscope_view,
};
Menu sensor_data_menu(
    SENSOR_DATA_MENU_ITEMS,
    sizeof(SENSOR_DATA_MENU_ITEMS) / sizeof(Action*)
);
OpenScreenAction open_sensor_data_menu("Sensor Data", &sensor_data_menu);

// main menu
Action *MAIN_MENU_ITEMS[] = {
    &open_sensor_data_menu,
    &open_settings_menu,
    &broadcast,
};
Menu main_menu(
    MAIN_MENU_ITEMS,
    sizeof(MAIN_MENU_ITEMS) / sizeof(Action*)
);
