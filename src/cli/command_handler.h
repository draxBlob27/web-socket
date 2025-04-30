#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "../websocket/websocket_client.h"
#include <memory>
#include <boost/asio.hpp>

namespace net = boost::asio;

class CommandHandler {
public:
    CommandHandler(std::shared_ptr<WebSocketClient> client, net::io_context& ioc);
    void process_command(const std::string& command);
    void run_command_loop();
    void print_help() const;

private:
    std::shared_ptr<WebSocketClient> client_;
    [[maybe_unused]] net::io_context& ioc_; 
};

#endif