#pragma once

#include "conn_pull.h"
#include "model.h"

#include <pqxx/pqxx>
#include <vector>

class Database {
 public:
	explicit Database(ConnectionPool& connection_pool) noexcept
		 : connection_pool_{connection_pool} {}

	void SaveRetiredPlayer(const model::RetiredPlayerRecord& record) {
		auto connection = connection_pool_.GetConnection();
		pqxx::work transaction{*connection};

		transaction.exec_params(
			 "INSERT INTO retired_players (name, score, play_time) VALUES ($1, $2, $3)",
			 record.name, record.score, record.play_time);

		transaction.commit();
	}

	std::vector<model::RetiredPlayerRecord> GetRecords(int start_index, int max_items) {
		auto connection = connection_pool_.GetConnection();
		pqxx::read_transaction transaction{*connection};

		auto result = transaction.exec_params(
			 "SELECT name, score, play_time FROM retired_players "
			 "ORDER BY score DESC, play_time ASC, name ASC "
			 "LIMIT $1 OFFSET $2",
			 max_items, start_index);

		std::vector<model::RetiredPlayerRecord> records;
		records.reserve(result.size());

		for (const auto& row : result) {
			model::RetiredPlayerRecord retired_player;
			retired_player.name = row["name"].as<std::string>();
			retired_player.score = row["score"].as<int>();
			retired_player.play_time = row["play_time"].as<double>();
			records.push_back(std::move(retired_player));
		}

		return records;
	}

 private:
	ConnectionPool& connection_pool_;
};
