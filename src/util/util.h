#ifndef WEBSOCKET_CLIENT_UTIL_H
#define WEBSOCKET_CLIENT_UTIL_H

#include <string>
#include <vector>
#include <cstdint>

namespace util {

// Trim whitespace from both ends of a string
std::string trim(const std::string& str);

// Split a string by a delimiter into a vector of substrings
std::vector<std::string> split(const std::string& str, char delimiter);

std::string generateWebSocketKey();

std::string computeAcceptKey(const std::string& key);

enum class LogLevel {
    LOG_DEBUG, 
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
};

void log(LogLevel level, const std::string& message);

void setLogLevel(LogLevel level);

std::string base64Encode(const std::string& data);

std::string base64Decode(const std::string& data);

struct UrlParts {
    bool secure;         
    std::string host;    // Hostname (e.g., "example.com")
    std::int32_t port;   // Port number (e.g., 443 for wss, 80 for ws)
    std::string path;    // Resource path (e.g., "/chat")
};

UrlParts parseWebSocketUrl(const std::string& url);

}  

#endif 
