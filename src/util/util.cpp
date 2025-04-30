#include "util.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <random>
#include <openssl/sha.h>
#include <stdexcept>  
#include <atomic>   

namespace util {

std::string trim(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(), ::isspace);
    auto end = std::find_if_not(str.rbegin(), str.rend(), ::isspace).base();
    return (start < end) ? std::string(start, end) : std::string();
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(std::move(token));
    }

    return tokens;
}

std::string generateWebSocketKey() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned char> dis(0, 255);

    std::string key(16, '\0');
    for (auto& byte : key) {
        byte = dis(gen);
    }

    return base64Encode(key);
}

std::string computeAcceptKey(const std::string& key) {
    const std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string combined = key + magic;

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(combined.c_str()), combined.size(), hash);

    return base64Encode(std::string(reinterpret_cast<char*>(hash), SHA_DIGEST_LENGTH));
}

static std::atomic<LogLevel> currentLogLevel{LogLevel::LOG_INFO};

void setLogLevel(LogLevel level) {
    currentLogLevel.store(level, std::memory_order_relaxed);
}

void log(LogLevel level, const std::string& message) {
    if (level < currentLogLevel.load(std::memory_order_relaxed)) return;

    std::string prefix;
    switch (level) {
        case LogLevel::LOG_DEBUG:   prefix = "[DEBUG] "; break;
        case LogLevel::LOG_INFO:    prefix = "[INFO] ";  break;
        case LogLevel::LOG_WARNING: prefix = "[WARN] ";  break;
        case LogLevel::LOG_ERROR:   prefix = "[ERROR] "; break;
    }

    std::cout << prefix << message << std::endl;
}

static const std::string base64_chars = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string base64Encode(const std::string& data) {
    std::string ret;
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    for (char c : data) {
        char_array_3[i++] = c;
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (int j = 0; j < 4; j++) {
                ret += base64_chars[char_array_4[j]];
            }
            i = 0;
        }
    }

    if (i) {
        for (int j = i; j < 3; j++) char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2);

        for (int j = 0; j < i + 1; j++) ret += base64_chars[char_array_4[j]];
        while (i++ < 3) ret += '=';
    }

    return ret;
}

std::string base64Decode(const std::string& encoded_data) {
    if (encoded_data.size() % 4 != 0) {
        throw std::invalid_argument("Invalid Base64 input length");
    }

    std::string ret;
    unsigned char char_array_4[4], char_array_3[3];
    int i = 0;

    for (char c : encoded_data) {
        if (c == '=') break;

        char_array_4[i++] = static_cast<unsigned char>(base64_chars.find(c));
        if (i == 4) {
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            ret.append(reinterpret_cast<char*>(char_array_3), 3);
            i = 0;
        }
    }

    if (i) {
        for (int j = 0; j < i; j++) char_array_4[j] = static_cast<unsigned char>(base64_chars.find(char_array_4[j]));
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        ret.append(reinterpret_cast<char*>(char_array_3), 1);
    }

    return ret;
}

UrlParts parseWebSocketUrl(const std::string& url) {
    UrlParts parts{};
    const std::string ws_prefix = "ws://";
    const std::string wss_prefix = "wss://";

    if (url.compare(0, ws_prefix.size(), ws_prefix) == 0) {
        parts.secure = false;
    } else if (url.compare(0, wss_prefix.size(), wss_prefix) == 0) {
        parts.secure = true;
    } else {
        throw std::invalid_argument("Invalid WebSocket URL scheme");
    }

    return parts;
}

} 
