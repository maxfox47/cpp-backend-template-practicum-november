#pragma once

#include "model.h"

#include <boost/json.hpp>
#include <chrono>
#include <deque>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

namespace json = boost::json;

namespace app {

struct PlayerInfo {
	geom::Point2D pos;
	geom::Vec2D speed;
	model::Direction dir;
	int score;
};

class Player {
 public:
	explicit Player(model::GameSession* session, model::Dog* dog, uint64_t id)
		 : dog_(dog), session_(session), id_(id) {
		if (!session)
			throw std::invalid_argument("Session cannot be null");
		if (!dog)
			throw std::invalid_argument("Dog cannot be null");
	}

	std::string GetName() const {
		if (!dog_) {
			return "unknown";
		}
		return dog_->GetName();
	}

	uint64_t GetId() const { return id_; }

	PlayerInfo GetInfo() const {
		return {dog_->GetPosition(), dog_->GetSpeed(), dog_->GetDirection(), dog_->GetScore()};
	}

	const model::Dog* GetDog() const { return dog_; }

	const model::GameSession* GetSession() const { return session_; }

	void SetSpeed(geom::Vec2D speed) { dog_->SetSpeed(speed); }

	void SetDirection(model::Direction dir) { dog_->SetDirection(dir); }

	double GetDefaultSpeed() const { return session_->GetDefaultSpeed(); }

	const std::vector<model::TakenItem>& GetBag() const { return dog_->GetBag(); }

	// Игровое время, проведённое в сессии, берём из модели пса,
	// которая накапливается по тикам игрового времени.
	double GetTotalPlayTime() const {
		if (!dog_) {
			return 0.0;
		}
		return dog_->GetPlayTime();
	}

 private:
	model::Dog* dog_;
	model::GameSession* session_;
	uint64_t id_;
};

class Players {
 public:
	Player* Add(model::GameSession* session, model::Dog* dog);
	Player* FindByDogIdAndMapId(int dog_id, const std::string& map_id);
	std::vector<std::string> GetNames() const;
	json::object GetPlayersInfo() const;
	const std::deque<Player>& GetAllPlayers() const noexcept { return players_; }
	std::deque<Player>& GetAllPlayers() noexcept { return players_; }
	uint64_t GetLastPlayerId() const noexcept { return last_player_id_; }
	void SetLastPlayerId(uint64_t id) noexcept { last_player_id_ = id; }
	Player* AddExisting(model::GameSession* session, model::Dog* dog, uint64_t id) {
		if (!session || !dog) {
			return nullptr;
		}

		players_.push_back(Player(session, dog, id));
		return &players_.back();
	}

 private:
	std::deque<Player> players_;
	uint64_t last_player_id_ = 0;
};

namespace detail {
struct TokenTag {};
} // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;

class PlayerTokens {
 public:
	PlayerTokens() = default;

	Player* FindPlayerByToken(Token token) {
		auto it = token_to_player_.find(token);
		if (it != token_to_player_.end()) {
			return it->second;
		}

		return nullptr;
	}

	Token AddPlayer(Player* player) {
		Token token = MakeToken();
		token_to_player_[token] = player;

		return token;
	}

	Token MakeToken() {
		std::stringstream ss;
		uint64_t num1 = generator1_();
		uint64_t num2 = generator2_();

		ss << std::hex << std::setw(16) << std::setfill('0') << num1;
		ss << std::hex << std::setw(16) << std::setfill('0') << num2;

		return Token{ss.str()};
	}

	const std::unordered_map<Token, Player*, util::TaggedHasher<Token>>& GetAll() const noexcept {
		return token_to_player_;
	}

	std::unordered_map<Token, Player*, util::TaggedHasher<Token>>& GetAll() noexcept {
		return token_to_player_;
	}

	void SetTokenForPlayer(const Token& token, Player* player) { token_to_player_[token] = player; }

 private:
	std::random_device random_device_;
	std::mt19937_64 generator1_{[this] {
		std::uniform_int_distribution<std::mt19937_64::result_type> dist;
		return dist(random_device_);
	}()};
	std::mt19937_64 generator2_{[this] {
		std::uniform_int_distribution<std::mt19937_64::result_type> dist;
		return dist(random_device_);
	}()};

	std::unordered_map<Token, Player*, util::TaggedHasher<Token>> token_to_player_;
};

} // namespace app
