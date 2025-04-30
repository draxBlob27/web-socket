#include "websocket_client.h"
#include <iostream>

void fail(beast::error_code ec, const char* what) {
    std::cerr << "Error in " << what << ": " << ec.message() << std::endl;
}

WebSocketClient::WebSocketClient(net::io_context& ioc, ssl::context& ctx)
    : resolver_(ioc), ws_(ioc, ctx), ctx_(ctx) {  
    load_root_certificates(ctx_);
    std::cout << "WebSocketClient constructed.\n";
}

WebSocketClient::~WebSocketClient() {
    if (is_connected_) {
        beast::error_code ec;
        ws_.close(websocket::close_code::normal, ec);
        if (ec) fail(ec, "close during destruction");
    }
}

void WebSocketClient::connect(const std::string& host, const std::string& port) {
    if (is_connecting_) {
        std::cerr << "Connection already in progress. Please wait or close the current connection attempt." << std::endl;
        return;
    }
    
    is_connecting_ = true;
    host_ = host;
    std::cout << "Resolving host: " << host_ << ":" << port << std::endl;

    if (!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host.c_str())) {
        fail({static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()}, "SSL set host name");
        is_connecting_ = false;  
        return;
    }

    ws_.next_layer().set_verify_mode(ssl::verify_peer);
    ws_.next_layer().set_verify_callback(ssl::host_name_verification(host));

    std::cout << "Starting async_resolve...\n";  
    resolver_.async_resolve(host, port, beast::bind_front_handler(&WebSocketClient::on_resolve, shared_from_this()));
    std::cout << "async_resolve called.\n";  
}

void WebSocketClient::send(const std::string& message, bool is_binary) {
    if (!is_connected_) {
      std::cerr << "Cannot send message: Not connected." << std::endl;
      return;
    }
    text_ = message;
    ws_.binary(is_binary);
    ws_.async_write(net::buffer(text_), beast::bind_front_handler(&WebSocketClient::on_write, shared_from_this()));
  }

void WebSocketClient::close() {
    if (!is_connected_) return;

    std::cout << "Closing connection..." << std::endl;
    is_connected_ = false;
    ws_.async_close(websocket::close_code::normal, beast::bind_front_handler(&WebSocketClient::on_close, shared_from_this()));
}

bool WebSocketClient::is_connected() const {
    return is_connected_;
}

void WebSocketClient::set_message_callback(MessageCallback callback) {
    message_callback_ = std::move(callback);
}

void WebSocketClient::on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
    std::cout << "In on_resolve...\n";  
    if (ec) {
        is_connecting_ = false;
        fail(ec, "resolve");
        return;
    }

    std::cout << "Resolved host. Found " << results.size() << " endpoints. Attempting connection..." << std::endl;
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));
    beast::get_lowest_layer(ws_).async_connect(results, beast::bind_front_handler(&WebSocketClient::on_connect, shared_from_this()));
}

void WebSocketClient::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type endpoint) {
    std::cout << "In on_connect...\n"; 
    if (ec) {
        is_connecting_ = false;
        fail(ec, "connect");
        return;
    }

    std::cout << "Connected to: " << endpoint.address().to_string() << ":" << endpoint.port() << std::endl;
    host_ += ':' + std::to_string(endpoint.port());

    std::cout << "Performing SSL handshake..." << std::endl;
    ws_.next_layer().async_handshake(ssl::stream_base::client, beast::bind_front_handler(&WebSocketClient::on_ssl_handshake, shared_from_this()));
}

void WebSocketClient::on_ssl_handshake(beast::error_code ec) {
    if (ec) {
        fail(ec, "ssl_handshake");
        std::cerr << "SSL Error: " << ERR_reason_error_string(ERR_get_error()) << std::endl;
        return;
    }

    std::cout << "SSL handshake successful. Performing WebSocket handshake..." << std::endl;
    ws_.async_handshake(host_, "/", beast::bind_front_handler(&WebSocketClient::on_handshake, shared_from_this()));
}

void WebSocketClient::on_handshake(beast::error_code ec) {
    is_connecting_ = false;
    if (ec) {
        fail(ec, "handshake");
        return;
    }

    std::cout << "Handshake successful. Connected!" << std::endl;
    is_connected_ = true;

    ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
    do_read();
}

void WebSocketClient::on_write(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if (ec) {
        fail(ec, "write");
        return;
    }
    std::cout << "Message sent successfully." << std::endl;
}

void WebSocketClient::do_read() {
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));
    ws_.async_read(buffer_, beast::bind_front_handler(&WebSocketClient::on_read, shared_from_this()));
}

void WebSocketClient::on_read(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if (ec == websocket::error::closed) {
        std::cout << "Server closed the connection: " << ws_.reason().reason << std::endl;
        is_connected_ = false;
        return;
    } else if (ec) {
        fail(ec, "read");
        return;
    }

    std::string message = beast::buffers_to_string(buffer_.data());
    buffer_.consume(buffer_.size());

    if (message_callback_) {
        message_callback_(message);
    } else {
        std::cout << "Received: " << message << std::endl;
    }

    do_read();
}

void WebSocketClient::on_close(beast::error_code ec) {
    if (ec) {
        fail(ec, "close");
    } else {
        std::cout << "Connection closed gracefully. Reason: " << ws_.reason().reason << std::endl;
    }
    is_connected_ = false;
}