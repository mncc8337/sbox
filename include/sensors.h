#ifndef SENSORS_H
#define SENSORS_H

#define SENSOR_ALIVE(id) ((sensor_mask >> id) & 1)
#define SENSOR_ACTIVE(id) ((lognsend_mask >> id) & 1)

#include <stdint.h>
#include <Adafruit_Sensor.h>

class Adafruit_Sensor;

enum SensorEnum {
    SENS_LIGHT,
    SENS_TEMPERATURE,
    SENS_HUMIDITY,
    SENS_PRESSURE,
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
extern uint16_t lognsend_mask;

extern const char* get_sensor_type_string(const int &type);
extern const char* get_sensor_unit_string(const int type);
extern uint16_t sensors_init();

extern void sleep_sensors();
extern void wake_sensors();

extern void set_low_power_sensor_mode();
extern void unset_low_power_sensor_mode();

extern void request_live_data_sensor_poll(int target_sensor);
extern bool requested_live_data_poll_ready(sensors_event_t &out_event);
extern bool all_data_poll_ready(sensors_data_t &out_data);

extern void sensors_task(void *parameters);

#endif
