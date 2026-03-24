#include <connectivity.h>
#include <NimBLEDevice.h>
#include <NimBLEUtils.h>

bool ble_init() {
    return NimBLEDevice::init("SBOX");
}
