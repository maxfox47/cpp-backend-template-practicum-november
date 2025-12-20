#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "database.h"
#include "model.h"
#include "player.h"
#include "serialization.h"

class StateSaver {
 public:
	explicit StateSaver(model::Game& game, std::optional<uint32_t> period_ms,
							  const std::string state_file, app::Players& players,
							  app::PlayerTokens& tokens, database::Database& db)
		 : game_(game), players_(players), tokens_(tokens), period_ms_(period_ms),
			state_file_(state_file), db_(db) {}

	void Tick(double ms) {
		game_.Tick(ms);
		CheckRetiredPlayers(ms);

		if (!period_ms_ || state_file_.empty()) {
			return;
		}

		from_last_save_ms_ += ms;
		if (from_last_save_ms_ >= *period_ms_) {
			serialization::SerializeState(state_file_, game_, players_, tokens_);
			from_last_save_ms_ = 0;
		}
	}

 private:
	void CheckRetiredPlayers(double ms) {
		double retirement_time = game_.GetDogRetirementTime();

		std::vector<app::Player*> players_to_retire;

		for (auto& player : players_.GetAllPlayers()) {
			if (player.GetIdleTime() >= retirement_time) {
				players_to_retire.push_back(&player);
			}
		}

		for (auto* player : players_to_retire) {
			RetirePlayer(player);
		}
	}

	void RetirePlayer(app::Player* player) {
		if (!player) {
			return;
		}

		const model::Dog* dog = player->GetDog();
		if (dog) {
			double play_time_seconds = player->GetPlayTime() / 1000.0;
			db_.SaveRetiredPlayer(dog->GetName(), dog->GetScore(), play_time_seconds);
		}

		auto all_tokens = tokens_.GetAll();
		for (const auto& [token, p] : all_tokens) {
			if (p == player) {
				tokens_.RemoveToken(token);
				break;
			}
		}

		players_.Remove(player);
	}

	model::Game& game_;
	app::Players& players_;
	app::PlayerTokens& tokens_;
	std::optional<uint32_t> period_ms_;
	double from_last_save_ms_ = 0;
	std::string state_file_;
	database::Database& db_;
};
