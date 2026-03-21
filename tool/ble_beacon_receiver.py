import asyncio
import struct
from enum import IntEnum
from bleak import BleakScanner

class SensorType(IntEnum):
    ACCELEROMETER = 1
    MAGNETIC_FIELD = 2
    ORIENTATION = 3
    GYROSCOPE = 4
    LIGHT = 5
    PRESSURE = 6
    PROXIMITY = 8
    GRAVITY = 9
    LINEAR_ACCELERATION = 10
    ROTATION_VECTOR = 11
    RELATIVE_HUMIDITY = 12
    AMBIENT_TEMPERATURE = 13
    OBJECT_TEMPERATURE = 14
    VOLTAGE = 15
    CURRENT = 16
    COLOR = 17
    TVOC = 18
    VOC_INDEX = 19
    NOX_INDEX = 20
    CO2 = 21
    ECO2 = 22
    PM10_STD = 23
    PM25_STD = 24
    PM100_STD = 25
    PM10_ENV = 26
    PM25_ENV = 27
    PM100_ENV = 28
    GAS_RESISTANCE = 29
    UNITLESS_PERCENT = 30
    ALTITUDE = 31

MULTIPLIER_MAP = {
    SensorType.AMBIENT_TEMPERATURE: 0.01,
    SensorType.RELATIVE_HUMIDITY: 0.01,
    SensorType.PRESSURE: 0.1,
    SensorType.LIGHT: 1,
}

UNIT_MAP = {
    SensorType.AMBIENT_TEMPERATURE: "°C",
    SensorType.RELATIVE_HUMIDITY: "%",
    SensorType.PRESSURE: "hPa",
    SensorType.LIGHT: "Lux",
    SensorType.ACCELEROMETER: "m/s2",
    SensorType.GYROSCOPE: "rad/s"
}

def parse_payload(raw_data):
    i = 0
    results = {}
    
    while i < len(raw_data):
        try:
            s_type_val = raw_data[i]
            i += 1
            
            try:
                s_type = SensorType(s_type_val)
            except ValueError:
                continue 

            if s_type in (SensorType.ACCELEROMETER, SensorType.GYROSCOPE):
                if i + 6 <= len(raw_data):
                    x, y, z = struct.unpack_from('<hhh', raw_data, i)
                    i += 6
                    
                    results[s_type.name] = f"X:{x/100:.2f} Y:{y/100:.2f} Z:{z/100:.2f} {UNIT_MAP.get(s_type, '')}"
                else:
                    break
                    
            else:
                if i + 2 <= len(raw_data):
                    raw_val, = struct.unpack_from('<h', raw_data, i)
                    i += 2
                    
                    multiplier = MULTIPLIER_MAP.get(s_type, 1.0)
                    final_val = round(raw_val * multiplier, 2)
                    
                    results[s_type.name] = f"{final_val} {UNIT_MAP.get(s_type, '')}"
                else:
                    break
                    
        except IndexError:
            break
            
    return results

def detection_callback(device, advertisement_data):
    if advertisement_data.local_name != "SBOX":
        return

    mfg_data = advertisement_data.manufacturer_data
    chunk1 = mfg_data.get(0xFFFF, b'')
    
    chunk2 = mfg_data.get(0xFEFF, b'')
    
    all_raw = chunk1 + chunk2
        
    if all_raw:
        print(f"\n[+] CATCHED | MAC: {device.address} | RSSI: {advertisement_data.rssi} dBm")
        print(f"    RAW: {all_raw.hex().upper()}")
        
        decoded = parse_payload(all_raw)
        for sensor, value in decoded.items():
            print(f"    -> {sensor:20}: {value}")

async def main():
    scanner = BleakScanner(
        detection_callback, 
        scanning_mode="active",
        duplicate_value=True
        # adapter="hci0"
    )

    await scanner.start()
    try:
        while True:
            await asyncio.sleep(1.0)
    except asyncio.CancelledError:
        pass
    finally:
        await scanner.stop()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        pass
