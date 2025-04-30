🔌 C++ WebSocket Client
A modern, feature-rich C++ WebSocket client built with a clean modular design and the GN (Generate Ninja) build system. This client supports secure connections, binary messages, and provides a user-friendly command-line interface for interacting with WebSocket servers.

✨ Features
WebSocket Connectivity

Connects to public WebSocket echo servers

Full support for both plain (ws://) and secure (wss://) connections

Command-Line Interface

Built using CLI11

Send and receive text messages interactively

Binary Message Support

Easily toggle between text and binary message formats

TLS/SSL Encryption

Secure communication using OpenSSL

Modular Architecture

Clean codebase with separate classes/files for WebSocket logic, CLI, and utilities

Cross-Compiler Support

Builds cleanly with both GCC and Clang

Unit Testing

Includes basic unit tests for critical modules

🛠️ Build System
This project uses GN for build configuration and Ninja for fast builds.

🔧 Requirements
GN (gn command-line tool)

Ninja (ninja build system)

C++17 compatible compiler (GCC or Clang)

OpenSSL development libraries

CMake (for tests)

📦 Building the Project
bash
Copy
Edit
# Generate build files
gn gen out/Debug --args='is_debug=true'
gn gen out/Release --args='is_debug=false'

# Build the binaries
ninja -C out/Debug
ninja -C out/Release
🧪 Running Unit Tests
bash
Copy
Edit
cd tests/
cmake .
make
./run_tests
🚀 Usage
bash
Copy
Edit
./websocket_client --server wss://echo.websocket.events/.ws --binary
CLI Options
--server <url>: WebSocket server URL (e.g., wss://echo.websocket.events/.ws)

--binary: Enable binary mode

--insecure: Disable SSL verification (use with caution)

--help: Show usage instructions

📁 Project Structure
bash
Copy
Edit
/src
  websocket_client.cpp   # Main WebSocket logic
  cli.cpp                # Command-line interface
  ssl_utils.cpp          # TLS helper functions
/tests
  websocket_tests.cpp    # Unit tests
/BUILD.gn                # GN build config
📚 Resources
RFC 6455: The WebSocket Protocol

GN Documentation

WebSocket Echo Tool

📄 License
This project is open-source under the MIT License. See LICENSE for details.

