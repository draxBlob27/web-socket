#ifndef ROOT_CERTIFICATES_HPP
#define ROOT_CERTIFICATES_HPP

#include <boost/asio/ssl.hpp>
#include <stdexcept>

namespace ssl = boost::asio::ssl;

inline void load_root_certificates(ssl::context& ctx) {
    ctx.set_default_verify_paths();
}

#endif