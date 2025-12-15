#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>

#include "database.h"
#include "model.h"
#include "player.h"
#include "serialization.h"

class StateSaver {
 public:
	explicit StateSaver(model::Game& game, std::optional<uint32_t> save_period_ms,
							  const std::string state_file, app::Players& players,
							  app::PlayerTokens& tokens, Database* database = nullptr)
		 : game_{game}
		 , players_{players}
		 , player_tokens_{tokens}
		 , save_period_ms_{save_period_ms}
		 , state_file_{state_file}
		 , database_{database} {}

	void Tick(double delta_ms) {
		game_.Tick(delta_ms);

		if (database_) {
			UpdateRetiredPlayers();
		}

		if (!save_period_ms_ || state_file_.empty()) {
			return;
		}

		time_since_last_save_ms_ += delta_ms;
		if (time_since_last_save_ms_ >= *save_period_ms_) {
			serialization::SerializeState(state_file_, game_, players_, player_tokens_);
			time_since_last_save_ms_ = 0;
		}
	}

 private:
	void UpdateRetiredPlayers() {
		const double retirement_time_ms = game_.GetRetirementTime() * 1000.0;
		std::vector<app::Token> tokens_to_retire;

		for (const auto& [token, player] : player_tokens_.GetAll()) {
			const model::Dog* dog = player->GetDog();
			if (dog && dog->GetIdleTime() >= retirement_time_ms) {
				tokens_to_retire.push_back(token);
			}
		}

		for (const auto& token : tokens_to_retire) {
			RetirePlayer(token);
		}
	}

	void RetirePlayer(const app::Token& token) {
		app::Player* player = player_tokens_.FindPlayerByToken(token);
		if (!player) {
			return;
		}

		const model::Dog* dog = player->GetDog();
		if (!dog) {
			return;
		}

		model::RetiredPlayerRecord retired_player;
		retired_player.name = dog->GetName();
		retired_player.score = dog->GetScore();
		retired_player.play_time = player->GetTotalPlayTime();

		database_->SaveRetiredPlayer(retired_player);

		RemovePlayerAndToken(token, player);
	}

	void RemovePlayerAndToken(const app::Token& token, app::Player* player) {
		auto& tokens = player_tokens_.GetAll();
		tokens.erase(token);

		auto& players = players_.GetAllPlayers();
		players.erase(std::remove_if(players.begin(), players.end(),
											  [player](const app::Player& current_player) {
												  return &current_player == player;
											  }),
						  players.end());
	}

	model::Game& game_;
	app::Players& players_;
	app::PlayerTokens& player_tokens_;
	std::optional<uint32_t> save_period_ms_;
	double time_since_last_save_ms_ = 0;
	std::string state_file_;
	Database* database_;
};

