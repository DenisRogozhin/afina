#ifndef AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H

#include <cstring>
#include <deque>
#include <memory>
#include <string>
#include <sys/epoll.h>
#include <afina/execute/Command.h>
#include <protocol/Parser.h>
#include <spdlog/logger.h>

namespace spdlog {
class logger;
}

namespace Afina {
namespace Network {
namespace STnonblock {

class Connection {
public:
    Connection(int s, std::shared_ptr<spdlog::logger> logger, std::shared_ptr<Afina::Storage> pstorage) : _socket(s) {
        std::memset(&_event, 0, sizeof(struct epoll_event));
        _event.data.ptr = this;
        isalive = true;
        _logger = logger;
        pStorage = pstorage;
    }

    inline bool isAlive() const { return isalive; }

    void Start();

protected:
    void OnError();
    void OnClose();
    void DoRead();
    void DoWrite();

private:
    friend class ServerImpl;

    std::shared_ptr<Afina::Storage> pStorage;
    std::shared_ptr<spdlog::logger> _logger;
    std::string argument_for_command;
    std::unique_ptr<Execute::Command> command_to_execute;
    std::size_t arg_remains = 0;
    Protocol::Parser parser;

    bool isalive;
    int _socket;
    char client_buffer[4096];
    struct epoll_event _event;
    int head_offset;
    std::deque<std::string> outgoing;
};

} // namespace STnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
