#include <iostream>
#include <cstdint>
#include <unistd.h>

#include "server.hpp"

constexpr std::uint16_t server_port = 8000;

int main(void)
{
  server::Socket server_socket(server_port);
  server::HttpServer http(std::move(server_socket));
  return EXIT_SUCCESS;
}