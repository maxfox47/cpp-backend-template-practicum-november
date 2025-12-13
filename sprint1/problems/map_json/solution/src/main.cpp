#include "sdk.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"
#include "http_server.h"

using namespace std::literals;
namespace net = boost::asio;

namespace {

template <typename Fn>
void RunWorkers(unsigned thread_count, const Fn& worker_func) {
    thread_count = std::max(1u, thread_count);
    std::vector<std::jthread> worker_threads;
    worker_threads.reserve(thread_count - 1);
    
    while (--thread_count) {
        worker_threads.emplace_back(worker_func);
    }
    worker_func();
}

}  // namespace

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: game_server <game-config-json>"sv << std::endl;
        return EXIT_FAILURE;
    }
    
    try {
        const unsigned thread_count = std::thread::hardware_concurrency();
        net::io_context io_context(thread_count);
        
        model::Game game_model = json_loader::LoadGame(argv[1]);
        http_handler::RequestHandler request_handler{game_model};

        net::signal_set signal_handler(io_context, SIGINT, SIGTERM);
        signal_handler.async_wait([&io_context](boost::system::error_code error_code, int signal_number) {
            if (!error_code) {
                std::cout << "Signal " << signal_number << " received, stopping..."sv << std::endl;
                io_context.stop();
            }
        });

        std::cout << "Server has started..."sv << std::endl;

        http_server::ServeHttp(io_context, {net::ip::address::from_string("0.0.0.0"), 8080},
                                      [&request_handler](auto&& http_request, auto&& response_sender) {
                                          request_handler(std::forward<decltype(http_request)>(http_request),
                                                       std::forward<decltype(response_sender)>(response_sender));
                                      });

        RunWorkers(std::max(1u, thread_count), [&io_context] { 
            io_context.run(); 
        });
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}