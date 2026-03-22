#include <connectivity.h>
#include <NimBLEDevice.h>
#include <NimBLEUtils.h>

#include <sensors.h>
#include <Adafruit_Sensor.h>

static void make_ble_beacon_payload(const sensors_data_t &data, std::vector<uint8_t>& payload) {
    payload.clear();

    payload.push_back(0xFF);
    payload.push_back(0xFF);

    for(unsigned i = 0; i < SENS_COUNT; i++) {
        if(!SENSOR_ALIVE(i)) continue;

        switch(i) {
            case SENS_LIGHT: {
                payload.push_back(SENSOR_TYPE_LIGHT);
                uint16_t lux = (uint16_t)(data.light + 0.5f);
                payload.push_back(lux & 0xFF);
                payload.push_back((lux >> 8) & 0xFF);
                break;
            }
            case SENS_TEMPERATURE: {
                payload.push_back(SENSOR_TYPE_AMBIENT_TEMPERATURE);
                int16_t t = (int16_t)(data.temperature * 100 + 0.5f);
                payload.push_back(t & 0xFF);
                payload.push_back((t >> 8) & 0xFF);
                break;
            }
            case SENS_HUMIDITY: {
                payload.push_back(SENSOR_TYPE_RELATIVE_HUMIDITY);
                uint16_t h = (uint16_t)(data.humidity * 100 + 0.5f);
                payload.push_back(h & 0xFF);
                payload.push_back((h >> 8) & 0xFF);
                break;
            }
            case SENS_PRESSURE: {
                payload.push_back(SENSOR_TYPE_PRESSURE);
                uint16_t p = (uint16_t)(data.pressure * 10 + 0.5f);
                payload.push_back(p & 0xFF);
                payload.push_back((p >> 8) & 0xFF);
                break;
            }
            case SENS_ACCELERATION: {
                payload.push_back(SENSOR_TYPE_ACCELEROMETER);
                for(int k = 0; k < 3; k++) {
                    int16_t a = (int16_t)(data.accel[k] * 100);
                    payload.push_back(a & 0xFF);
                    payload.push_back((a >> 8) & 0xFF);
                }
                break;
            }
            case SENS_GYROSCOPE: {
                payload.push_back(SENSOR_TYPE_GYROSCOPE);
                for(int k = 0; k < 3; k++) {
                    int16_t a = (int16_t)(data.gyro[k] * 100);
                    payload.push_back(a & 0xFF);
                    payload.push_back((a >> 8) & 0xFF);
                }
                break;
            }
        }
    }

    if(payload.size() > 20) {
        payload.insert(payload.begin() + 20, {0xFF, 0xFF});
    }
}

void ble_beacon_start() {
    NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();
    advertising->reset();
    NimBLEAdvertisementData advertisement_data = NimBLEAdvertisementData();

    advertisement_data.setName("SBOX");
    // advertisement_data.setFlags(0x04); 
    // advertisement_data.setAppearance(0x552);
    // advertisement_data.addTxPower();

    advertising->setAdvertisementData(advertisement_data);

    advertising->setMinInterval(0xa0);
    advertising->setMaxInterval(0x1e0);

    advertising->start();
}

void ble_beacon_set_data(const sensors_data_t &data) {
    std::vector<uint8_t> payload;
    payload.reserve(62);
    make_ble_beacon_payload(data, payload);

    NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();

    NimBLEAdvertisementData advertisement_data = NimBLEAdvertisementData();
    advertisement_data.setName("SBOX");
    size_t first_chunk_size = std::min((size_t)20, payload.size());
    advertisement_data.setManufacturerData(payload.data(), first_chunk_size);
    advertising->setAdvertisementData(advertisement_data);

    if(payload.size() > 20) {
        NimBLEAdvertisementData scan_response_data = NimBLEAdvertisementData();
        size_t remaining_size = payload.size() - 20;
        scan_response_data.setManufacturerData(&payload[20], remaining_size);
        advertising->setScanResponseData(scan_response_data);
    }
}

void ble_beacon_stop() {
    NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();

    if(advertising->isAdvertising())
        advertising->stop();
}
