#include "database.h"

namespace database {

void Database::InitializeSchema() {
	// Получаем соединение из пула
	auto connection_wrapper = connection_pool_.AcquireConnection();
	// Начинаем транзакцию
	pqxx::work transaction{*connection_wrapper};

	// Создаем таблицу для хранения записей об ушедших игроках
	transaction.exec(R"(
        CREATE TABLE IF NOT EXISTS retired_players (
            id SERIAL PRIMARY KEY,
            name VARCHAR(100) NOT NULL,
            score INTEGER NOT NULL,
            play_time DOUBLE PRECISION NOT NULL
        );
    )");

	// Создаем составной индекс для эффективной сортировки записей
	// Порядок сортировки: score DESC (по убыванию), play_time ASC (по возрастанию), name ASC (по возрастанию)
	transaction.exec(R"(
        CREATE INDEX IF NOT EXISTS idx_score_time_name 
        ON retired_players (score DESC, play_time ASC, name ASC);
    )");

	// Применяем изменения
	transaction.commit();
}

void Database::SaveRetiredPlayer(const std::string& player_name, int player_score, double player_play_time) {
	// Получаем соединение из пула
	auto connection_wrapper = connection_pool_.AcquireConnection();
	// Начинаем транзакцию
	pqxx::work transaction{*connection_wrapper};

	// Проверяем, существует ли игрок с таким именем
	auto check_result = transaction.exec_params(
		 "SELECT id, score, play_time FROM retired_players WHERE name = $1", player_name);

	if (!check_result.empty()) {
		// Если игрок существует, обновляем его очки и время игры
		auto row = check_result[0];
		int current_score = row["score"].as<int>();
		double current_play_time = row["play_time"].as<double>();

		transaction.exec_params("UPDATE retired_players SET score = $1, play_time = $2 WHERE name = $3",
							  current_score + player_score, current_play_time + player_play_time, player_name);
	} else {
		// Если игрок не существует, вставляем новую запись
		transaction.exec_params("INSERT INTO retired_players (name, score, play_time) VALUES ($1, $2, $3)",
							  player_name, player_score, player_play_time);
	}

	// Применяем изменения
	transaction.commit();
}

std::vector<RetiredPlayer> Database::GetRecords(int start_index, int max_items_count) {
	// Проверяем валидность параметров
	if (max_items_count > MAX_RECORDS_LIMIT) {
		throw std::invalid_argument("max_items_count cannot exceed " + std::to_string(MAX_RECORDS_LIMIT));
	}
	if (start_index < 0) {
		throw std::invalid_argument("start_index cannot be negative");
	}

	// Получаем соединение из пула
	auto connection_wrapper = connection_pool_.AcquireConnection();
	// Начинаем транзакцию
	pqxx::work transaction{*connection_wrapper};

	// Выполняем запрос с сортировкой и пагинацией
	auto result = transaction.exec_params(
		 R"(
            SELECT name, score, play_time 
            FROM retired_players 
            ORDER BY score DESC, play_time ASC, name ASC 
            LIMIT $1 OFFSET $2
        )",
		 max_items_count, start_index);

	std::vector<RetiredPlayer> records;
	records.reserve(result.size());

	// Заполняем вектор объектов RetiredPlayer из результата запроса
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
