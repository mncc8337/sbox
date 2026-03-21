#include <cstring>
#include <screen.h>
#include <action.h>
#include <bitmap.h>
#include <sensors.h>
#include <Adafruit_Sensor.h>

static float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

SensorView::SensorView(Adafruit_Sensor &sensor) : sensor(sensor) {
    sensor_t sensor_info;
    sensor.getSensor(&sensor_info);
    strncpy(sensor_name, sensor_info.name, sizeof(sensor_name));
    sensor_type = sensor_info.type;

    max_sensor_value = sensor_info.max_value;
    min_sensor_value = sensor_info.min_value;

    multi_axis = (
        sensor_type == SENSOR_TYPE_ACCELEROMETER
        || sensor_type == SENSOR_TYPE_GYROSCOPE
        || sensor_type == SENSOR_TYPE_MAGNETIC_FIELD
        || sensor_type == SENSOR_TYPE_GYROSCOPE
        || sensor_type == SENSOR_TYPE_COLOR
    );

    auto_scaling = (
        sensor_type != SENSOR_TYPE_RELATIVE_HUMIDITY
        && sensor_type != SENSOR_TYPE_ORIENTATION
        && sensor_type != SENSOR_TYPE_UNITLESS_PERCENT
        && sensor_type != SENSOR_TYPE_COLOR
    );

    for(unsigned i = 0; i < 127; i++)
        graph_data[i] = 0.0f;
};

void SensorView::process_navigation(
    unsigned long button_select_press_duration,
    bool button_up_clicked,
    bool button_down_clicked
) {
    if(graph_screen && (button_up_clicked || button_down_clicked)) {
            if(button_up_clicked && button_down_clicked) {
                // clear
                if(multi_axis) {
                    memset(graph_data, 0, sizeof(graph_data));
                    graph_data_pos = 0;
                    axis++;
                    axis %= 3;
                }
            } else if(button_down_clicked) {
                if(sample_interval > 50)
                    sample_interval -= 10;
            } else {
                sample_interval += 10;
            }
    }

    if(button_select_press_duration > 50) {
        graph_screen = !graph_screen;
        request_redraw();
    }
}

void SensorView::draw(U8G2 &u8g2) {
    sensors_event_t event;

    if(!graph_screen) {
        sensor.getEvent(&event);
        float *data = event.data;

        u8g2.setFont(u8g2_font_glasstown_nbp_tf);
        const int FONT_HEIGHT = 12;
        int y = FONT_HEIGHT;

        u8g2.setCursor(0, y); u8g2.printf("Name: %s", sensor_name); y += FONT_HEIGHT;
        // u8g2.setCursor(0, y); u8g2.printf("ID: %d", event.sensor_id); y += FONT_HEIGHT;
        // u8g2.setCursor(0, y); u8g2.printf("TS: %d", event.timestamp); y += FONT_HEIGHT;

        u8g2.setCursor(0, y);
        u8g2.printf("Type: %s", get_sensor_type_string(sensor_type)); y += FONT_HEIGHT;
        u8g2.setCursor(0, y);

        if(!multi_axis) {
            u8g2.setCursor(0, y);
            u8g2.printf("Val: %.2f %s", data[0], get_sensor_unit_string(sensor_type));
            y += FONT_HEIGHT;
        } else {
            u8g2.setCursor(0, y);
            u8g2.printf("Unit: %s", get_sensor_unit_string(sensor_type));
            y += FONT_HEIGHT;
            u8g2.setCursor(0, y);
            u8g2.printf(
                "Val: x.%.2f y.%.2f z.%.2f",
                data[0],
                data[1],
                data[2]
            );
            y += FONT_HEIGHT;
        }
    } else {
        if(millis() - last_sampling_ts >= sample_interval) {
            sensor.getEvent(&event);
            graph_data[graph_data_pos++] = event.data[axis];
            graph_data_pos %= 127;
            last_sampling_ts = millis();
        }
        u8g2.setFont(u8g2_font_u8glib_4_tf);
        const int FONT_HEIGHT = 6;
        if(!multi_axis) {
            u8g2.setCursor(0, FONT_HEIGHT);
            u8g2.printf("%s | %dms", get_sensor_type_string(sensor_type), sample_interval);
        } else {
            u8g2.setCursor(0, FONT_HEIGHT);
            u8g2.printf(
                "%s, %c | %dms",
                get_sensor_type_string(sensor_type),
                axis == 0 ? 'x' : axis == 1 ? 'y' : 'z',
                sample_interval
            );
        }
        u8g2.setCursor(3, FONT_HEIGHT * 2);
        u8g2.print(get_sensor_unit_string(sensor_type));
        u8g2.drawVLine(0, FONT_HEIGHT + 1, 64 - FONT_HEIGHT - 2);
        u8g2.drawHLine(0, 63, 128);

        float local_max = min_sensor_value;
        float local_min = max_sensor_value;

        unsigned local_max_id = 0;
        unsigned local_min_id = 0;

        for(unsigned i = 0; i < 127; i++) {
            if(graph_data[i] > local_max) {
                local_max = graph_data[i];
                local_max_id = i;
            }
            if(graph_data[i] < local_min) {
                local_min = graph_data[i];
                local_min_id = i;
            }
        }

        float draw_max = max_sensor_value;
        float draw_min = min_sensor_value;

        if(auto_scaling) {
            draw_max = local_max;
            draw_min = local_min;

            float range = local_max - local_min;
            if(range < 0.1f) range = 1.0f;

            if(draw_max - draw_min < 0.001f) {
                draw_max += range * 0.1f;
                draw_min -= range * 0.1f;
            }
        }

        bool max_label_drawn = false;
        bool min_label_drawn = false;

        for(unsigned i = 0; i < 126; i++) {
            int current_idx = (graph_data_pos + i) % 127;
            int next_idx = (graph_data_pos + i + 1) % 127;

            int y1 = mapf(
                graph_data[current_idx],
                draw_min,
                draw_max,
                63 - FONT_HEIGHT - 1,
                FONT_HEIGHT * 3 + 1
            );
            int y2 = mapf(
                graph_data[next_idx],
                draw_min,
                draw_max,
                63 - FONT_HEIGHT - 1,
                FONT_HEIGHT * 3 + 1
            );

            char buf[16];

            if(!max_label_drawn && current_idx == local_max_id) {
                snprintf(buf, sizeof(buf), "%.2f", local_max);
                int strw = u8g2.getStrWidth(buf);

                int xp = i + 1 - strw / 2;
                xp = xp < 127 - strw ? xp : 127 - strw;
                xp = xp <= 1 ? 2 : xp;
                u8g2.setCursor(xp, y1 - 2);
                u8g2.print(buf);
                max_label_drawn = true;
            } else if(!min_label_drawn && current_idx == local_min_id) {
                snprintf(buf, sizeof(buf), "%.2f", local_min);
                int strw = u8g2.getStrWidth(buf);

                int xp = i + 1 - strw / 2;
                xp = xp < 127 - strw ? xp : 127 - strw;
                xp = xp <= 1 ? 2 : xp;
                u8g2.setCursor(xp, y1 + FONT_HEIGHT);
                u8g2.print(buf);
                min_label_drawn = true;
            }

            u8g2.drawLine(i + 1, y1, i + 2, y2);
        }
    }

    // redraw_request = false;
}

bool SensorView::prevent_sleep() {
    return true;
}
