#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "../src/websocket/websocket_client.h"
#include "../src/util/root_certificates.hpp"
#include <chrono>
#include <thread>
#include <deque>

namespace net = boost::asio;
namespace ssl = boost::asio::ssl;

class WebSocketClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        ctx_ = std::make_unique<ssl::context>(ssl::context::tlsv13_client);
        load_root_certificates(*ctx_);
        ctx_->set_verify_mode(ssl::verify_peer);
        client_ = std::make_shared<WebSocketClient>(ioc_, *ctx_);
    }

    void TearDown() override {
        if (client_ && client_->is_connected()) {
            client_->close();
            ioc_.run_for(std::chrono::seconds(1));
        }
        ioc_.stop();
        ioc_.restart();
    }

    bool WaitForCondition(std::function<bool()> condition, int timeout_ms = 10000) {
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(timeout_ms)) {
            ioc_.run_one();
            if (condition()) return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return condition();
    }

    net::io_context ioc_;
    std::unique_ptr<ssl::context> ctx_;
    std::shared_ptr<WebSocketClient> client_;
    std::deque<std::string> received_messages_;
};

TEST_F(WebSocketClientTest, Construction) {
    EXPECT_FALSE(client_->is_connected());
}

TEST_F(WebSocketClientTest, ConnectSuccess) {
    client_->connect("echo.websocket.org", "443");
    EXPECT_TRUE(WaitForCondition([this]() { return client_->is_connected(); })) << "Failed to connect";
}

TEST_F(WebSocketClientTest, SendTextAndReceive) {
    client_->set_message_callback([this](const std::string& message) {
        received_messages_.push_back(message);
    });

    client_->connect("echo.websocket.org", "443");
    ASSERT_TRUE(WaitForCondition([this]() { return client_->is_connected(); })) << "Connection failed";

    
    ioc_.run_for(std::chrono::seconds(1));
    received_messages_.clear(); 

    std::string test_message = "Hello, WebSocket!";
    client_->send(test_message, false);
    EXPECT_TRUE(WaitForCondition([this, test_message]() {
        return !received_messages_.empty() && received_messages_.back() == test_message;
    })) << "Did not receive echo, last message: " << (received_messages_.empty() ? "none" : received_messages_.back());
}

TEST_F(WebSocketClientTest, SendBinaryAndReceive) {
    client_->set_message_callback([this](const std::string& message) {
        received_messages_.push_back(message);
    });

    client_->connect("echo.websocket.org", "443");
    ASSERT_TRUE(WaitForCondition([this]() { return client_->is_connected(); })) << "Connection failed";

    ioc_.run_for(std::chrono::seconds(1));
    received_messages_.clear();

    std::string binary_message = std::string("\x01\x02\x03", 3);
    client_->send(binary_message, true);
    EXPECT_TRUE(WaitForCondition([this, binary_message]() {
        return !received_messages_.empty() && received_messages_.back() == binary_message;
    })) << "Did not receive echo, last message: " << (received_messages_.empty() ? "none" : received_messages_.back());
}

TEST_F(WebSocketClientTest, CloseConnection) {
    client_->connect("echo.websocket.org", "443");
    ASSERT_TRUE(WaitForCondition([this]() { return client_->is_connected(); })) << "Connection failed";
    client_->close();
    EXPECT_TRUE(WaitForCondition([this]() { return !client_->is_connected(); })) << "Failed to close";
}

TEST_F(WebSocketClientTest, SendWithoutConnection) {
    client_->send("test", false);
    EXPECT_FALSE(client_->is_connected());
}

TEST_F(WebSocketClientTest, ConnectFailure) {
    client_->connect("invalid.websocket.server", "443");
    EXPECT_FALSE(WaitForCondition([this]() { return client_->is_connected(); }, 2000)) << "Should not connect";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}