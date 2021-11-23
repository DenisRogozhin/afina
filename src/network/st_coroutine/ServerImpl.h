#ifndef AFINA_NETWORK_ST_COROUTINE_SERVER_H
#define AFINA_NETWORK_ST_COROUTINE_SERVER_H

#include <atomic>
#include <set>
#include <thread>

#include <memory>
#include <stdexcept>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <afina/network/Server.h>
#include <afina/coroutine/Engine.h>

namespace spdlog {
class logger;
}

namespace Afina {
namespace Network {
namespace STcoroutine {

/**
 * # Network resource manager implementation
 * Server that is spawning a separate thread for each connection
 */
class ServerImpl : public Server {
public:
    ServerImpl(std::shared_ptr<Afina::Storage> ps, std::shared_ptr<Logging::Service> pl);
    ~ServerImpl();

    // See Server.h
    void Start(uint16_t port, uint32_t, uint32_t) override;

    // See Server.h
    void Stop() override;

    // See Server.h
    void Join() override;

protected:
    /**
     * Method is running in the connection acceptor thread
     */
    void OnRun();

private:

    void coro_Start();

    int coro_accept(int server_socket, struct sockaddr client_addr, socklen_t client_addr_len);

    int coro_read(int client_socket,  char * client_buffer);
   
    int coro_send(int client_socket,  std::string & result);
   
    void unblocker();

    void work_cycle(int client_socket);

    // Logger instance
    std::shared_ptr<spdlog::logger> _logger;

    // Atomic flag to notify threads when it is time to stop. Note that
    // flag must be atomic in order to safely publisj changes cross thread
    // bounds
    std::atomic<bool> running;

    // Server socket to accept connections on
    int _server_socket;

    std::set<int> sockets;

    int _event_fd;

    int epoll_descr;

    std::thread _work_thread;

    Afina::Coroutine::Engine engine;

    std::array<struct epoll_event, 64> mod_list;
};

} // namespace STcoroutine
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_ST_COROUTINE_SERVER_H
