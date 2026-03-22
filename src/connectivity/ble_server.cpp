#include "NimBLECharacteristic.h"
#include <connectivity.h>
#include <NimBLEDevice.h>
#include <NimBLEUtils.h>
#include <NimBLEServer.h>
#include <sensors.h>
#include <Adafruit_Sensor.h>

static NimBLEServer* ble_server = nullptr;

static NimBLECharacteristic* bat_characteristic = nullptr;
static NimBLECharacteristic* temp_characteristic = nullptr;
static NimBLECharacteristic* hum_characteristic = nullptr;
static NimBLECharacteristic* press_characteristic = nullptr;
static NimBLECharacteristic* light_characteristic = nullptr;
static NimBLECharacteristic* accel_characteristic = nullptr;
static NimBLECharacteristic* gyro_characteristic = nullptr;

NimBLECharacteristic **sensor_characteristic_map[] {
    &temp_characteristic,
    &hum_characteristic,
    &press_characteristic,
    &light_characteristic,
    &accel_characteristic,
    &gyro_characteristic,
};

volatile bool device_connected = false;

class MyServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        device_connected = true;
    };

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        device_connected = false;
        NimBLEDevice::startAdvertising();
    }
};

void ble_server_start() {
    ble_server = NimBLEDevice::createServer();
    ble_server->setCallbacks(new MyServerCallbacks());

    NimBLEService* battery_service = ble_server->createService("180F");
    bat_characteristic = battery_service->createCharacteristic(
        "2A19", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    NimBLEService* env_service = ble_server->createService("181A");
    temp_characteristic = env_service->createCharacteristic(
        "2A6E", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    hum_characteristic = env_service->createCharacteristic(
        "2A6F", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    press_characteristic = env_service->createCharacteristic(
        "2A6D", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    light_characteristic = env_service->createCharacteristic(
        "2AFB", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    NimBLEService* measurement_service = ble_server->createService("185A");
    accel_characteristic = measurement_service->createCharacteristic(
        "2C06", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    gyro_characteristic = measurement_service->createCharacteristic(
        "2C09", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
    advertising->reset();
    advertising->setName("SBOX");
    advertising->addServiceUUID("180F");
    advertising->addServiceUUID("181A");
    advertising->addServiceUUID("185A");

    advertising->setAppearance(0x552);
    advertising->enableScanResponse(true);
    advertising->start();
}

void ble_server_update(const sensors_data_t &data, const uint8_t bat_level) {
    if(!device_connected) return; 

    std::vector<uint8_t> data_to_send;

    for(unsigned i = 0; i < SENS_COUNT; i++) {
        if (!SENSOR_ALIVE(i)) continue; 
        
        data_to_send.clear();

        switch(i) {
            case SENS_TEMPERATURE: {
                int16_t t = (int16_t)(data.temperature * 100 + 0.5f);
                data_to_send.assign((uint8_t*)&t, (uint8_t*)&t + 2);
                break;
            }
            case SENS_HUMIDITY: {
                uint16_t h = (uint16_t)(data.humidity * 100 + 0.5f);
                data_to_send.assign((uint8_t*)&h, (uint8_t*)&h + 2);
                break;
            }
            case SENS_PRESSURE: {
                uint32_t p = (uint32_t)(data.pressure * 1000 + 0.5f);
                data_to_send.assign((uint8_t*)&p, (uint8_t*)&p + 4);
                break;
            }
            case SENS_LIGHT: {
                uint32_t lux_val = (uint32_t)(data.light * 100.0f + 0.5f);
                data_to_send.assign((uint8_t*)&lux_val, (uint8_t*)&lux_val + 3);
                break;
            }
            case SENS_ACCELERATION: {
                int16_t accel_data[3];
                for(int k=0; k<3; k++) accel_data[k] = (int16_t)(data.accel[k] * 100);
                data_to_send.assign((uint8_t*)accel_data, (uint8_t*)accel_data + 6);
                break;
            }
            case SENS_GYROSCOPE: {
                int16_t gyro_data[3];
                for(int k=0; k<3; k++) gyro_data[k] = (int16_t)(data.gyro[k] * 100);
                data_to_send.assign((uint8_t*)gyro_data, (uint8_t*)gyro_data + 6);
                break;
            }
        }

        if(!data_to_send.empty()) {
            NimBLECharacteristic *characteristic = *sensor_characteristic_map[i];
            if(characteristic != nullptr) {
                characteristic->setValue(data_to_send);
                characteristic->notify();
            }
        }
    }

    bat_characteristic->setValue(&bat_level, 1);
    bat_characteristic->notify();
}

void ble_server_stop() {
    if (ble_server != nullptr) {
        NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
        if (advertising->isAdvertising()) {
            advertising->stop();
        }

        std::vector<uint16_t> connected_peers = ble_server->getPeerDevices();
        if (connected_peers.size() > 0) {
            for(size_t i = 0; i < connected_peers.size(); i++) {
                ble_server->disconnect(connected_peers[i]);
            }
        }

        device_connected = false;
    }
}
