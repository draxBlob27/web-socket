#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include "../util/root_certificates.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ssl.hpp>
#include <functional>
#include <memory>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

using MessageCallback = std::function<void(const std::string&)>;

class WebSocketClient : public std::enable_shared_from_this<WebSocketClient> {
private:
    tcp::resolver resolver_;  
    websocket::stream<ssl::stream<beast::tcp_stream>> ws_; 
    beast::flat_buffer buffer_;
    std::string host_;
    std::string text_;
    MessageCallback message_callback_;
    bool is_connected_ = false;
    bool is_connecting_ = false;
    ssl::context& ctx_;

public:
    explicit WebSocketClient(net::io_context& ioc, ssl::context& ctx);
    ~WebSocketClient();
    void connect(const std::string& host, const std::string& port);
    void send(const std::string& message, bool is_binary = false);
    void close();
    bool is_connected() const;
    void set_message_callback(MessageCallback callback);

private:
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type endpoint);
    void on_ssl_handshake(beast::error_code ec);
    void on_handshake(beast::error_code ec);
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_close(beast::error_code ec);
    void do_read();
};

#endif