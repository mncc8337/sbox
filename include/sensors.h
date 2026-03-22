#ifndef SENSORS_H
#define SENSORS_H

#define SENSOR_ALIVE(id) ((sensor_mask >> id) & 1)

#include <stdint.h>

class Adafruit_Sensor;

enum SensorEnum {
    SENS_TEMPERATURE,
    SENS_HUMIDITY,
    SENS_PRESSURE,
    SENS_LIGHT,
    SENS_ACCELERATION,
    SENS_GYROSCOPE,
    SENS_COUNT // put this at the end of the list
};

typedef struct {
    float temperature;
    float humidity;
    float pressure;
    float light;
    float accel[3];
    float gyro[3];
} sensors_data_t;

extern Adafruit_Sensor *sensors[SENS_COUNT];
extern uint16_t sensor_mask;

extern const char* get_sensor_type_string(const int &type);
extern const char* get_sensor_unit_string(const int type);
extern uint16_t init_sensors();

extern void sleep_sensors();
extern void wake_sensors();

extern void set_low_power_sensor_mode();
extern void unset_low_power_sensor_mode();

extern void get_sensors_data(sensors_data_t &data);

#endif
