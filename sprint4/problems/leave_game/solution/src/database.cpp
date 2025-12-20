#include "database.h"

namespace database {

void Database::InitDb() {
	auto conn_wrapper = pool_.GetConnection();
	pqxx::work work{*conn_wrapper};

	work.exec(R"(
        CREATE TABLE IF NOT EXISTS retired_players (
            id SERIAL PRIMARY KEY,
            name VARCHAR(100) NOT NULL,
            score INTEGER NOT NULL,
            play_time DOUBLE PRECISION NOT NULL
        );
    )");

	work.exec(R"(
        CREATE INDEX IF NOT EXISTS idx_score_time_name 
        ON retired_players (score DESC, play_time ASC, name ASC);
    )");

	work.commit();
}

void Database::SaveRetiredPlayer(const std::string& name, int score, double play_time) {
	auto conn_wrapper = pool_.GetConnection();
	pqxx::work work{*conn_wrapper};

	auto check_result =
		 work.exec_params("SELECT id, score, play_time FROM retired_players WHERE name = $1", name);

	if (!check_result.empty()) {

		auto row = check_result[0];
		int current_score = row["score"].as<int>();
		double current_play_time = row["play_time"].as<double>();

		work.exec_params("UPDATE retired_players SET score = $1, play_time = $2 WHERE name = $3",
							  current_score + score, current_play_time + play_time, name);
	} else {

		work.exec_params("INSERT INTO retired_players (name, score, play_time) VALUES ($1, $2, $3)",
							  name, score, play_time);
	}

	work.commit();
}

std::vector<RetiredPlayer> Database::GetRecords(int start, int max_items) {
	if (max_items > 100) {
		throw std::invalid_argument("max_items cannot exceed 100");
	}

	auto conn_wrapper = pool_.GetConnection();
	pqxx::work work{*conn_wrapper};

	auto result = work.exec_params(
		 R"(
            SELECT name, score, play_time 
            FROM retired_players 
            ORDER BY score DESC, play_time ASC, name ASC 
            LIMIT $1 OFFSET $2
        )",
		 max_items, start);

	std::vector<RetiredPlayer> records;
	records.reserve(result.size());

	for (const auto& row : result) {
		RetiredPlayer record;
		record.name = row["name"].as<std::string>();
		record.score = row["score"].as<int>();
		record.play_time = row["play_time"].as<double>();
		records.push_back(record);
	}

	return records;
}

} // namespace database
