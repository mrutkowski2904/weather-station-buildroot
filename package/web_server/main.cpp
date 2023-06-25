#include <iostream>
#include <cstdint>
#include <unistd.h>

#include "server.hpp"
#include "handler.hpp"

constexpr std::uint16_t server_port = 8000;

int main(void)
{
  server::Socket server_socket(server_port);
  server::HttpServer http(std::move(server_socket));

  http.RegisterHandler("/temperature", handler::GetTemperature);
  http.RegisterHandler("/humidity", handler::GetHumidity);
  http.RegisterHandler("/pressure", handler::GetPressure);

  http.Run();
  return EXIT_SUCCESS;
}