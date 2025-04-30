#include "command_handler.h"
#include "../websocket/websocket_client.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <boost/asio.hpp>

namespace net = boost::asio;

CommandHandler::CommandHandler(std::shared_ptr<WebSocketClient> client, net::io_context& ioc)
    : client_(std::move(client)), ioc_(ioc) {
}

void CommandHandler::process_command(const std::string& command) {
    if (command.empty()) {
        std::cout << "> " << std::flush;  
        return;
    }

    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;

    if (cmd == "help" || cmd == "?") {
        print_help();
        std::cout << "> " << std::flush;
    } 
    else if (cmd == "connect") {
        std::string host, port;
        iss >> host >> port;

        if (host.empty() || port.empty()) {
            std::cout << "Usage: connect <host> <port>\n";
            std::cout << "> " << std::flush;
            return;
        }

        if (client_->is_connected()) {
            std::cout << "Already connected. Use 'close' to disconnect first.\n";
            std::cout << "> " << std::flush;
            return;
        }

        if (host.find("wss://") == 0) host = host.substr(6);
        else if (host.find("ws://") == 0) host = host.substr(5);

        std::cout << "Connecting to " << host << ":" << port << "...\n";
        client_->connect(host, port);
        std::cout << "> " << std::flush;
    } 
    else if (cmd == "send") {
        std::string message;
        std::getline(iss >> std::ws, message);

        if (message.empty()) {
            std::cout << "Usage: send <message>\n";
            std::cout << "> " << std::flush;
            return;
        }

        if (!client_->is_connected()) {
            std::cout << "Not connected. Use 'connect' first.\n";
            std::cout << "> " << std::flush;
            return;
        }

        client_->send(message, false);  
        std::cout << "> " << std::flush;
    } 
    else if (cmd == "sendbin") {
        std::string message;
        std::getline(iss >> std::ws, message);
    
        if (message.empty()) {
            std::cout << "Usage: sendbin <message> (or 'test' for a binary example)\n";
            std::cout << "> " << std::flush;
            return;
        }
    
        if (!client_->is_connected()) {
            std::cout << "Not connected. Use 'connect' first.\n";
            std::cout << "> " << std::flush;
            return;
        }

        client_->send(message, true);
        std::cout << "Sent binary: " << message << "\n";
        std::cout << "> " << std::flush;
    }
    else if (cmd == "close") {
        if (!client_->is_connected()) {
            std::cout << "No active connection to close.\n";
            std::cout << "> " << std::flush;
            return;
        }

        std::cout << "Closing connection...\n";
        client_->close();
        std::cout << "> " << std::flush;
    } 
    else if (cmd == "exit" || cmd == "quit") {
        if (client_->is_connected()) {
            std::cout << "Closing active connection before exiting...\n";
            client_->close();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::cout << "Exiting application.\n";
        std::cout << "> " << std::flush; 
        exit(0);
    } 
    else {
        std::cout << "Unknown command: " << cmd << "\n";
        print_help();
        std::cout << "> " << std::flush;
    }
}

void CommandHandler::run_command_loop() {
    print_help();
    std::cout << "> " << std::flush;

    while (true) {
        std::string command;
        if (!std::getline(std::cin, command)) {
            std::cout << "\nInput error. Exiting...\n";
            break;
        }
        process_command(command);
    }

    if (client_->is_connected()) {
        std::cout << "Closing active connection...\n";
        client_->close();
    }
}

void CommandHandler::print_help() const {
    std::cout << "Available commands:\n"
              << "  connect <host> <port>  - Connect to a WebSocket server\n"
              << "  send <message>         - Send a text message to the server\n"
              << "  sendbin <message>      - Send a binary message to the server\n"
              << "  close                  - Close the connection\n"
              << "  help                   - Show this help message\n"
              << "  exit                   - Exit the application\n";
}