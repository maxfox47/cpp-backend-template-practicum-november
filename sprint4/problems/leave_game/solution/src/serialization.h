#pragma once

#include "model.h"
#include "player.h"

#include <string>

namespace serialization {

void SerializeState(const std::string& file, const model::Game& game, const app::Players& players,
						  const app::PlayerTokens& tokens);

void DeserializeState(const std::string& file, model::Game& game, app::Players& players,
							 app::PlayerTokens& tokens);

void SerializeGameState(const std::string& file, const model::Game& game);

void DeserializeGameState(const std::string& file, model::Game& game);

} // namespace serialization
