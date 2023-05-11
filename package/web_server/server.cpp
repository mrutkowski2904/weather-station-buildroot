#include <iostream>
#include <cassert>
#include <cstdlib>
#include <cerrno>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "server.hpp"

server::Socket::Socket(const std::uint16_t port)
{
  assert(port != 0);

  this->server_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (this->server_socket <= 0)
  {
    std::cerr << "socket creation: " << std::endl;
    exit(errno);
  }
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  bind(this->server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
}

void server::Socket::Listen(std::function<void(int)> on_connection)
{
  int client_socket, epoll_fd, in_conns;
  struct epoll_event event;
  sockaddr remote_addr;
  socklen_t remote_addr_len = sizeof(remote_addr);
  char remote_addr_str[INET_ADDRSTRLEN];

  epoll_fd = epoll_create(1);
  event.events = EPOLLIN;
  event.data.fd = this->server_socket;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, this->server_socket, &event);
  struct epoll_event epoll_events[max_epoll_events];

  if (listen(this->server_socket, server::max_connections))
  {
    std::cerr << "socket listen: " << std::endl;
    exit(errno);
  }

  while (!this->stop)
  {
    in_conns = epoll_wait(epoll_fd, epoll_events, server::max_epoll_events, server::epoll_timeout_ms);
    if (in_conns < 0)
    {
      std::cerr << "epoll_wait: " << std::endl;
      std::cout << std::strerror(errno) << std::endl;
      continue;
    }

    for (int i = 0; i < in_conns; i++)
    {
      if (epoll_events[i].events & EPOLLIN)
      {
        client_socket = accept4(this->server_socket, &remote_addr, &remote_addr_len, SOCK_NONBLOCK);
        if (client_socket != -1)
        {
          inet_ntop(AF_INET, remote_addr.sa_data, remote_addr_str, remote_addr_len);
          std::cout << "new connection from: " << remote_addr_str << std::endl;
          on_connection(client_socket);
        }
      }
    }
  }
  close(epoll_fd);
  close(server_socket);
}

void server::Socket::Close()
{
  this->stop = true;
}