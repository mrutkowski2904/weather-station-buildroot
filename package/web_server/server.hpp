#pragma once

#include <cstdint>
#include <functional>

namespace server
{
  constexpr int max_connections = 10;
  constexpr int max_epoll_events = 32;
  constexpr int epoll_timeout_ms = 40;

  struct Socket
  {
    Socket(const std::uint16_t port);
    void Listen(std::function<void(int)> on_connection);
    void Close();

  private:
    int socket_fd;
    bool stop = false;
  };

  struct HttpServer
  {
    HttpServer(server::Socket &&socket);

  private:
    bool IsValidHttpRequest();
    Socket socket;
  };
}
