#include "FS.h"
#include <data_logger.h>

File logfile;

File get_logfile() {
    return logfile = LittleFS.open("weather_log.csv", FILE_APPEND);
}
