#pragma once

#include <pqxx/pqxx>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace database {

// Пул соединений с базой данных PostgreSQL для эффективного управления подключениями
// Использует паттерн RAII для автоматического возврата соединений в пул
class ConnectionPool {
	using ConnectionPtr = std::shared_ptr<pqxx::connection>;

 public:
	// RAII-обертка для соединения из пула
	// Автоматически возвращает соединение в пул при уничтожении
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

		// При уничтожении обертки автоматически возвращаем соединение в пул
		~ConnectionWrapper() {
			if (connection_) {
				pool_->ReleaseConnection(std::move(connection_));
			}
		}

	 private:
		std::shared_ptr<pqxx::connection> connection_;
		ConnectionPool* pool_;
	};

	// Создает пул соединений заданного размера
	// connection_factory - функция, создающая новое соединение с БД
	template <typename ConnectionFactory>
	ConnectionPool(size_t capacity, ConnectionFactory&& connection_factory) {
		connections_pool_.reserve(capacity);
		for (size_t i = 0; i < capacity; ++i) {
			connections_pool_.emplace_back(connection_factory());
		}
	}

	// Получает соединение из пула (блокирует поток, если все соединения заняты)
	// Возвращает RAII-обертку, которая автоматически вернет соединение в пул
	ConnectionWrapper AcquireConnection() {
		std::unique_lock lock{mutex_};
		// Ждем, пока освободится хотя бы одно соединение
		condition_variable_.wait(lock, [this] { return active_connections_count_ < connections_pool_.size(); });

		return {std::move(connections_pool_[active_connections_count_++]), *this};
	}

 private:
	// Возвращает соединение обратно в пул (вызывается автоматически из ConnectionWrapper)
	void ReleaseConnection(ConnectionPtr&& connection) {
		{
			std::lock_guard lock{mutex_};
			assert(active_connections_count_ != 0);
			connections_pool_[--active_connections_count_] = std::move(connection);
		}
		// Уведомляем один из ожидающих потоков о доступности соединения
		condition_variable_.notify_one();
	}

	std::mutex mutex_;
	std::condition_variable condition_variable_;
	std::vector<ConnectionPtr> connections_pool_;
	size_t active_connections_count_ = 0;
};

} // namespace database

