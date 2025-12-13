// #include "session.h"
#include "player.h"

namespace app {

Player* Players::Add(model::GameSession* session, model::Dog* dog) {
	if (!session || !dog) {
		throw std::invalid_argument("Session and Dog cannot be null");
	}

	players_.push_back(Player(session, dog, last_player_id_++));

	return &players_.back();
}

Player* Players::FindByDogIdAndMapId(int dog_id, const std::string& map_id) { return nullptr; }

std::vector<std::string> Players::GetNames() const {
	std::vector<std::string> names = {};
	for (const Player& player : players_) {
		names.push_back(player.GetName());
	}
	return names;
}

json::object Players::GetPlayersInfo() const {
		json::object players;

		for(const Player& player : players_) {
			json::object player_info;
			PlayerInfo info = player.GetInfo();
			player_info["pos"] = {info.pos.x, info.pos.y};
			player_info["speed"] = {info.speed.x, info.speed.y};
			switch(info.dir) {
				case model::Direction::NORTH:
					player_info["dir"] = "U";
					break;
				case model::Direction::SOUTH:
					player_info["dir"] = "D";
					break;
				case model::Direction::WEST:
					player_info["dir"] = "L";
					break;
				case model::Direction::EAST:
					player_info["dir"] = "R";
					break;
			}

			players[std::to_string(player.GetId())] = player_info;
		}

		return players;
	}

} // namespace app

