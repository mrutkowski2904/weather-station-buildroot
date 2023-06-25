#pragma once

#include <string>

namespace handler {
    std::string GetTemperature(void);
    std::string GetHumidity(void);
    std::string GetPressure(void);

    int ReadDevice(std::string path);

    constexpr std::string_view temperature_path = "/sys/bus/iio/devices/iio:device0/in_temp_input";
    constexpr std::string_view humidity_path = "/sys/bus/iio/devices/iio:device0/in_humidityrelative_input";
    constexpr std::string_view pressure_path = "/dev/barometer0";
};