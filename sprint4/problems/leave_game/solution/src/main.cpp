#include "conn_pull.h"
#include "player.h"
#include "sdk.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/program_options.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

#include "database.h"
#include "json_loader.h"
#include "logger.h"
#include "request_handler.h"
#include "serialization.h"
#include "state_saver.h"
#include "ticker.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace logging = boost::log;
namespace po = boost::program_options;
namespace fs = std::filesystem;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
	n = std::max(1u, n);
	std::vector<std::jthread> workers;
	workers.reserve(n - 1);
	// Запускаем n-1 рабочих потоков, выполняющих функцию fn
	while (--n) {
		workers.emplace_back(fn);
	}
	fn();
}

void InitLogging() {
	logging::add_common_attributes();

	auto console_sink = logging::add_console_log(
		 std::cout, logging::keywords::format = &JsonFormatter, logging::keywords::auto_flush = true);
}

struct Args {
	std::optional<uint32_t> tick_period;
	std::string config_file;
	fs::path www_root;
	bool randomize_spawn_points = false;
	std::optional<std::string> state_file;
	std::optional<uint32_t> save_period;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
	Args args;
	uint32_t tick_period_tmp;
	uint32_t save_period_tmp;
	std::string state_file_tmp;

	po::options_description desc("Allowed options");
	desc.add_options()("help,h", "produce help message")(
		 "tick-period,t", po::value(&tick_period_tmp)->value_name("milliseconds"), "set tick period")(
		 "save-state-period", po::value(&save_period_tmp)->value_name("milliseconds"),
		 "set save period")("state-file", po::value(&state_file_tmp)->value_name("file"),
								  "set state file path")(
		 "config-file,c", po::value(&args.config_file)->value_name("file"), "set config file path")(
		 "www-root,w", po::value(&args.www_root)->value_name("dir"), "set static files root")(
		 "randomize-spawn-points", po::bool_switch(&args.randomize_spawn_points),
		 "spawn dogs at random positions");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.contains("help")) {
		std::cout << desc << std::endl;
		return std::nullopt;
	}

	if (vm.contains("tick-period")) {
		args.tick_period = tick_period_tmp;
	}

	if (vm.contains("state-file")) {
		args.state_file = state_file_tmp;
	}

	if (vm.contains("save-state-period")) {
		args.save_period = save_period_tmp;
	}

	if (args.config_file.empty()) {
		throw std::runtime_error("Config file path is not specified"s);
	}
	if (args.www_root.empty()) {
		throw std::runtime_error("Static files root is not specified"s);
	}

	return args;
}

} // namespace

int main(int argc, const char* argv[]) {
	auto args_opt = ParseCommandLine(argc, argv);
	if (!args_opt) {
		return EXIT_FAILURE;
	}
	auto& args = *args_opt;

	try {
		InitLogging();

		// 1. Загружаем карту из файла и построить модель игры
		model::Game game = json_loader::LoadGame(args.config_file);
		std::filesystem::path static_path = args.www_root;
		app::Players players;
		app::PlayerTokens tokens;

		const char* db_url = std::getenv("GAME_DB_URL");
		if (!db_url) {
			throw std::runtime_error("GAME_DB_URL environment variable is not set");
		}

		std::unique_ptr<database::ConnectionPool> connection_pool;
		std::unique_ptr<database::Database> database;

		try {
			connection_pool = std::make_unique<database::ConnectionPool>(
				 1, [db_url] { return std::make_shared<pqxx::connection>(db_url); });
			database = std::make_unique<database::Database>(*connection_pool);
			database->InitializeSchema();
		} catch (const std::exception& ex) {
			BOOST_LOG_TRIVIAL(error)
				 << logging::add_value(exception_c, ex.what()) << "failed to initialize database";
			throw;
		}

		StateSaver state_saver(game, args.save_period, args.state_file.value_or(""), players, tokens,
									  *database);

		if (args.state_file) {
			try {
				serialization::DeserializeState(*args.state_file, game, players, tokens);
			} catch (const std::exception& ex) {
				BOOST_LOG_TRIVIAL(error)
					 << logging::add_value(exception_c, ex.what()) << "failed to restore state";
				return EXIT_FAILURE;
			}
		}

		// 2. Инициализируем io_context
		const unsigned num_threads = std::thread::hardware_concurrency();
		net::io_context ioc(num_threads);
		auto api_strand = net::make_strand(ioc);

		// 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
		net::signal_set signals(ioc, SIGINT, SIGTERM);
		signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
			if (!ec) {
				ioc.stop();
			}
		});

		// 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
		// http_handler::RequestHandler handler{game, std::filesystem::absolute(static_path)};
		auto handler = std::make_shared<http_handler::RequestHandler>(
			 game, std::filesystem::absolute(static_path), api_strand, args.randomize_spawn_points,
			 args.tick_period.has_value(), state_saver, players, tokens, *database);
		http_handler::LoggingRequestHandler log_handler(*handler);

		std::shared_ptr<Ticker> ticker;
		if (args.tick_period) {
			std::chrono::milliseconds period{*args.tick_period};
			ticker = std::make_shared<Ticker>(
				 api_strand, period, [&state_saver](std::chrono::steady_clock::duration delta) {
					 double ms = std::chrono::duration_cast<std::chrono::milliseconds>(delta).count();
					 state_saver.Tick(ms);
				 });
			ticker->Start();
		}

		// 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
		const auto address = net::ip::make_address("0.0.0.0");
		constexpr int port = 8080;
		http_server::ServeHttp(
			 ioc, {address, port}, [&log_handler](auto&& req, const std::string ip, auto&& send) {
				 log_handler(std::forward<decltype(req)>(req), ip, std::forward<decltype(send)>(send));
			 });

		BOOST_LOG_TRIVIAL(info) << logging::add_value(port_p, port)
										<< logging::add_value(ip_add, "0.0.0.0") << "server started";

		// 6. Запускаем обработку асинхронных операций
		RunWorkers(std::max(1u, num_threads), [&ioc] { ioc.run(); });

		if (args.state_file) {
			try {
				serialization::SerializeState(*args.state_file, game, players, tokens);
			} catch (const std::exception& ex) {
				BOOST_LOG_TRIVIAL(error) << boost::log::add_value(exception_c, ex.what())
												 << "failed to save state on shutdown";
			}
		}

		BOOST_LOG_TRIVIAL(info) << boost::log::add_value(status_c, "0") << "server exited";
	} catch (const std::exception& ex) {
		BOOST_LOG_TRIVIAL(info) << logging::add_value(exception_c, ex.what())
										<< logging::add_value(status_c, "EXIT_FAILURE") << "server exited";
		std::cerr << ex.what() << std::endl;

		return EXIT_FAILURE;
	}
}
