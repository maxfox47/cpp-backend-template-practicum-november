#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "model.h"
#include "player.h"
#include "serialization.h"

class StateSaver {
 public:
	explicit StateSaver(model::Game& game, std::optional<uint32_t> period_ms,
							  const std::string state_file, app::Players& players,
							  app::PlayerTokens& tokens)
		 : game_(game), players_(players), tokens_(tokens), period_ms_(period_ms),
			state_file_(state_file) {}

	void Tick(double ms) {
		game_.Tick(ms);
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
	model::Game& game_;
	app::Players& players_;
	app::PlayerTokens& tokens_;
	std::optional<uint32_t> period_ms_;
	double from_last_save_ms_ = 0;
	std::string state_file_;
};

