#include <ble.h>
#include <NimBLEDevice.h>
#include <NimBLEUtils.h>
#include <NimBLEServer.h>

NimBLEServer* ble_server = nullptr;
NimBLECharacteristic* temp_characteristic = nullptr;
NimBLECharacteristic* bat_characteristic = nullptr;

void ble_beacon_start() {
    NimBLEDevice::init("SBOX");
    
    NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();
    NimBLEAdvertisementData advertisement_data = NimBLEAdvertisementData();

    advertisement_data.setFlags(0x04); 
    advertisement_data.setName("SBOX");

    std::string manufacturer_data(4, 0);

    // manufacturer data
    manufacturer_data[0] = (char)0xFF;
    manufacturer_data[1] = (char)0xFF;
    
    // sensors data
    manufacturer_data[2] = (char)12;
    manufacturer_data[3] = (char)25;

    advertisement_data.setManufacturerData(manufacturer_data);
    advertising->setAdvertisementData(advertisement_data);

    advertising->setMinInterval(0x100);
    advertising->setMaxInterval(0x120);

    advertising->start();
}

void ble_server_start() {
    NimBLEDevice::init("SBOX"); 

    ble_server = NimBLEDevice::createServer();

    NimBLEService* battery_service = ble_server->createService("180F");
    
    bat_characteristic = battery_service->createCharacteristic(
        "2A19",
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    
    uint8_t bat_level = 12;
    bat_characteristic->setValue(&bat_level, 1);
    battery_service->start();

    NimBLEService* env_service = ble_server->createService("181A");
    temp_characteristic = env_service->createCharacteristic(
        "2A6E",
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    
    int16_t temp_val = 2500;
    temp_characteristic->setValue((uint8_t*)&temp_val, 2);
    
    env_service->start();

    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
    advertising->addServiceUUID("180F");
    advertising->addServiceUUID("181A");
    
    advertising->start();
}

void ble_stop() {
    NimBLEDevice::deinit(true);
}
