#include "Arduino.h"
#include "freertos/projdefs.h"
#include <sensors.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_AHTX0.h>
#include <hp_BH1750.h>
#include <BH1750_US.h>
#include <BMI160_US.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <mutexes.h>

#include <esp_log.h>
#include <sys/unistd.h>

Adafruit_BMP280 bmp280;
Adafruit_AHTX0 ahtx0;
hp_BH1750 bh1750_hw;
BH1750_US bh1750(bh1750_hw, 1750);
BMI160_US_Accelerometer bmi160_accel(160);
BMI160_US_Gyroscope bmi160_gyro(160);

Adafruit_Sensor *sensors[SENS_COUNT];
uint16_t sensor_mask;
uint16_t lognsend_mask = 0xffff;

Adafruit_BMP280::sensor_sampling bmp280_sampling = Adafruit_BMP280::SAMPLING_X16;
Adafruit_BMP280::sensor_filter bmp280_filter = Adafruit_BMP280::FILTER_X4;
Adafruit_BMP280::standby_duration bmp280_standby_duration = Adafruit_BMP280::STANDBY_MS_1;

int bmi160_accel_range = BMI160_ACCEL_RANGE_2G;
int bmi160_accel_odr = BMI160_ACCEL_RATE_100HZ;

int bmi160_gyro_range = BMI160_GYRO_RANGE_2000;
int bmi160_gyro_odr = BMI160_GYRO_RATE_50HZ;

QueueHandle_t live_data_sensor_event_queue;
volatile int live_data_sensor_id = -1;

QueueHandle_t all_lognsend_sensor_data_queue;

const char* get_sensor_type_string(const int &type) {
    switch(type) {
        case SENSOR_TYPE_ACCELEROMETER: return "Accelerometer";
        case SENSOR_TYPE_MAGNETIC_FIELD: return "Magnetic Field";
        case SENSOR_TYPE_ORIENTATION: return "Orientation";
        case SENSOR_TYPE_GYROSCOPE: return "Gyroscope";
        case SENSOR_TYPE_LIGHT: return "Light";
        case SENSOR_TYPE_PRESSURE: return "Pressure";
        case SENSOR_TYPE_PROXIMITY: return "Proximity";
        case SENSOR_TYPE_GRAVITY: return "Gravity";
        case SENSOR_TYPE_LINEAR_ACCELERATION: return "Linear Acceleration";
        case SENSOR_TYPE_ROTATION_VECTOR: return "Rotation Vector";
        case SENSOR_TYPE_RELATIVE_HUMIDITY: return "Relative Humidity";
        case SENSOR_TYPE_AMBIENT_TEMPERATURE: return "Ambient Temperature";
        case SENSOR_TYPE_OBJECT_TEMPERATURE: return "Object Temperature";
        case SENSOR_TYPE_VOLTAGE: return "Voltage";
        case SENSOR_TYPE_CURRENT: return "Current";
        case SENSOR_TYPE_COLOR: return "Color";
        case SENSOR_TYPE_TVOC: return "TVOC";
        case SENSOR_TYPE_VOC_INDEX: return "VOC Index";
        case SENSOR_TYPE_NOX_INDEX: return "NOX Index";
        case SENSOR_TYPE_CO2: return "CO2";
        case SENSOR_TYPE_ECO2: return "eCO2";
        case SENSOR_TYPE_PM10_STD: return "PM10 STD";
        case SENSOR_TYPE_PM25_STD: return "PM25 STD";
        case SENSOR_TYPE_PM100_STD: return "PM100 STD";
        case SENSOR_TYPE_PM10_ENV: return "PM10 ENV";
        case SENSOR_TYPE_PM25_ENV: return "PM25 ENV";
        case SENSOR_TYPE_PM100_ENV: return "PM100 ENV";
        case SENSOR_TYPE_GAS_RESISTANCE: return "Gas Resistance";
        case SENSOR_TYPE_UNITLESS_PERCENT: return "Unitless Percent";
        case SENSOR_TYPE_ALTITUDE: return "Altitude";
        default: return "Unknown";
    }
}

const char* get_sensor_unit_string(const int type) {
    switch(type) {
        case SENSOR_TYPE_ACCELEROMETER: 
        case SENSOR_TYPE_GRAVITY: 
        case SENSOR_TYPE_LINEAR_ACCELERATION: 
            return "m/s2";
            
        case SENSOR_TYPE_MAGNETIC_FIELD: return "uT";
        case SENSOR_TYPE_ORIENTATION: return "deg";
        case SENSOR_TYPE_GYROSCOPE: return "rad/s";
        case SENSOR_TYPE_LIGHT: return "Lux";
        case SENSOR_TYPE_PRESSURE: return "hPa";
        case SENSOR_TYPE_PROXIMITY: return "cm";
        case SENSOR_TYPE_RELATIVE_HUMIDITY: return "%";
        case SENSOR_TYPE_UNITLESS_PERCENT: return "%";
        
        case SENSOR_TYPE_AMBIENT_TEMPERATURE: 
        case SENSOR_TYPE_OBJECT_TEMPERATURE: 
            return "\xB0""C";
            
        case SENSOR_TYPE_VOLTAGE: return "V";
        case SENSOR_TYPE_CURRENT: return "mA";
        case SENSOR_TYPE_TVOC: return "ppb";

        case SENSOR_TYPE_CO2: 
        case SENSOR_TYPE_ECO2:
            return "ppm";
        
        case SENSOR_TYPE_PM10_STD:
        case SENSOR_TYPE_PM25_STD:
        case SENSOR_TYPE_PM100_STD:
        case SENSOR_TYPE_PM10_ENV:
        case SENSOR_TYPE_PM25_ENV:
        case SENSOR_TYPE_PM100_ENV: 
            return "ug/m3";
            
        case SENSOR_TYPE_GAS_RESISTANCE: return "Ohm";
        case SENSOR_TYPE_ALTITUDE: return "m";
        case SENSOR_TYPE_COLOR: return "rgb";

        case SENSOR_TYPE_ROTATION_VECTOR: 
        case SENSOR_TYPE_VOC_INDEX: 
        case SENSOR_TYPE_NOX_INDEX: 
        default:
            return "";
    }
}

static void bmi160_on() {
    BMI160.setRegister(0x7E, 0x11); delay(50);
    BMI160.setRegister(0x7E, 0x15); delay(50);
}

static void bmi160_off() {
    BMI160.setRegister(0x7E, 0x10); delay(50);
    BMI160.setRegister(0x7E, 0x14); delay(50);
}

uint16_t sensors_init() {
    sensor_mask = 0;
    for(int i = 0; i < SENS_COUNT; i++) {
        sensors[i] = nullptr;
    }

    if(bh1750_hw.begin(0x23)) {
        sensor_mask |= 1 << SENS_LIGHT;
        sensors[SENS_LIGHT] = &bh1750;
        bh1750_hw.calibrateTiming();
        bh1750_hw.start(BH1750_QUALITY_HIGH2, 69);
        ESP_LOGI("SENSORS", "BH1750 initialized");
    } else {
        ESP_LOGE("SENSORS", "BH1750 failed to initialize");
    }

    if(ahtx0.begin()) {
        sensor_mask |= 1 << SENS_TEMPERATURE;
        sensor_mask |= 1 << SENS_HUMIDITY;
        sensors[SENS_TEMPERATURE] = ahtx0.getTemperatureSensor();
        sensors[SENS_HUMIDITY] = ahtx0.getHumiditySensor();
        ESP_LOGI("SENSORS", "AHT20 initialized");
    } else {
        ESP_LOGE("SENSORS", "AHT20 failed to initialize");
    }

    if(bmp280.begin(0x77)) {
        sensor_mask |= 1 << SENS_PRESSURE;
        sensors[SENS_PRESSURE] = bmp280.getPressureSensor();

        bmp280.setSampling(
            Adafruit_BMP280::MODE_NORMAL,
            Adafruit_BMP280::SAMPLING_X2,
            bmp280_sampling,
            bmp280_filter,
            bmp280_standby_duration
        );
        ESP_LOGI("SENSORS", "BMP280 initialized");
    } else {
        ESP_LOGE("SENSORS", "BMP280 failed to initialize");
    }

    if(BMI160.begin(BMI160GenClass::I2C_MODE, 0x69)) {
        sensor_mask |= 1 << SENS_ACCELERATION;
        sensor_mask |= 1 << SENS_GYROSCOPE;
        sensors[SENS_ACCELERATION] = &bmi160_accel;
        sensors[SENS_GYROSCOPE] = &bmi160_gyro;
        BMI160.setFullScaleAccelRange(bmi160_accel_range); 
        BMI160.setAccelRate(bmi160_accel_odr);

        BMI160.setFullScaleGyroRange(bmi160_gyro_range);
        BMI160.setGyroRange(bmi160_gyro_range);
        BMI160.setGyroRate(bmi160_gyro_odr);
        ESP_LOGI("SENSORS", "BMI160 initialized");
    } else {
        ESP_LOGE("SENSORS", "BMI160 failed to initialize");
    }

    // setup rtos task stuff
    live_data_sensor_event_queue = xQueueCreate(1, sizeof(sensors_event_t));
    all_lognsend_sensor_data_queue =  xQueueCreate(1, sizeof(sensors_data_t));

    return sensor_mask;
}

void sleep_sensors() {
    if(SENSOR_ALIVE(SENS_PRESSURE))
        bmp280.setSampling(Adafruit_BMP280::MODE_SLEEP);

    // ahtx0 and bh1750 dont need to sleep

    if(SENSOR_ALIVE(SENS_ACCELERATION))
        bmi160_off();

    ESP_LOGI("SENSORS", "Set all sensors to sleep mode");
}

void wake_sensors() {
    if(SENSOR_ALIVE(SENS_PRESSURE))
        bmp280.setSampling(Adafruit_BMP280::MODE_NORMAL);

    if(SENSOR_ALIVE(SENS_ACCELERATION))
        bmi160_on();

    ESP_LOGI("SENSORS", "All sensors waken");
}

void set_low_power_sensor_mode() {
    if(SENSOR_ALIVE(SENS_ACCELERATION))
        bmi160_off();

    ESP_LOGI("SENSORS", "Set all sensors to low power mode");
}

void unset_low_power_sensor_mode() {
    if(SENSOR_ALIVE(SENS_ACCELERATION))
        bmi160_on();

    ESP_LOGI("SENSORS", "Unset all sensors to low power mode");
}

void request_live_data_sensor_poll(int target_sensor) {
    live_data_sensor_id = target_sensor;
    xQueueReset(live_data_sensor_event_queue);
}

bool requested_live_data_poll_ready(sensors_event_t &out_event) {
    if(xQueueReceive(live_data_sensor_event_queue, &out_event, 0) == pdPASS) {
        live_data_sensor_id = -1;
        return true;
    }
    return false;
}

bool all_data_poll_ready(sensors_data_t &out_data) {
    if(xQueueReceive(all_lognsend_sensor_data_queue, &out_data, 0) == pdPASS) {
        return true;
    }
    return false;
}

extern bool is_session_running;
void sensors_task(void *parameters) {
    uint16_t ready_mask = 0;
    sensors_data_t all_data = {0};

    while(true) {
        int live_data_local_id = live_data_sensor_id;

        // poll all enabled sensors
        if(is_session_running) {
            sensors_event_t t_event;

            for(unsigned i = 0; i < SENS_COUNT; i++) {
                if(!SENSOR_ALIVE(i) || !SENSOR_ACTIVE(i) || ((ready_mask >> i) & 1)) continue;

                bool ready = sensors[i]->getEvent(&t_event);
                if(!ready) continue;

                t_event.sensor_id = i;
                ready_mask |= 1 << i;

                // poll live data sensor
                if(i == live_data_local_id)
                    xQueueOverwrite(live_data_sensor_event_queue, &t_event);

                switch(i) {
                    case SENS_TEMPERATURE:
                        all_data.temperature = t_event.temperature;
                        break;
                    case SENS_HUMIDITY:
                        all_data.humidity = t_event.relative_humidity;
                        break;
                    case SENS_PRESSURE:
                        all_data.pressure = t_event.pressure;
                        break;
                    case SENS_LIGHT:
                        all_data.light = t_event.light;
                        break;
                    case SENS_ACCELERATION: 
                        all_data.accel[0] = t_event.acceleration.x;
                        all_data.accel[1] = t_event.acceleration.y;
                        all_data.accel[2] = t_event.acceleration.z;
                        break;
                    case SENS_GYROSCOPE:
                        all_data.gyro[0] = t_event.gyro.x;
                        all_data.gyro[1] = t_event.gyro.y;
                        all_data.gyro[2] = t_event.gyro.z;
                        break;
                }
            }

            if(ready_mask == lognsend_mask) {
                xQueueOverwrite(all_lognsend_sensor_data_queue, &all_data);
                ready_mask = 0;
                all_data = {0};
                // data packet isnt sent that regularly
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            }
            // repoll, some sensors aint ready
        } else {
            // poll live data sensor
            if(live_data_local_id >= 0 && SENSOR_ALIVE(live_data_local_id)) {
                sensors_event_t t_event;
                bool ready = sensors[live_data_local_id]->getEvent(&t_event);
                t_event.sensor_id = live_data_local_id;

                if(ready) {
                    xQueueOverwrite(live_data_sensor_event_queue, &t_event);

                    // it is guarantee that no sensorview screen has sample interval
                    // smaller than 40ms
                    vTaskDelay(pdMS_TO_TICKS(40));
                    continue;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelete(NULL);
}
