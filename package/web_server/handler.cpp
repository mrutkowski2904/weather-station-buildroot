#include <iostream>
#include <sstream>
#include <fstream>

#include "handler.hpp"

std::string handler::GetTemperature(void)
{
    int temperature = handler::ReadDevice(std::string(temperature_path));
    return std::to_string(temperature / 1000);
}

std::string handler::GetHumidity(void)
{
    int humidity = handler::ReadDevice(std::string(humidity_path));
    return std::to_string(humidity / 1000);
}

std::string handler::GetPressure(void)
{
    int pressure = handler::ReadDevice(std::string(pressure_path));
    return std::to_string(pressure);
}

int handler::ReadDevice(std::string path)
{
    std::ifstream file_stream(path);
    std::stringstream str_stream;
    std::string data;
    str_stream << file_stream.rdbuf();
    data = str_stream.str();
    return data.length() ? std::stoi(data) : 0;
}