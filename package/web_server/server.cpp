#include <iostream>
#include <thread>
#include <mutex>
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

  this->socket_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (this->socket_fd <= 0)
  {
    std::cerr << "socket creation: " << std::endl;
    exit(errno);
  }
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  bind(this->socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
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
  event.data.fd = this->socket_fd;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, this->socket_fd, &event);
  struct epoll_event epoll_events[max_epoll_events];

  if (listen(this->socket_fd, server::max_connections))
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
        client_socket = accept(this->socket_fd, &remote_addr, &remote_addr_len);
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
  close(socket_fd);
}

void server::Socket::Close()
{
  this->stop = true;
}

// ==============================
//   HTTP SERVER IMPLEMENTATION
// ==============================

server::HttpServer::HttpServer(server::Socket &&server_socket) : socket(server_socket)
{
}

void server::HttpServer::Run()
{
  std::function<void(int)> connection_handler = std::bind(
      &HttpServer::HandleConnection,
      this,
      std::placeholders::_1);

  this->socket.Listen(connection_handler);
}

void server::HttpServer::RegisterHandler(std::string path, std::function<std::string()> handler)
{
  this->path_handlers_mutex.lock();
  this->path_handlers[path] = handler;
  this->path_handlers_mutex.unlock();
}

void server::HttpServer::HandleConnection(int remote_socket)
{
  this->threads_mutex.lock();
  std::function<void(int)> conn_thread = std::bind(&HttpServer::ConnectionThread, this, std::placeholders::_1);
  this->threads.push_back(std::thread(conn_thread, remote_socket));
  this->threads_mutex.unlock();
}

void server::HttpServer::ConnectionThread(int remote_socket)
{
  // receive data
  std::string request_data;
  char request_buffer[request_buffer_size];
  memset(request_buffer, 0, request_buffer_size);
  ssize_t request_size = recv(remote_socket, request_buffer, request_buffer_size, 0);
  if (request_size == -1)
  {
    ConenctionThreadExit(remote_socket);
    return;
  }
  request_data.assign(request_buffer);

  // get requested path
  std::pair<bool, std::string> path = GetRequestPath(request_data);
  if (!path.first)
  {
    // invalid HTTP header
    ConenctionThreadExit(remote_socket);
    return;
  }

  // find handler or return 404
  this->path_handlers_mutex.lock();
  auto handler = this->path_handlers.find(path.second);
  this->path_handlers_mutex.unlock();

  // call user handler
  if (handler != this->path_handlers.end())
  {
    std::string user_response = handler->second();

    // add http header
    std::string header = CreateHttpHeader();
    user_response = header + user_response;

    // send response
    send(remote_socket, user_response.c_str(), user_response.length(), 0);
  }
  else
  {
    // no handler
    // TODO: implement not found handler
  }

  ConenctionThreadExit(remote_socket);
}

void server::HttpServer::ConenctionThreadExit(int remote_socket)
{
  // close remote socket (connection)
  close(remote_socket);

  // remove this thread from threads
  std::thread::id this_id = std::this_thread::get_id();
  this->threads_mutex.lock();
  auto threads_iter = std::find_if(this->threads.begin(), this->threads.end(), [=](std::thread &current_thread)
                                   { return current_thread.get_id() == this_id; });

  if (threads_iter != this->threads.end())
  {
    threads_iter->detach();
    this->threads.erase(threads_iter);
  }
  this->threads_mutex.unlock();
}

std::string server::HttpServer::CreateHttpHeader()
{
  return "HTTP/1.1 200 OK\r\n\n";
}

std::pair<bool, std::string> server::HttpServer::GetRequestPath(std::string &request_data)
{
  std::size_t whitespace_no1 = request_data.find(' ');
  if (whitespace_no1 == std::string::npos)
    return std::make_pair(false, "");
  request_data = request_data.substr(whitespace_no1 + 1, request_data.length());

  std::size_t whitespace_no2 = request_data.find(' ');
  if (whitespace_no2 == std::string::npos)
    return std::make_pair(false, "");

  request_data = request_data.substr(0, whitespace_no2);
  return std::make_pair(true, request_data);
}