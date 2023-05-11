#include <iostream>
#include <cstdint>
#include <unistd.h>

#include "server.hpp"

constexpr std::uint16_t server_port = 8000;

int main(void)
{
  server::Socket socket(server_port);
  socket.Listen([](int remote_socket)
                { 
                  // TODO: handle in http server module with thread poll
                  std::cout << "connected" << std::endl; 
                  std::cout << "socket_fd: "<<remote_socket<<std::endl; 
                  close(remote_socket); });
  return EXIT_SUCCESS;
}