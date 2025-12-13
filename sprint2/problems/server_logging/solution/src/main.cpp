#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ip/address.hpp>

#include <thread>
#include <chrono>
#include <vector>
#include <csignal>
#include <iostream>
#include <boost/json.hpp>

#include "http_server.h"
#include "request_handler.h"
#include "logging.h"

using namespace std::literals;
namespace net = boost::asio;

namespace {

template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::thread> workers;
    workers.reserve(n - 1);
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
    for (auto& t : workers) t.join();
}

}  // namespace

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        // Разрешен единственный случай прямого вывода: неправильные аргументы
        std::cout << "Usage: server_logging <game-config.json> <static-root>" << std::endl;
        return EXIT_FAILURE;
    }

    try {
        InitJsonLogging();

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr unsigned short port = 8080;

        // log server started
        boost::json::object start_data{{"port", port}, {"address", "0.0.0.0"}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, boost::json::value(start_data))
                                << "server started";

        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& ec, int /*signal_number*/) {
            if (!ec) {
                ioc.stop();
            }
        });

        http_handler::RequestHandler handler(argv[1], argv[2]);
        http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
            handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });

        RunWorkers(std::max(1u, num_threads), [&ioc] { ioc.run(); });

        boost::json::object exit_ok{{"code", 0}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, boost::json::value(exit_ok))
                                << "server exited";
        return 0;
    } catch (const std::exception& ex) {
        boost::json::object exit_err{{"code", static_cast<int>(EXIT_FAILURE)}, {"exception", ex.what()}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, boost::json::value(exit_err))
                                << "server exited";
        return EXIT_FAILURE;
    }
}


