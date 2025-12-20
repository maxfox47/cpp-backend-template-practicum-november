#pragma once

#include <pqxx/pqxx>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace database {

// ConnectionPool управляет пулом соединений с базой данных PostgreSQL
// Предоставляет потокобезопасный способ получения и возврата соединений
class ConnectionPool {
	using ConnectionPtr = std::shared_ptr<pqxx::connection>;

 public:
	// ConnectionWrapper - это RAII-обертка для соединения с базой данных
	// Гарантирует, что соединение, полученное из пула, будет возвращено при уничтожении
	class ConnectionWrapper {
	 public:
		ConnectionWrapper(std::shared_ptr<pqxx::connection>&& connection, ConnectionPool& pool) noexcept
			 : connection_{std::move(connection)}, pool_{&pool} {}

		ConnectionWrapper(const ConnectionWrapper&) = delete;
		ConnectionWrapper& operator=(const ConnectionWrapper&) = delete;

		ConnectionWrapper(ConnectionWrapper&&) = default;
		ConnectionWrapper& operator=(ConnectionWrapper&&) = default;

		pqxx::connection& operator*() const& noexcept { return *connection_; }
		pqxx::connection& operator*() const&& = delete;

		pqxx::connection* operator->() const& noexcept { return connection_.get(); }

		~ConnectionWrapper() {
			// Возвращаем соединение в пул при уничтожении обертки
			if (connection_) {
				pool_->ReleaseConnection(std::move(connection_));
			}
		}

	 private:
		std::shared_ptr<pqxx::connection> connection_;
		ConnectionPool* pool_;
	};

	// Конструктор: инициализирует пул соединений с заданной емкостью
	// и функцией-фабрикой для создания новых соединений
	// ConnectionFactory - функциональный объект, возвращающий std::shared_ptr<pqxx::connection>
	template <typename ConnectionFactory>
	ConnectionPool(size_t capacity, ConnectionFactory&& connection_factory) {
		connections_pool_.reserve(capacity);
		for (size_t i = 0; i < capacity; ++i) {
			connections_pool_.emplace_back(connection_factory());
		}
	}

	// Получает соединение из пула
	// Если соединения недоступны, текущий поток блокируется до тех пор,
	// пока не освободится хотя бы одно соединение
	ConnectionWrapper AcquireConnection() {
		std::unique_lock lock{mutex_};
		// Блокируем текущий поток и ждем, пока condition_variable_ не получит уведомление
		// и не освободится хотя бы одно соединение
		condition_variable_.wait(lock, [this] { return active_connections_count_ < connections_pool_.size(); });
		// После выхода из цикла ожидания мьютекс остается захваченным

		return {std::move(connections_pool_[active_connections_count_++]), *this};
	}

 private:
	// Возвращает соединение обратно в пул
	// Уведомляет один из ожидающих потоков о том, что соединение стало доступным
	void ReleaseConnection(ConnectionPtr&& connection) {
		// Возвращаем соединение в пул
		{
			std::lock_guard lock{mutex_};
			assert(active_connections_count_ != 0);
			connections_pool_[--active_connections_count_] = std::move(connection);
		}
		// Уведомляем один из ожидающих потоков об изменении состояния пула
		condition_variable_.notify_one();
	}

	std::mutex mutex_;
	std::condition_variable condition_variable_;
	std::vector<ConnectionPtr> connections_pool_;
	size_t active_connections_count_ = 0;
};

} // namespace database
