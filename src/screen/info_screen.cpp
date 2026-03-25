#include <screen.h>
#include <time.h>
#include <sensors.h>
#include <esp_mac.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <esp_chip_info.h>

extern uint8_t battery_percentage;
extern uint16_t psu_voltage_avg;
extern bool is_session_running;

void InfoScreen::process_navigation(
    unsigned long button_select_press_duration,
    bool button_up_clicked,
    bool button_down_clicked
) {
    if(button_down_clicked) {
        current_y -= 8;
        redraw_request = true;
    }

    if(button_up_clicked && current_y < 6) {
        current_y += 8;
        redraw_request = true;
    }

    if(millis() - last_update_ts >= 1000) {
        redraw_request = true;
        last_update_ts = millis();
    }
}

void InfoScreen::draw(U8G2 &u8g2) {
    u8g2.setFont(u8g2_font_5x8_tf);

    int y = current_y;

    u8g2.setCursor(0, y); u8g2.printf("Battery: %dmV | %d%%", psu_voltage_avg, battery_percentage);
    y += 8;

    u8g2.setCursor(0, y);
    if(is_session_running) {
        u8g2.print("Session started");
    } else {
        u8g2.print("Session stopped");
    }
    y += 8;

    uint32_t free_heap = esp_get_free_heap_size() / 1024;
    u8g2.setCursor(0, y); u8g2.printf("Free mem: %luKiB", free_heap);
    y += 8;

    struct tm timeinfo;
    getLocalTime(&timeinfo);
    char buff[32];
    strftime(buff, 32, "%a %F %T", &timeinfo);
    u8g2.setCursor(0, y); u8g2.printf("Current time:");
    y += 8;
    u8g2.setCursor(5, y); u8g2.printf(buff);
    y += 8;

    uint64_t uptime_sec = esp_timer_get_time() / 1000000;
    u8g2.setCursor(0, y); u8g2.printf("Uptime: %llus", uptime_sec);
    y += 8;

    uint8_t mac[6];

    esp_read_mac(mac, ESP_MAC_WIFI_STA); 
    u8g2.setCursor(0, y); 
    u8g2.printf("WiFiMAC %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    y += 8;

    esp_read_mac(mac, ESP_MAC_BT); 
    u8g2.setCursor(0, y); 
    u8g2.printf("BTMAC %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    y += 8;

    int alive_count = 0;
    for(unsigned i = 0; i < SENS_COUNT; i++) {
        if(SENSOR_ALIVE(i)) alive_count++;
    }
    u8g2.setCursor(0, y); 
    u8g2.printf("Sensors OK: %d/%d", alive_count, SENS_COUNT);
    y += 8;

    u8g2.setCursor(0, y);
    u8g2.print("RST Reason: ");
    switch(esp_reset_reason()) {
        case ESP_RST_UNKNOWN:
            u8g2.print("UNKNOWN");
            break;
        case ESP_RST_POWERON:
            u8g2.print("PWR_ON");
            break;
        case ESP_RST_EXT:
            u8g2.print("EXT_PIN");
            break;
        case ESP_RST_SW:
            u8g2.print("SOFT");
            break;
        case ESP_RST_PANIC:
            u8g2.print("PANIC");
            break;
        case ESP_RST_INT_WDT:
            u8g2.print("INT_WDT");
            break;
        case ESP_RST_TASK_WDT:
            u8g2.print("TSK_WDT");
            break;
        case ESP_RST_WDT:
            u8g2.print("OTHER_WDT");
            break;
        case ESP_RST_DEEPSLEEP:
            u8g2.print("WAKE_DS");
            break;
        case ESP_RST_BROWNOUT:
            u8g2.print("BROWNOUT");
            break;
        case ESP_RST_SDIO:
            u8g2.print("SDIO");
            break;
        default:
            u8g2.print("OTHER");
    }
    y += 8;

    u8g2.setCursor(0, y);
    u8g2.print("Chip: ");
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    switch(chip_info.model) {
        case CHIP_ESP32:
            u8g2.print("ESP32");
            break;
        case CHIP_ESP32S2:
            u8g2.print("ESP32-S2");
            break;
        case CHIP_ESP32S3:
            u8g2.print("ESP32-S3");
            break;
        case CHIP_ESP32C3:
            u8g2.print("ESP32-C3");
            break;
        case CHIP_ESP32H2:
            u8g2.print("ESP32-H2");
            break;
        default:
            u8g2.printf("OTHER (%d)", chip_info.model);
            break;
    }

    u8g2.printf(" rev%d %dcore", chip_info.revision, chip_info.cores); y += 8;
    u8g2.setCursor(0, y); u8g2.printf("Features:"); y += 8;

    uint32_t feature_mask = chip_info.features;
    if(feature_mask & 1) {
        u8g2.setCursor(5, y); u8g2.printf("Embedded flash"); y += 8;
    }
    feature_mask >>= 1;
    if(feature_mask & 1) {
        u8g2.setCursor(5, y); u8g2.printf("2.4GHz WiFi"); y += 8;
    }
    feature_mask >>= 3;
    if(feature_mask & 1) {
        u8g2.setCursor(5, y); u8g2.printf("Bluetooth LE"); y += 8;
    }
    feature_mask >>= 1;
    if(feature_mask & 1) {
        u8g2.setCursor(5, y); u8g2.printf("Bluetooth classic"); y += 8;
    }
    feature_mask >>= 1;
    if(feature_mask & 1) {
        u8g2.setCursor(5, y); u8g2.printf("IEEE 802.15.4"); y += 8;
    }
    feature_mask >>= 1;
    if(feature_mask & 1) {
        u8g2.setCursor(5, y); u8g2.printf("Embedded PSRAM"); y += 8;
    }

    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);
    u8g2.setCursor(0, y); 
    u8g2.printf("Flash size: %lu MiB", flash_size / (1024 * 1024)); 
    y += 8;

    u8g2.setCursor(0, y);
    y += 8;
    u8g2.print("Build:");

    u8g2.setCursor(5, y);
    u8g2.printf("%s %s", __DATE__, __TIME__);
    y += 8;

    redraw_request = false;
}
