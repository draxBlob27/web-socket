#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "websocket/websocket_client.h"
#include "util/root_certificates.hpp"
#include "cli/command_handler.h"

namespace net = boost::asio;
namespace ssl = boost::asio::ssl;

int main(int argc, char** argv) {
    try {
        net::io_context ioc;
        ssl::context ctx{ssl::context::tlsv13_client};
        load_root_certificates(ctx);
        ctx.set_verify_mode(ssl::verify_peer);

        // Work guard to keep io_context alive
        auto work_guard = net::make_work_guard(ioc);

        auto client = std::make_shared<WebSocketClient>(ioc, ctx);

        client->set_message_callback([](const std::string& message) {
            std::cout << "Received: " << message << std::endl;
        });

        CommandHandler handler(client, ioc);

        if (argc >= 3) {
            std::string host = argv[1];
            std::string port = argv[2];
            if (host.find("wss://") == 0) host = host.substr(6);
            else if (host.find("ws://") == 0) host = host.substr(5);

            std::cout << "Connecting to " << host << ":" << port << "...\n";
            client->connect(host, port);

            ioc.run_for(std::chrono::seconds(5));
            auto start = std::chrono::steady_clock::now();
            while (!client->is_connected() && std::chrono::steady_clock::now() - start < std::chrono::seconds(5)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                ioc.poll();
            }

            if (client->is_connected() && argc >= 4) {
                std::string message = argv[3];
                std::cout << "Sending: " << message << std::endl;
                client->send(message);
                ioc.run_for(std::chrono::seconds(2));
            } else if (argc >= 4) {
                std::cerr << "Error: Unable to send message. Not connected.\n";
            }
        } else {
            std::cout << "Usage: " << argv[0] << " <host> <port> [message]\n";
            std::cout << "Starting in interactive mode...\n";
        }

        std::thread ioc_thread([&ioc]() {
            try {
                std::cout << "io_context thread started.\n";
                ioc.run();
                std::cout << "io_context thread finished.\n";
            } catch (const std::exception& e) {
                std::cerr << "I/O error: " << e.what() << std::endl;
            }
        });

        if (ioc_thread.joinable()) {
            std::cout << "io_context thread launched.\n";
        }

        handler.run_command_loop();

        std::cout << "Stopping io_context...\n";
        work_guard.reset();  
        ioc.stop();
        if (ioc_thread.joinable()) {
            ioc_thread.join();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}