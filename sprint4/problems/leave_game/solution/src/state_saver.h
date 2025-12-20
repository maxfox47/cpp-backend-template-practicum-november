#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "database.h"
#include "model.h"
#include "player.h"
#include "serialization.h"

// Класс StateSaver отвечает за периодическое сохранение состояния игры
// и управление уходом неактивных игроков из игры
class StateSaver {
 public:
	// Конструктор: инициализирует StateSaver с игрой, периодом сохранения, путем к файлу состояния,
	// менеджерами игроков и токенов, а также экземпляром базы данных
	explicit StateSaver(model::Game& game, std::optional<uint32_t> save_period_ms,
							  const std::string& state_file_path, app::Players& players,
							  app::PlayerTokens& tokens, database::Database& database)
		 : game_{game}, players_{players}, tokens_{tokens}, save_period_ms_{save_period_ms},
			state_file_path_{state_file_path}, database_{database} {}

	// Выполняет один тик игры: обновляет состояние игры, проверяет неактивных игроков
	// и сохраняет состояние, если истек период сохранения
	// ms - время, прошедшее с последнего тика в миллисекундах
	void Tick(double delta_time_ms) {
		game_.Tick(delta_time_ms);
		CheckAndRetireInactivePlayers(delta_time_ms);

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
	// Проверяет игроков, которые были неактивны дольше порога времени ухода,
	// и добавляет их в список для ухода из игры
	void CheckAndRetireInactivePlayers(double delta_time_ms) {
		double retirement_time_threshold = game_.GetDogRetirementTime();

		std::vector<app::Player*> inactive_players;

		for (auto& player : players_.GetAllPlayers()) {
			if (player.GetIdleTime() >= retirement_time_threshold) {
				inactive_players.push_back(&player);
			}
		}

		for (auto* player : inactive_players) {
			RetirePlayerAndSaveRecord(player);
		}
	}

	// Удаляет игрока из игры: сохраняет его запись в базе данных
	// и удаляет его из активных игроков и токенов
	void RetirePlayerAndSaveRecord(app::Player* player) {
		if (!player) {
			return;
		}

		const model::Dog* dog = player->GetDog();
		if (dog) {
			double play_time_seconds = player->GetPlayTime() / 1000.0;
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

		// Удаляем игрока из активных игроков
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
