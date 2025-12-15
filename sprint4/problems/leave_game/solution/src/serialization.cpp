#include "serialization.h"
#include "model.h"
#include "model_serialization.h"
#include "player.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace serialization {

model::GameStateRepr MakeStateFromGame(const model::Game& game) {
	model::GameStateRepr state;

	const auto& sessions = game.GetGameSessions();
	for (const auto& session : sessions) {
		model::GameSessionRepr srepr;

		const auto* map = session.GetMap();
		if (!map) {
			continue;
		}
		srepr.map_id = *map->GetId();

		for (const auto& dog : session.GetDogs()) {
			srepr.dogs.emplace_back(dog);
		}

		for (const auto& lo : session.GetLostObjects()) {
			model::LostObjectRepr lrepr;
			lrepr.type = lo.type;
			lrepr.pos = lo.pos;
			srepr.lost_objects.push_back(lrepr);
		}

		srepr.last_dog_id = session.GetLastDogId();

		state.sessions.push_back(std::move(srepr));
	}

	return state;
}

static void RestoreGameFromState(const model::GameStateRepr& state, model::Game& game) {
	for (const auto& srepr : state.sessions) {
		const auto* map = game.FindMap(model::Map::Id{srepr.map_id});
		if (!map) {
			continue;
		}

		auto* session =
			 game.AddGameSession(model::GameSession{map, game.GetPeriod(), game.GetProbability()});
		session->SetLastDogId(srepr.last_dog_id);

		for (const auto& drepr : srepr.dogs) {
			auto dog = drepr.Restore();
			session->AddExistingDog(std::move(dog));
		}

		for (const auto& lrepr : srepr.lost_objects) {
			session->AddLostObject({lrepr.type, lrepr.pos});
		}
	}
}

static app::serialization::PlayersRepr MakePlayersState(const app::Players& players,
																		  const app::PlayerTokens& tokens) {
	app::serialization::PlayersRepr ps;

	for (const auto& player : players.GetAllPlayers()) {
		app::serialization::PlayerRepr pr;
		pr.id = player.GetId();

		const model::Dog* dog = player.GetDog();
		const model::GameSession* session = player.GetSession();

		pr.dog_id = dog->GetId();

		const model::Map* map = session->GetMap();
		pr.map_id = *map->GetId();

		ps.players.push_back(std::move(pr));
	}

	ps.last_player_id = players.GetLastPlayerId();

	for (const auto& [token, player_ptr] : tokens.GetAll()) {
		if (!player_ptr) {
			continue;
		}

		app::serialization::TokenRepr tr;
		tr.token = *token;
		tr.player_id = player_ptr->GetId();
		ps.tokens.push_back(std::move(tr));
	}

	return ps;
}

static void RestorePlayersFromState(const app::serialization::PlayersRepr& ps, model::Game& game,
												app::Players& players, app::PlayerTokens& tokens) {
	for (const auto& pr : ps.players) {
		const auto* map = game.FindMap(model::Map::Id{pr.map_id});
		if (!map) {
			continue;
		}

		model::GameSession* session = game.FindSessionByMap(map);

		if (!session) {
			continue;
		}

		model::Dog* dog = session->FindDogById(pr.dog_id);
		if (!dog) {
			continue;
		}

		app::Player* player = players.AddExisting(session, dog, pr.id);
		(void)player;
	}

	players.SetLastPlayerId(ps.last_player_id);

	std::unordered_map<uint64_t, app::Player*> id_to_player;
	for (auto& p : players.GetAllPlayers()) {
		id_to_player[p.GetId()] = &p;
	}

	for (const auto& tr : ps.tokens) {
		auto it = id_to_player.find(tr.player_id);
		if (it == id_to_player.end()) {
			continue;
		}
		app::Token token{tr.token};
		tokens.SetTokenForPlayer(token, it->second);
	}
}

void SerializeState(const std::string& file, const model::Game& game, const app::Players& players,
						  const app::PlayerTokens& tokens) {
	fs::path target{file};
	fs::path tmp = target.string() + ".tmp";
	serialization::StateRepr sr;
	sr.game_state = MakeStateFromGame(game);
	sr.players_state = MakePlayersState(players, tokens);

	std::ofstream ofs(tmp, std::ios::binary);
	if (!ofs) {
		throw std::runtime_error("Failed to open temp state file");
	}

	boost::archive::text_oarchive oar(ofs);
	oar & sr;
	fs::rename(tmp, target);
}

void DeserializeState(const std::string& file, model::Game& game, app::Players& players,
							 app::PlayerTokens& tokens) {
	const fs::path path{file};
	if (!fs::exists(path)) {
		return;
	}

	serialization::StateRepr sr;
	std::ifstream ifs(path, std::ios::binary);
	if (!ifs) {
		throw std::runtime_error("Failed to open state file");
	}
	boost::archive::text_iarchive iar(ifs);
	iar & sr;

	RestoreGameFromState(sr.game_state, game);
	RestorePlayersFromState(sr.players_state, game, players, tokens);
}

void SerializeGameState(const std::string& file, const model::Game& game) {
	fs::path target{file};
	fs::path tmp = target.string() + ".tmp";

	model::GameStateRepr state = MakeStateFromGame(game);

	std::ofstream ofs(tmp, std::ios::binary);
	if (!ofs) {
		throw std::runtime_error("Failed to open temp state file");
	}
	boost::archive::text_oarchive oar(ofs);
	oar & state;

	fs::rename(tmp, target);
}

void DeserializeGameState(const std::string& file, model::Game& game) {
	const fs::path path{file};
	if (!fs::exists(path)) {
		return;
	}

	model::GameStateRepr state;

	std::ifstream ifs(path, std::ios::binary);
	if (!ifs) {
		throw std::runtime_error("Failed to open state file");
	}
	boost::archive::text_iarchive iar(ifs);
	iar & state;

	RestoreGameFromState(state, game);
}

} // namespace serialization

