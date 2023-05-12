#pragma once

#include <cstdint>
#include <functional>
#include <thread>
#include <map>
#include <vector>
#include <mutex>

namespace server
{
  constexpr int max_connections = 10;
  constexpr int max_epoll_events = 32;
  constexpr int epoll_timeout_ms = 40;
  constexpr size_t request_buffer_size = 1024;

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
    void Run();
    void RegisterHandler(std::string path, std::function<std::string()> handler);

  private:
    Socket socket;

    // path, handler pair
    std::unordered_map<std::string, std::function<std::string()>> path_handlers;
    std::mutex path_handlers_mutex;

    std::vector<std::thread> threads;
    std::mutex threads_mutex;

    void HandleConnection(int remote_socket);
    void ConnectionThread(int remote_socket);
    void ConenctionThreadExit(int remote_socket);
    static std::string CreateHttpHeader();
    static std::pair<bool, std::string> GetRequestPath(std::string &request_data);
  };
}
