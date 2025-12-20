#pragma once

#include "connection_pool.h"

#include <string>
#include <vector>

namespace database {

// Максимальное количество записей, которое можно запросить за один раз
constexpr int MAX_RECORDS_LIMIT = 100;

// Структура данных для записи об ушедшем игроке
struct RetiredPlayer {
	std::string name;      // Имя игрока
	int score;             // Набранные очки
	double play_time;      // Время игры в секундах
};

// Класс для работы с базой данных PostgreSQL
// Управляет таблицей retired_players и операциями с записями об ушедших игроках
class Database {
 public:
	explicit Database(ConnectionPool& connection_pool) : connection_pool_{connection_pool} {}

	// Инициализирует схему БД: создает таблицу retired_players и индекс для сортировки
	void InitializeSchema();
	
	// Сохраняет или обновляет запись об ушедшем игроке
	// Если игрок с таким именем уже существует, обновляет его очки и время игры
	void SaveRetiredPlayer(const std::string& player_name, int player_score, double player_play_time);
	
	// Получает записи об ушедших игроках с пагинацией
	// start_index - индекс начала выборки (OFFSET)
	// max_items_count - максимальное количество записей (LIMIT, не более MAX_RECORDS_LIMIT)
	// Записи сортируются по score DESC, play_time ASC, name ASC
	std::vector<RetiredPlayer> GetRecords(int start_index = 0, int max_items_count = MAX_RECORDS_LIMIT);

 private:
	ConnectionPool& connection_pool_;
};

} // namespace database
