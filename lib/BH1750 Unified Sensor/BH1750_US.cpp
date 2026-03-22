#include <BH1750_US.h>

BH1750_US::BH1750_US(BH1750 &hw, int32_t id)
    : sensor_hw(hw), sensor_id(id) {}

bool BH1750_US::getEvent(sensors_event_t* event) {
    memset(event, 0, sizeof(sensors_event_t));
    event->version = 1;
    event->sensor_id = sensor_id;
    event->type = SENSOR_TYPE_LIGHT;
    event->timestamp = millis();
    event->light = sensor_hw.readLightLevel();
    
    return true; 
}

void BH1750_US::getSensor(sensor_t* sensor) {
    memset(sensor, 0, sizeof(sensor_t));
    strncpy(sensor->name, "BH1750", sizeof(sensor->name) - 1);
    sensor->version = 1;
    sensor->sensor_id = sensor_id;
    sensor->type = SENSOR_TYPE_LIGHT;
    sensor->min_value = 0.0f;
    sensor->max_value = 54612.5f;
    sensor->resolution = 1.0f;
}

