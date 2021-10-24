#include "Connection.h"

#include <iostream>

namespace Afina {
namespace Network {
namespace STnonblock {

// See Connection.h
void Connection::Start() {
	_event.events |= EPOLLIN;
	head_offset = 0;  
}

// See Connection.h
void Connection::OnError() { 
	isalive = false;
}

// See Connection.h
void Connection::OnClose() {
	isalive = false; 
}

// See Connection.h
void Connection::DoRead() { 
	try {
            int readed_bytes = -1;
            while ((readed_bytes = read(_socket, client_buffer, sizeof(client_buffer))) > 0) {
                _logger->debug("Got {} bytes from socket", readed_bytes);

                // Single block of data readed from the socket could trigger inside actions a multiple times,
                // for example:
                // - read#0: [<command1 start>]
                // - read#1: [<command1 end> <argument> <command2> <argument for command 2> <command3> ... ]
                while (readed_bytes > 0) {
                    _logger->debug("Process {} bytes", readed_bytes);
                    // There is no command yet
                    if (!command_to_execute) {
                        std::size_t parsed = 0;
                        if (parser.Parse(client_buffer, readed_bytes, parsed)) {
                            // There is no command to be launched, continue to parse input stream
                            // Here we are, current chunk finished some command, process it
                            _logger->debug("Found new command: {} in {} bytes", parser.Name(), parsed);
                            command_to_execute = parser.Build(arg_remains);
                            if (arg_remains > 0) {
                                arg_remains += 2;
                            }
                        }

                        // Parsed might fails to consume any bytes from input stream. In real life that could happens,
                        // for example, because we are working with UTF-16 chars and only 1 byte left in stream
                        if (parsed == 0) {
                            break;
                        } else {
                            std::memmove(client_buffer, client_buffer + parsed, readed_bytes - parsed);
                            readed_bytes -= parsed;
                        }
                    }

                    // There is command, but we still wait for argument to arrive...
                    if (command_to_execute && arg_remains > 0) {
                        _logger->debug("Fill argument: {} bytes of {}", readed_bytes, arg_remains);
                        // There is some parsed command, and now we are reading argument
                        std::size_t to_read = std::min(arg_remains, std::size_t(readed_bytes));
                        argument_for_command.append(client_buffer, to_read);

                        std::memmove(client_buffer, client_buffer + to_read, readed_bytes - to_read);
                        arg_remains -= to_read;
                        readed_bytes -= to_read;
                    }

                    // Thre is command & argument - RUN!
                    if (command_to_execute && arg_remains == 0) {
                        _logger->debug("Start command execution");

                        std::string result;
                        if (argument_for_command.size()) {
                            argument_for_command.resize(argument_for_command.size() - 2);
                        }
                        command_to_execute->Execute(*pStorage, argument_for_command, result);

                        // Send response
                        result += "\r\n";
                        outgoing.emplace_back(result);
			_event.events |= EPOLLOUT;
				

                        // Prepare for the next command
                        command_to_execute.reset();
                        argument_for_command.resize(0);
                        parser.Reset();
                    }
                } // while (readed_bytes)
            }

            if (readed_bytes == 0) {
		 isalive = false;
                _logger->debug("Connection closed");	     
             } else { //readed_bytes = -1
			if (errno != EWOULDBLOCK) {	
				isalive = false;
                		throw std::runtime_error(std::string(strerror(errno)));
               		}
		}
        } catch (std::runtime_error &ex) {
            _logger->error("Failed to process connection on descriptor {}: {}", _socket, ex.what());
        } 
}

// See Connection.h
void Connection::DoWrite() { 
	while (!outgoing.empty()) {
		auto const & b = outgoing.front();
		while (head_offset < b.size()) {
			int n = write(_socket, &b[head_offset], b.size() - head_offset);
			if ((n > 0) && (n <= b.size() - head_offset)) {
				head_offset += n;	
				continue;
			}
			else if (errno == EWOULDBLOCK) {
				return;		
			}
			else {
				isalive = false;
			}
		}
		outgoing.pop_front();
		head_offset = 0;
	}
	_event.events &= ~EPOLLOUT;
	// если не добавить строчку снизу, то когда я подключаюсь к серверу с командой:
	// echo -n -e "set foo 0 0 6\r\nfooval\r\n" | nc localhost 8080
	// то после ее выполнения клиент не завершается, а продолжает висеть:(
	// не очень понимаю, почему, кажется, что она тут не нужна
	isalive = false;
}

} // namespace STnonblock
} // namespace Network
} // namespace Afina
