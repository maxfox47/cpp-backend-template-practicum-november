#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "database.h"
#include "model.h"
#include "player.h"
#include "serialization.h"

// Класс для управления состоянием игры: сохранение состояния и обработка ухода неактивных игроков
class StateSaver {
 public:
	explicit StateSaver(model::Game& game, std::optional<uint32_t> save_period_ms,
							  const std::string& state_file_path, app::Players& players,
							  app::PlayerTokens& tokens, database::Database& database)
		 : game_{game}, players_{players}, tokens_{tokens}, save_period_ms_{save_period_ms},
			state_file_path_{state_file_path}, database_{database} {}

	// Выполняет один тик обновления состояния игры
	// delta_time_ms - время, прошедшее с последнего тика в миллисекундах
	void Tick(double delta_time_ms) {
		game_.Tick(delta_time_ms);
		// Проверяем и удаляем неактивных игроков (тех, кто не двигался слишком долго)
		CheckAndRetireInactivePlayers(delta_time_ms);

		// Сохраняем состояние игры на диск, если задан период сохранения
		if (!save_period_ms_ || state_file_path_.empty()) {
			return;
		}

		time_since_last_save_ms_ += delta_time_ms;
		if (time_since_last_save_ms_ >= *save_period_ms_) {
			serialization::SerializeState(state_file_path_, game_, players_, tokens_);
			time_since_last_save_ms_ = 0;
		}
	}

 private:
	// Проверяет всех игроков на неактивность и удаляет тех, кто превысил лимит времени простоя
	void CheckAndRetireInactivePlayers(double delta_time_ms) {
		double retirement_time_threshold = game_.GetDogRetirementTime();

		std::vector<app::Player*> inactive_players;

		// Собираем список игроков, которые неактивны слишком долго
		for (auto& player : players_.GetAllPlayers()) {
			if (player.GetIdleTime() >= retirement_time_threshold) {
				inactive_players.push_back(&player);
			}
		}

		// Удаляем неактивных игроков и сохраняем их результаты в БД
		for (auto* player : inactive_players) {
			RetirePlayerAndSaveRecord(player);
		}
	}

	// Удаляет игрока из игры и сохраняет его результаты в базу данных
	void RetirePlayerAndSaveRecord(app::Player* player) {
		if (!player) {
			return;
		}

		// Сохраняем результаты игрока в БД перед удалением
		const model::Dog* dog = player->GetDog();
		if (dog) {
			double play_time_seconds = player->GetPlayTime() / 1000.0;  // Конвертируем миллисекунды в секунды
			database_.SaveRetiredPlayer(dog->GetName(), dog->GetScore(), play_time_seconds);
		}

		// Удаляем токен игрока
		auto all_tokens = tokens_.GetAll();
		for (const auto& [token, token_player] : all_tokens) {
			if (token_player == player) {
				tokens_.RemoveToken(token);
				break;
			}
		}

		// Удаляем игрока из списка активных игроков
		players_.Remove(player);
	}

	model::Game& game_;
	app::Players& players_;
	app::PlayerTokens& tokens_;
	std::optional<uint32_t> save_period_ms_;
	double time_since_last_save_ms_ = 0;
	std::string state_file_path_;
	database::Database& database_;
};
