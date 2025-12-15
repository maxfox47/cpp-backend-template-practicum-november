#pragma once

#include <pqxx/pqxx>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

class ConnectionPool {
	using ConnectionPoolType = ConnectionPool;
	using ConnectionPtr = std::shared_ptr<pqxx::connection>;

 public:
	class ConnectionWrapper {
	 public:
		ConnectionWrapper(std::shared_ptr<pqxx::connection>&& connection, ConnectionPoolType& pool) noexcept
			 : connection_{std::move(connection)}, pool_{&pool} {}

		ConnectionWrapper(const ConnectionWrapper&) = delete;
		ConnectionWrapper& operator=(const ConnectionWrapper&) = delete;

		ConnectionWrapper(ConnectionWrapper&&) = default;
		ConnectionWrapper& operator=(ConnectionWrapper&&) = default;

		pqxx::connection& operator*() const& noexcept { return *connection_; }
		pqxx::connection& operator*() const&& = delete;

		pqxx::connection* operator->() const& noexcept { return connection_.get(); }

		~ConnectionWrapper() {
			if (connection_) {
				pool_->ReturnConnection(std::move(connection_));
			}
		}

	 private:
		std::shared_ptr<pqxx::connection> connection_;
		ConnectionPoolType* pool_;
	};

	// ConnectionFactory is a functional object returning std::shared_ptr<pqxx::connection>
	template <typename ConnectionFactory>
	ConnectionPool(size_t capacity, ConnectionFactory&& connection_factory) {
		pool_.reserve(capacity);
		for (size_t i = 0; i < capacity; ++i) {
			pool_.emplace_back(connection_factory());
		}
	}

	ConnectionWrapper GetConnection() {
		std::unique_lock lock{mutex_};
		// Блокируем текущий поток и ждём, пока cond_var_ не получит уведомление и не освободится
		// хотя бы одно соединение
		cond_var_.wait(lock, [this] { return used_connections_ < pool_.size(); });
		// После выхода из цикла ожидания мьютекс остаётся захваченным

		return {std::move(pool_[used_connections_++]), *this};
	}

 private:
	void ReturnConnection(ConnectionPtr&& conn) {
		// Возвращаем соединение обратно в пул
		{
			std::lock_guard lock{mutex_};
			assert(used_connections_ != 0);
			pool_[--used_connections_] = std::move(conn);
		}
		// Уведомляем один из ожидающих потоков об изменении состояния пула
		cond_var_.notify_one();
	}

	std::mutex mutex_;
	std::condition_variable cond_var_;
	std::vector<ConnectionPtr> pool_;
	size_t used_connections_ = 0;
};
