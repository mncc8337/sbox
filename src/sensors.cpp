#include <sensors.h>
#include <BH1750_US.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_AHTX0.h>
#include <BH1750.h>
#include <BMI160_US.h>

Adafruit_BMP280 bmp280;
Adafruit_AHTX0 ahtx0;
BH1750 bh1750_hw;
BH1750_US bh1750(bh1750_hw, 100);
BMI160_US_Accelerometer bmi160_accel(103);
BMI160_US_Gyroscope bmi160_gyro(104);

Adafruit_Sensor *sensors[SENS_COUNT];
uint16_t sensor_mask;

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

uint16_t init_sensors() {
    sensor_mask = 0;
    for(int i = 0; i < SENS_COUNT; i++) {
        sensors[i] = nullptr;
    }

    if(ahtx0.begin()) {
        sensor_mask |= 1 << SENS_AHTX0_TEMPERATURE;
        sensor_mask |= 1 << SENS_AHTX0_HUMIDITY;
        sensors[SENS_AHTX0_TEMPERATURE] = ahtx0.getTemperatureSensor();
        sensors[SENS_AHTX0_HUMIDITY] = ahtx0.getHumiditySensor();
    }

    if(bmp280.begin(0x77)) {
        sensor_mask |= 1 << SENS_BMP280_PRESSURE;
        sensors[SENS_BMP280_PRESSURE] = bmp280.getPressureSensor();
    }

    if(bh1750_hw.begin(BH1750::CONTINUOUS_HIGH_RES_MODE_2, 0x23)) {
        sensor_mask |= 1 << SENS_BH1750;
        sensors[SENS_BH1750] = &bh1750;
    }

    if(BMI160.begin(BMI160GenClass::I2C_MODE, 0x69)) {
        BMI160.setFullScaleAccelRange(BMI160_ACCEL_RANGE_4G); 
        BMI160.setGyroRange(1000);
        sensor_mask |= 1 << SENS_BMI160_ACCELERATION;
        sensor_mask |= 1 << SENS_BMI160_GYROSCOPE;
        sensors[SENS_BMI160_ACCELERATION] = &bmi160_accel;
        sensors[SENS_BMI160_GYROSCOPE] = &bmi160_gyro;
    }

    return sensor_mask;
}

void sleep_sensors() {
    if(SENSOR_ALIVE(SENS_BMP280_PRESSURE))
        bmp280.setSampling(Adafruit_BMP280::MODE_SLEEP);

    if(SENSOR_ALIVE(SENS_BH1750))
        bh1750_hw.configure((BH1750::Mode)BH1750_POWER_DOWN);

    // ahtx0 dont need to sleep
    
    if(SENSOR_ALIVE(SENS_BMI160_ACCELERATION)) {
        BMI160.setRegister(0x7E, 0x10); delay(50);
        BMI160.setRegister(0x7E, 0x14); delay(50);
    }
}

void wake_sensors() {
    if(SENSOR_ALIVE(SENS_BMP280_PRESSURE))
        bmp280.setSampling(Adafruit_BMP280::MODE_NORMAL);

    if(SENSOR_ALIVE(SENS_BH1750))
        bh1750_hw.configure(BH1750::CONTINUOUS_HIGH_RES_MODE_2);

    if(SENSOR_ALIVE(SENS_BMI160_ACCELERATION)) {
        BMI160.setRegister(0x7E, 0x11); delay(50);
        BMI160.setRegister(0x7E, 0x15); delay(50);
    }
}

void set_low_power_sensor_mode() {
    if(SENSOR_ALIVE(SENS_BH1750))
        bh1750_hw.configure(BH1750::ONE_TIME_HIGH_RES_MODE_2);

    if(SENSOR_ALIVE(SENS_BMI160_ACCELERATION)) {
        BMI160.setRegister(0x7E, 0x10); delay(50);
        BMI160.setRegister(0x7E, 0x14); delay(50);
    }
}

void unset_low_power_sensor_mode() {
    if(SENSOR_ALIVE(SENS_BH1750))
        bh1750_hw.configure(BH1750::CONTINUOUS_HIGH_RES_MODE_2);

    if(SENSOR_ALIVE(SENS_BMI160_ACCELERATION)) {
        BMI160.setRegister(0x7E, 0x11); delay(50);
        BMI160.setRegister(0x7E, 0x15); delay(50);
    }
}

void get_sensors_data(sensors_data_t &data) {
    sensors_event_t event;

    for(unsigned i = 0; i < SENS_COUNT; i++) {
        if(!SENSOR_ALIVE(i)) continue;
        
        sensors[i]->getEvent(&event);

        switch(i) {
            case SENS_AHTX0_TEMPERATURE:
                data.temperature = event.temperature;
                break;
            case SENS_AHTX0_HUMIDITY:
                data.humidity = event.relative_humidity;
                break;
            case SENS_BMP280_PRESSURE:
                data.pressure = event.pressure;
                break;
            case SENS_BH1750:
                data.light = event.light;
                break;
            case SENS_BMI160_ACCELERATION: 
                data.accel[0] = event.acceleration.x;
                data.accel[1] = event.acceleration.y;
                data.accel[2] = event.acceleration.z;
                break;
            case SENS_BMI160_GYROSCOPE:
                data.gyro[0] = event.gyro.x;
                data.gyro[1] = event.gyro.y;
                data.gyro[2] = event.gyro.z;
                break;
        }
    }
}
