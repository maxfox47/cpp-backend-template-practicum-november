#pragma once

#include <pqxx/pqxx>

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace database {

class ConnectionPool {
	using PoolType = ConnectionPool;
	using ConnectionPtr = std::shared_ptr<pqxx::connection>;
	using ConnectionFactory = std::function<ConnectionPtr()>;

 public:
	class ConnectionWrapper {
	 public:
		ConnectionWrapper(ConnectionPtr conn, PoolType& pool) noexcept
			 : conn_{std::move(conn)}, pool_{&pool} {}

		ConnectionWrapper(const ConnectionWrapper&) = delete;
		ConnectionWrapper& operator=(const ConnectionWrapper&) = delete;

		ConnectionWrapper(ConnectionWrapper&&) = default;
		ConnectionWrapper& operator=(ConnectionWrapper&&) = default;

		pqxx::connection& operator*() const& noexcept { return *conn_; }
		pqxx::connection& operator*() const&& = delete;

		pqxx::connection* operator->() const& noexcept { return conn_.get(); }

		~ConnectionWrapper() {
			if (conn_) {
				pool_->ReturnConnection(std::move(conn_));
			}
		}

	 private:
		ConnectionPtr conn_;
		PoolType* pool_;
	};

	template <typename Factory>
	ConnectionPool(size_t capacity, Factory&& connection_factory)
		 : capacity_{capacity}, connection_factory_{std::forward<Factory>(connection_factory)} {
		pool_.reserve(capacity_);
	}

	ConnectionWrapper GetConnection() {
		std::unique_lock lock{mutex_};

		// Если есть свободное соединение в пуле, используем его
		if (used_connections_ < pool_.size()) {
			return {std::move(pool_[used_connections_++]), *this};
		}

		// Если пул не заполнен, создаём новое соединение
		if (pool_.size() < capacity_) {
			auto conn = connection_factory_();
			pool_.push_back(conn);
			used_connections_++;
			return {std::move(conn), *this};
		}

		// Ждём, пока освободится соединение
		cond_var_.wait(lock, [this] { return used_connections_ < pool_.size(); });
		return {std::move(pool_[used_connections_++]), *this};
	}

 private:
	void ReturnConnection(ConnectionPtr&& conn) {
		{
			std::lock_guard lock{mutex_};
			assert(used_connections_ != 0);
			pool_[--used_connections_] = std::move(conn);
		}
		cond_var_.notify_one();
	}

	std::mutex mutex_;
	std::condition_variable cond_var_;
	std::vector<ConnectionPtr> pool_;
	size_t capacity_;
	ConnectionFactory connection_factory_;
	size_t used_connections_ = 0;
};

}  // namespace database
