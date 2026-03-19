#include <cstring>
#include <screen.h>
#include <action.h>
#include <bitmap.h>
#include <Adafruit_Sensor.h>

#define FONT_HEIGHT 12

SensorView::SensorView(Adafruit_Sensor **sensor_ptr) : sensor_ptr(sensor_ptr) {};

void SensorView::process_navigation(
    unsigned long button_select_press_duration,
    bool button_up_clicked,
    bool button_down_clicked
) {
    // if (button_up_clicked && scroll_y > 0) {
    //     request_redraw();
    // }
    // if (button_down_clicked && scroll_y < FONT_HEIGHT * 5) {
    //     request_redraw();
    // }
}

void SensorView::draw(U8G2 &u8g2, int offset_y) {
    if(!initialized) {
        sensor_t sensor_info;
        (*sensor_ptr)->getSensor(&sensor_info);
        strncpy(sensor_name, sensor_info.name, sizeof(sensor_name));
        sensor_type = sensor_info.type;

        initialized = true;
    }

    sensors_event_t event;
    (*sensor_ptr)->getEvent(&event);

    u8g2.setFont(u8g2_font_glasstown_nbp_tf);

    int y = FONT_HEIGHT + offset_y;

    u8g2.setCursor(0, y); u8g2.printf("Name: %s", sensor_name); y += FONT_HEIGHT;
    // u8g2.setCursor(0, y); u8g2.printf("ID: %d", event.sensor_id); y += FONT_HEIGHT;
    // u8g2.setCursor(0, y); u8g2.printf("TS: %d", event.timestamp); y += FONT_HEIGHT;

    switch(sensor_type) {
        case SENSOR_TYPE_AMBIENT_TEMPERATURE:
            u8g2.setCursor(0, y); u8g2.print("Type: Temp"); y += FONT_HEIGHT;
            u8g2.setCursor(0, y); u8g2.printf("Val: %.2f *C", event.temperature);
            break;
        case SENSOR_TYPE_RELATIVE_HUMIDITY:
            u8g2.setCursor(0, y); u8g2.print("Type: Humid"); y += FONT_HEIGHT;
            // Dấu %% để in ra ký tự % nhé
            u8g2.setCursor(0, y); u8g2.printf("Val: %.2f %%", event.relative_humidity); 
            break;
        case SENSOR_TYPE_PRESSURE:
            u8g2.setCursor(0, y); u8g2.print("Type: Press"); y += FONT_HEIGHT;
            u8g2.setCursor(0, y); u8g2.printf("Val: %.2f hPa", event.pressure);
            break;
        case SENSOR_TYPE_LIGHT:
            u8g2.setCursor(0, y); u8g2.print("Type: Light"); y += FONT_HEIGHT;
            u8g2.setCursor(0, y); u8g2.printf("Val: %.0f lx", event.light);
            break;
        case SENSOR_TYPE_ACCELEROMETER:
            u8g2.setCursor(0, y);
            u8g2.print("Type: Accel"); y += FONT_HEIGHT;
            u8g2.setCursor(0, y);
            u8g2.printf(
                "xyz: m/s2 %.2f %.2f %.2f",
                event.acceleration.x,
                event.acceleration.y,
                event.acceleration.z
            );
            y += FONT_HEIGHT;
            break;
        case SENSOR_TYPE_GYROSCOPE:
            u8g2.setCursor(0, y); u8g2.print("Type: Gyro"); y += FONT_HEIGHT;
            u8g2.setCursor(0, y); u8g2.printf(
                "xyz: rad/s %.2f %.2f %.2f",
                event.gyro.x,
                event.gyro.y,
                event.gyro.z
            ); y += FONT_HEIGHT;
            break;
        default:
            u8g2.setCursor(0, y); u8g2.print("No Data");
            break;
    }

    // redraw_request = false;
}
