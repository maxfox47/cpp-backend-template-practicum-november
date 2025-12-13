#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/strand.hpp>

#include <thread>
#include <chrono>
#include <vector>
#include <csignal>
#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/json.hpp>
#include <memory>

#include "http_server.h"
#include "request_handler.h"
#include "logging.h"
#include "application/game.h"
#include "application/players.h"
#include "util/token.h"

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
        std::cout << "Usage: game_server <game-config.json> <static-root>" << std::endl;
        return EXIT_FAILURE;
    }

    try {
        InitJsonLogging();

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr unsigned short port = 8080;

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

        // Загружаем конфигурацию
        std::ifstream config_file(argv[1]);
        if (!config_file) {
            throw std::runtime_error("Cannot open config file");
        }
        std::stringstream buffer;
        buffer << config_file.rdbuf();
        boost::json::value config_data = boost::json::parse(buffer.str());

        // Создаем игровые компоненты
        application::Game game(config_data);
        application::Players players;
        util::PlayerTokens token_generator;

        // Создаем strand для API запросов
        auto api_strand = net::make_strand(ioc);

        // Создаем обработчик запросов
        auto handler = std::make_shared<http_handler::RequestHandler>(
            argv[1], argv[2], api_strand, game, players, token_generator);

        // Запускаем HTTP сервер
        http_server::ServeHttp(ioc, {address, port}, [handler](auto&& req, auto&& send) {
            (*handler)(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
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

