#include "database.h"

#include <pqxx/zview.hxx>

namespace database {

using namespace std::literals;
using pqxx::operator"" _zv;

void Database::InitializeSchema() {
	auto conn_wrapper = pool_.GetConnection();
	pqxx::work work{*conn_wrapper};

	work.exec(R"(
        CREATE TABLE IF NOT EXISTS retired_players (
            id SERIAL PRIMARY KEY,
            name VARCHAR(100) NOT NULL,
            score INTEGER NOT NULL,
            play_time DOUBLE PRECISION NOT NULL
        );
    )"_zv);

	work.exec(R"(
        CREATE INDEX IF NOT EXISTS idx_score_time_name 
        ON retired_players (score DESC, play_time ASC, name ASC);
    )"_zv);

	work.commit();
}

void Database::SaveRetiredPlayer(const std::string& name, int score, double play_time) {
	auto conn_wrapper = pool_.GetConnection();
	pqxx::work work{*conn_wrapper};

	auto check_result = work.exec_params(
		 R"(SELECT id, score, play_time FROM retired_players WHERE name = $1)"_zv, name);

	if (!check_result.empty()) {
		auto row = check_result[0];
		int current_score = row["score"].as<int>();
		double current_play_time = row["play_time"].as<double>();

		work.exec_params(
			 R"(UPDATE retired_players SET score = $1, play_time = $2 WHERE name = $3)"_zv,
			 current_score + score, current_play_time + play_time, name);
	} else {
		work.exec_params(
			 R"(INSERT INTO retired_players (name, score, play_time) VALUES ($1, $2, $3))"_zv,
			 name, score, play_time);
	}

	work.commit();
}

std::vector<RetiredPlayerRecord> Database::GetRecords(int start_index, int max_items_count) {
	if (max_items_count > 100) {
		throw std::invalid_argument("max_items_count cannot exceed 100");
	}

	auto conn_wrapper = pool_.GetConnection();
	pqxx::read_transaction tr{*conn_wrapper};

	auto result = tr.exec_params(
		 R"(
            SELECT name, score, play_time 
            FROM retired_players 
            ORDER BY score DESC, play_time ASC, name ASC 
            LIMIT $1 OFFSET $2
        )"_zv,
		 max_items_count, start_index);

	std::vector<RetiredPlayerRecord> records;
	records.reserve(result.size());

	for (const auto& row : result) {
		RetiredPlayerRecord record;
		record.name = row["name"].as<std::string>();
		record.score = row["score"].as<int>();
		record.play_time = row["play_time"].as<double>();
		records.push_back(record);
	}

	return records;
}

}  // namespace database
