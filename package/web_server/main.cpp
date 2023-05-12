#include <iostream>
#include <cstdint>
#include <unistd.h>

#include <sstream>
#include <fstream>

#include "server.hpp"

constexpr std::uint16_t server_port = 8000;

int main(void)
{
  server::Socket server_socket(server_port);
  server::HttpServer http(std::move(server_socket));

  http.RegisterHandler("/test", []
                       { return "test"; });

  http.RegisterHandler("/", []
                       {
    std::ifstream index("index.html");
    std::stringstream index_buffer;
    index_buffer << index.rdbuf();
      return index_buffer.str(); });

  http.Run();
  return EXIT_SUCCESS;
}