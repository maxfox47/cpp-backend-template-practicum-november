#include "sdk.h"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <thread>
#include <vector>

#include "http_server.h"

namespace {
namespace net = boost::asio;
using namespace std::literals;
namespace sys = boost::system;
namespace http = boost::beast::http;

using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
};

StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive,
                                  std::string_view content_type = ContentType::TEXT_HTML) {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, std::string(content_type));
    response.body() = std::string(body);
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    return response;
}

StringResponse HandleRequest(StringRequest&& req) {
    if (req.method() == http::verb::get || req.method() == http::verb::head) {
        std::string target(req.target());
        if (!target.empty() && target[0] == '/') {
            target = target.substr(1);
        }
        std::string body = "Hello, " + target;
        if (req.method() == http::verb::head) {
            StringResponse response(http::status::ok, req.version());
            response.set(http::field::content_type, std::string(ContentType::TEXT_HTML));
            response.content_length(body.size());
            response.keep_alive(req.keep_alive());
            return response;
        } else {
            return MakeStringResponse(http::status::ok, body, req.version(), req.keep_alive());
        }
    } else {
        std::string body = "Invalid method";
        StringResponse response(http::status::method_not_allowed, req.version());
        response.set(http::field::content_type, std::string(ContentType::TEXT_HTML));
        response.set(http::field::allow, "GET, HEAD");
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(req.keep_alive());
        return response;
    }
}

template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main() {
    const unsigned num_threads = std::thread::hardware_concurrency();

    net::io_context ioc(num_threads);

    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
        if (!ec) {
            ioc.stop();
        }
    });

    const auto address = net::ip::make_address("0.0.0.0");
    constexpr net::ip::port_type port = 8080;
    http_server::ServeHttp(ioc, {address, port}, [](auto&& req, auto&& sender) {
        sender(HandleRequest(std::forward<decltype(req)>(req)));
    });

    std::cout << "Server has started..."sv << std::endl;

    RunWorkers(num_threads, [&ioc] {
        ioc.run();
    });
}
